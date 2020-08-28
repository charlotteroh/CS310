import java.util.*;

public class RaftNode {

	private RaftLog mLog;
	private Timer mTimer;
	private AppendEntryRequester mAppendEntryRequester;
	private VoteRequester mVoteRequester;
	private int mNumServers;
	private int mNodeServerNumber;
	private RaftMode mCurrentMode;
	private int mTerm;
	private int mCommitIndex;

  // leader timeout
  private int[] nextIndex;
  private int[] matchIndex;
  private Map<Integer, Boolean> voteStatus;

	/**
	 * RaftNode constructor, called by test harness.
	 * @param log this replica's local log, pre-populated with entries
	 * @param timer this replica's local timer
	 * @param sendAppendEntry call this to send RPCs as leader and check responses
	 * @param sendVoteRequest call this to send RPCs as candidate and check responses
	 * @param numServers how many servers in the configuration (numbered starting at 0)
	 * @param nodeServerNumber server number for this replica (this RaftNode object)
	 * @param currentMode initial mode for this replica
	 * @param term initial term (e.g., last term seen before failure).  Terms start at 1.
	 **/
	public RaftNode(
			RaftLog log,
			Timer timer,
			AppendEntryRequester sendAppendEntry,
			VoteRequester sendVoteRequest,
			int numServers,
			int nodeServerNumber,
			RaftMode currentMode,
			int term) {
		mLog = log;
		mTimer = timer;
		mAppendEntryRequester = sendAppendEntry;
		mVoteRequester = sendVoteRequest;
		mNumServers = numServers;
		mNodeServerNumber = nodeServerNumber;
		mCurrentMode = currentMode;
		mTerm = term;
    mCommitIndex = -1;
    // votes
    nextIndex = new int[mNumServers];
    matchIndex = new int[mNumServers];
    Arrays.fill(nextIndex, mLog.getLastEntryIndex()+1);
    voteStatus = new HashMap<>();
	}

	public RaftMode getCurrentMode() {
		return mCurrentMode;
	}

	public int getCommitIndex() {
		return mCommitIndex;
	}

	public int getTerm() {
		return mTerm;
	}

	public RaftLog getCurrentLog() {
		return mLog;
	}

	public int getServerId() {
		return mNodeServerNumber;
	}

	public int getLastApplied() {
		return mLog.getLastEntryIndex();
	}

  public boolean votedInTerm(int term) {
	// Return whether or not this RaftNode has voted in specified term.
	// Note: called only for current term or future term.
    if (!voteStatus.containsKey(term)) return false;
	  return voteStatus.get(term);
	}

	/**
	 * @param candidateTerm candidate's term
	 * @param candidateID   candidate requesting vote
	 * @param lastLogIndex  index of candidate's last log entry
	 * @param lastLogTerm   term of candidate's last log entry
	 * @return 0, if server votes for candidate; otherwise, server's current term
	 */
	public int receiveVoteRequest(int candidateTerm,
								  int candidateID,
								  int lastLogIndex,
								  int lastLogTerm) {
    if (candidateTerm <= mTerm) return mTerm;
    if (mTerm < candidateTerm) mCurrentMode = RaftMode.FOLLOWER;

    mTerm = candidateTerm;
    if (lastLogTerm < mLog.getLastEntryTerm()) return mTerm;

    if (lastLogIndex >= mLog.getLastEntryIndex()) {
			voteStatus.put(mTerm, true);
			return 0;
		}

    if (candidateID == mNodeServerNumber) {
      voteStatus.put(mTerm, true);
      return 0;
    }

		return mTerm;
	}

	/**
	 * @param leaderTerm   leader's term
	 * @param leaderID     current leader
	 * @param prevLogIndex index of log entry before entries to append
	 * @param prevLogTerm  term of log entry before entries to append
	 * @param entries      entries to append (in order of 0 to append.length-1)
	 * @param leaderCommit index of highest committed entry
	 * @return 0, if server appended entries; otherwise, server's current term
	 */
	public int receiveAppendEntry(int leaderTerm,
								  int leaderID,
								  int prevLogIndex,
								  int prevLogTerm,
								  Entry[] entries,
								  int leaderCommit) {

    if (leaderTerm >= mTerm && (mCurrentMode == RaftMode.CANDIDATE || mCurrentMode == RaftMode.LEADER)) {
    	mCurrentMode = RaftMode.FOLLOWER;
    }
    mTimer.resetTimer();

    if (leaderTerm < mTerm) {
      return mTerm;
    }

    if (prevLogIndex > mLog.getLastEntryIndex() || prevLogIndex < 0 || mLog.getEntry(prevLogIndex).term != prevLogTerm) {
      if (prevLogIndex >= 0) {
        return mTerm;
      }
    }

    int idx = 0;
    boolean diff = false;

    for (int i=0; i<=Math.min(mLog.getLastEntryIndex(), entries.length - 1); i++) {
      if (!entries[i].toString().equals(mLog.getEntry(i).toString())) {
        diff = true;
        break;
      }
      idx++;
    }

    mLog.insert(entries, idx);

    if (leaderCommit > mCommitIndex) {
      mCommitIndex = Math.min(leaderCommit, mLog.getLastEntryIndex());
    }

		return 0;
	}

	public int handleTimeout() {
    if (mCurrentMode == RaftMode.FOLLOWER) {
      // goes to candidate
      mCurrentMode = RaftMode.CANDIDATE;
      System.out.println("hello bitch");
      mTerm++;
      requestVotes();
      mTimer.resetTimer();
    }

    else if (mCurrentMode == RaftMode.CANDIDATE) {
      if (mVoteRequester.countYesResponses(mTerm) > mNumServers / 2 && mVoteRequester.maxResponseTerm(mTerm) <= mTerm) {
        mCurrentMode = RaftMode.LEADER;
        sendHeartbeat();
      }
      else if (mVoteRequester.maxResponseTerm(mTerm) > mTerm) {
        mCurrentMode = RaftMode.FOLLOWER;
        mTerm = mVoteRequester.maxResponseTerm(mTerm);
      }
      else {
        mTerm++;
       	voteStatus.put(mTerm, true);
        requestVotes();
        mTimer.resetTimer();
      }
    }

    else if (mCurrentMode == RaftMode.LEADER) {
      List<AppendResponse> response = mAppendEntryRequester.getResponses(mTerm);
      for (AppendResponse x: response) {
        if (x.success==false) {
          nextIndex[x.responderId] -= 1;
          if (x.term > mTerm) {
            mCurrentMode = RaftMode.FOLLOWER;
						mTerm = x.term;
          }
        }
        else {
          matchIndex[x.responderId] = mLog.getLastEntryIndex();
          nextIndex[x.responderId] = mLog.getLastEntryIndex() + 1;
        }
      }
      for (int i=0; i<=mLog.getLastEntryIndex(); i++) {
        int max = 0;
        int numOfMatch = 0;
        for (int j=0; j<mNumServers; j++) {
          if (matchIndex[j] >= i) {
            max++;
          }
        }
        if (max > mNumServers/2 && i > mCommitIndex && mLog.getEntry(i).term == mTerm) {
        	mCommitIndex = i;
				}
      }
      sendHeartbeat();
    }

    return -1;
	}

  public void sendHeartbeat() {
    for (int i=0; i<mNumServers; i++) {
      if (i != mNodeServerNumber) {
        int prevLogIndex = Math.max(-1,nextIndex[i]-1);
        int prevLogTerm = -1;
        if (mLog.getEntry(nextIndex[i]-1) != null) {
          prevLogTerm = mLog.getEntry(nextIndex[i]-1).term;
        }
        mAppendEntryRequester.send(i, mTerm, mNodeServerNumber, prevLogIndex, prevLogTerm, new Entry[0], mCommitIndex);
      }
    }
  }

	/**
	 *  Private method shows how to issue a round of RPC calls.
	 *  Responses come in over time: after at least one timer interval, call mVoteRequester to query/retrieve responses.
	 */
	private void requestVotes() {
		for (int i = 0; i < mNumServers; i++) {
			if (i != mNodeServerNumber) {
				mVoteRequester.send(i, mTerm, mNodeServerNumber, mLog.getLastEntryIndex(), mLog.getLastEntryTerm());
			}
		}
	}
}
