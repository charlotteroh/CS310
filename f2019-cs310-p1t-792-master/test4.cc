// test case 4-1
// deadlock
// from midterm question
#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>
using namespace std;

int soda = 0;

void threadA(void *a) {
  thread_lock(1);
  // wait for sognal
  while (soda == 1) {
    thread_yield();
    thread_wait(1,2);
  }
  // increment
  soda = soda+1;
  thread_signal(1,2);
  thread_yield();
  thread_unlock(1);
  cout << "A done\n";
}

void threadB(void *a) {
  thread_lock(1);
  // wait for signal
  while (soda == 1) {
    thread_yield();
    thread_wait(1,2);
  }
  // increment
  soda = soda+1;
  thread_signal(1,2);
  thread_yield();
  thread_unlock(1);
  cout << "B done\n";
}

void userA(void *a) {
  thread_lock(1);
  // wait for producer signal
  while (soda == 0) {
    thread_yield();
    thread_wait(1,2);
  }
  // decrement
  soda = soda-1;
  thread_signal(1,2);
  thread_yield();
  thread_unlock(1);
  cout << "User A done\n";
}

void userB(void *a) {
  thread_lock(1);
  // wait for producer signal
  while (soda == 0) {
    thread_yield();
    thread_wait(1,2);
  }
  // decrement
  soda = soda-1;
  thread_signal(1,2);
  thread_yield();
  thread_unlock(1);
  cout << "User B done\n";
}

void parent(void* a) {
  thread_create((thread_startfunc_t) userA, a);
  thread_create((thread_startfunc_t) userB, a);
  thread_create((thread_startfunc_t) threadB, a);
  thread_yield();
  threadA(a);
}

int main() {
  if (thread_libinit( (thread_startfunc_t) parent, (void *) 100)) {
    cout << "thread_libinit failed\n";
    exit(1);
  }
}
