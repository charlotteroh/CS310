// test case 6
// deadlock
#include <stdio.h>
#include "thread.h"

void deadlock(int cond) {
  thread_lock(0);
  for (int i=0; i<10; i++) {
    thread_signal(0, (cond+1)%10);
    cout << "Woke thread up\n";
    thread_wait(0, cond);
  }
  thread_unlock(0);
}

void init() {
  for (int i=0; i<100; i++) {
    thread_create((thread_startfunc_t) deadlock, (void *) (i%10));
  }
}

int main() {
  thread_libinit((thread_startfunc_t) init, NULL);
  return 0;
}
