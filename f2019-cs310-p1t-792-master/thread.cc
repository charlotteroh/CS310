#include "interrupt.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <queue>
#include <deque>
#include <utility>
#include <iostream>
#include <map>
#include <algorithm>
#include <vector>
using namespace std;

// storage
static ucontext_t *store;
static ucontext_t *running;

typedef unsigned int lock_t;
typedef pair<unsigned int, unsigned int> lock_ct;

static deque<ucontext_t*> ready_queue; // ready queue
static map<lock_t, deque<ucontext_t *> > lock_queue; // lock queue
static map<lock_ct, deque<ucontext_t *> > cv_queue; // cv queue

// lock map: use in lock/unlock, wait, signal/broadcast
static map<lock_t, ucontext_t *> lockmap;

static int complete;
static map<ucontext_t *, bool> completethread; // true if complete

static bool thread_libinit_called = false; // check whether libinit is called

// interrupts
int interrupt_enable(int ret) {
  interrupt_enable();
  return ret;
}

// free
void freemem(void *ucontext_ptr) {
  free(((ucontext_t *) ucontext_ptr)->uc_stack.ss_sp);
  free(ucontext_ptr);
}

int interrupt_disable(int ret) {
  interrupt_disable();
  return ret;
}

void edge(bool done) {
  map<ucontext_t *, bool>::iterator it = completethread.begin();
  if (done) free(store);
  while (it != completethread.end()) {
    if ((it->first != running) && (done || it->second)) {
      freemem(it->first);
      completethread.erase(it++);
    }
    else {
      it ++;
    }
  }
  complete_threads = 0;
}

int done() {
  edge(true);
  cout << "Thread library exiting.\n";
  exit(0);
  return 0;
}

// start function used in create
void start(void *ucontext_ptr, thread_startfunc_t func, void *arg) {
  interrupt_enable();
  func(arg);

  completethread[(ucontext_t*) ucontext_ptr] = true;
  complete++;

  // free CPU
  thread_yield();
}

// CODE GOES HERE
int thread_libinit(thread_startfunc_t func, void *arg) {
  /* thread_libinit initializes the thread library. A user program should call
  thread_libinit exactly once (before calling any other thread functions).
  thread_libinit creates and runs the first thread. This first thread is
  initialized to call the function pointed to by func with the single argument
  arg. Note that a successful call to thread_libinit will not return to the
  calling function. Instead, control transfers to func, and the function
  that calls thread_libinit will never execute again. */

  if (store != NULL) {return -1;}
  running = store = NULL;
  complete = 0;
  store = (ucontext_t *) malloc(sizeof(ucontext_t));

  if (store == NULL) {return -1;}
  // create initial thread
  if (thread_create(func, arg) == -1) {return -1;}
  // start initial thread
  if (thread_yield() == -1) {return -1;}
  // not executed
  return -1;
}

int thread_create(thread_startfunc_t func, void *arg){
  /* thread_create is used to create a new thread. When the newly created
  thread starts, it will call the function pointed to by func and pass
  it the single argument arg. */
  interrupt_disable();
  if (store == NULL) {
    return interrupt_enable(-1);
  }

  void* stack = malloc(STACK_SIZE);
  ucontext_t* ucontext_ptr = (ucontext_t *) malloc(sizeof(ucontext_t));

  if (stack == NULL || ucontext_ptr == NULL) {
    free(stack);
    free(ucontext_ptr);
    return interrupt_enable(-1);
  }
  /*
  * Initialize a context structure by copying the current thread's context.
  */
  getcontext(ucontext_ptr); // ucontext_ptr has type (ucontext_t *)

  if (getcontext(ucontext_ptr) == -1) {return -1;}
  /*
  * Direct the new thread to use a different stack. Your thread library
  * should allocate STACK_SIZE bytes for each thread's stack.
  */
  ucontext_ptr->uc_stack.ss_sp = stack;
  ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
  ucontext_ptr->uc_stack.ss_flags = 0;
  ucontext_ptr->uc_link = NULL;

  /*
  * Direct the new thread to start by calling start(arg1, arg2).
  */
  makecontext(ucontext_ptr, (void (*)()) start, 3, ucontext_ptr, func, arg);

  completethread[ucontext_ptr] = false;
  ready_queue.push_back(ucontext_ptr);

  return interrupt_enable(0);
}

int thread_yield(void) {
  /* thread_yield causes the current thread to yield the CPU to the next
  runnable thread. It has no effect if there are no other runnable threads.
  thread_yield is used to test the thread library. A normal concurrent program
  should not depend on thread_yield; nor should a normal concurrent program
  produce incorrect answers if thread_yield calls are inserted arbitrarily. */
  interrupt_disable();
  if (store == NULL) {
    return interrupt_enable(-1);
  }

  // done with threads!
  if (ready_queue.size() == 0) {
    if (completethread[running]) {
      done();
    }
    else {
      return interrupt_enable(0);
    }
  }

  // memory
  if (complete > 10) {
    edge(false);
  }

  // edge cases
  bool start = running == NULL;
  bool stay = start || completethread[running];

  if (stay == false) {
    ready_queue.push_back(running);
  }

  // run thread, and set thread to running
  running = ready_queue.front();
  ready_queue.pop_front();

  if (swapcontext(stay ? store : ready_queue.back(), running) == -1) {
    return -1;
  }
  if (start == false) {
    interrupt_enable();
  }
  return 0;
}

int thread_lock(lock_t lock) {
  /* A lock is identified by an unsigned integer (0 - 0xffffffff). Each lock
  has a set of condition variables associated with it (numbered 0 - 0xffffffff),
  so a condition variable is identified uniquely by the tuple (lock number,
  cond number). Programs can use arbitrary numbers for locks and condition
  variables (i.e., they need not be numbered from 0 - n). */

  interrupt_disable();

  if (store == NULL) {
    return interrupt_enable(-1);
  }

  // NULL if lock isn't there
  if (lockmap.find(lock) == lockmap.end()) {
    lockmap[lock] = NULL;
  }

  // error if no lock
  if (lockmap[lock] == running) {
    return interrupt_enable(-1);
  }

  // lock is occupied
  if (lockmap[lock] != NULL) {
    lock_queue[lock].push_back(running);
    if (ready_queue.size() == 0) {
      interrupt_enable();
      done();
    }
    running = ready_queue.front();
    ready_queue.pop_front();
    if (swapcontext(lock_queue[lock].back(), running) == -1) {
      return -1;
    }
    else {
      lockmap[lock] = running;
    }

  return interrupt_enable(0);
}

int thread_unlock(lock_t lock) {
  interrupt_disable();

  if (store == NULL) {
    return interrupt_enable(-1);
  }

  // NULL if lock isn't there
  if (lockmap.find(lock) == lockmap.end()) {
    lockmap[lock] = NULL;
  }

  // error if no lock
  if (lockmap[lock] != running) {
    return interrupt_enable(-1);
  }

  // unlock
  if (lock_queue[lock].size() > 0) {
    lockmap[lock] = lock_queue[lock].front();
    ready_queue.push_back(lock_queue[lock].front());
    lock_queue[lock].pop_front();
  }
  else {
    lockmap[lock] = NULL;
  }

  return interrupt_enable(0);
}

int thread_wait(lock_t lock, lock_t cond) {
  interrupt_disable();
  if (store == NULL) {
    return interrupt_enable(-1);
  }

  lock_ct lock_cv = make_pair(lock, cond);

  // NULL if lock isn't there
  if (lockmap.find(lock) == lockmap.end()) {
    lockmap[lock] = NULL;
  }
  // error if no lock
  if (lockmap[lock] != running) {
    return interrupt_enable(-1);
  }

  // unlock
  if (lock_queue[lock].size() > 0) {
    lockmap[lock] = lock_queue[lock].front();
    ready_queue.push_back(lock_queue[lock].front());
    lock_queue[lock].pop_front();
  }
  else {
    lockmap[lock] = NULL;
  }

  // go to cv queue
  cv_queue[lock_cv].push_back(running);

  // yield
  if (ready_queue.size() == 0) {
    interrupt_enable();
    done();
  }
  running = ready_queue.front();
  ready_queue.pop_front();
  if (swapcontext(cv_queue[lock_cv].back(), running) == -1) {
    return -1;
  }
  interrupt_enable();

  // lock again
  return thread_lock(lock);
}

int thread_signal(lock_t lock, lock_t cond) {
  // wake only one waiting thread
  interrupt_disable();
  if (store == NULL) {
    return interrupt_enable(-1);
  }
  lock_ct lock_cv = make_pair(lock, cond);

  // NULL if lock isn't there
  if (lockmap.find(lock) == lockmap.end()) {
    lockmap[lock] = NULL;
  }

  // signal
  if (cv_queue[lock_cv].size() == 0) {
    return interrupt_enable(0);
  }

  // put to end of ready queue and delete from cv queue
  ready_queue.push_back(cv_queue[lock_cv].front());
  cv_queue[lock_cv].pop_front();

  return interrupt_enable(0);
}

int thread_broadcast(lock_t lock, lock_t cond) {
  // wake all waiting threads
  interrupt_disable();
  if (store == NULL) {
    return interrupt_enable(-1);
  }
  lock_ct lock_cv = make_pair(lock, cond);

  // NULL if lock isn't there
  if (lockmap.find(lock) == lockmap.end()) {
    lockmap[lock] = NULL;
  }

  // for every waiting thread, put to end of ready queue and delete from cv queue
  for (int i = 0; i < cv_queue[lock_cv].size(); i ++) {
    ready_queue.push_back(cv_queue[lock_cv].front());
    cv_queue[lock_cv].pop_front();
  }

  return interrupt_enable(0);
}
