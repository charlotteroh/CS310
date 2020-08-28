// test case 1
// test if order of calling is correct
#include <stdio.h>
#include "thread.h"

void lockerror(void) {
  thread_lock(1);
  if (thread_lock(1) == -1) {
    cout << "Lock isn't unlocked yet\n";
  }
  thread_unlock(1);
  if (thread_unlock(1) == -1) {
    cout << "Unlocked before initializing lock\n";
  }
  thread_unlock(1);
  if (thread_unlock(1) == -1) {
    cout << "Unlocked before initializing lock\n";
  }
  thread_unlock(1);
  if (thread_unlock(1) == -1) {
    cout << "Unlocked before initializing lock\n";
  }
  if (thread_wait(1,1) == -1) {
    cout << "Thread waiting without locked\n";
  }
}

void initerror(void) {
  if(thread_libinit((thread_startfunc_t) lockerror, NULL) == -1)
    cout << "Thread already initialized\n";
  thread_create((thread_startfunc_t) lockerror, NULL);
}

int main() {
  if (thread_create((thread_startfunc_t) lockerror, NULL) == -1) {
    cout << "Thread created before initialized\n";
  }
  if (thread_yield() == -1) {
    cout << "Thread yielded before initialized\n";
  }
  if (thread_lock(1) == -1) {
    cout << "Thread locked before initialized\n";
  }
  if (thread_unlock(1) == -1) {
    cout << "Thread unlocked before initialized\n";
  }
  if (thread_yield() == -1) {
    cout << "Thread yielded before initialized\n";
  }
  if (thread_wait(1,1) == -1) {
    cout << "Thread waiting before initialized\n";
  }
  if (thread_signal(1,1) == -1) {
    cout << "Thread signaling before initialized\n";
  }
  if (thread_broadcast(1,1) == -1) {
    cout << "Thread broadcasted before initialized\n";
  }

  thread_libinit((thread_startfunc_t) initerror, NULL);
  return 0;
}
