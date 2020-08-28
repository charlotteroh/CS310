// test case 5
// multiple yields
#include "thread.h"
#include <fstream>
#include <stdlib.h>
#include <iostream>
using namespace std;

void one() {
  thread_lock(0);
  if (thread_unlock(1) == -1) {
    cout << "One crash\n";
  }
  thread_unlock(0);
}

void two() {
  thread_lock(1);
  if (thread_unlock(0) == -1) {
    cout << "Two crash\n";
  }
  thread_unlock(1);
}

void init(void) {
    thread_lock(2);
    thread_yield();
    thread_create((thread_startfunc_t) two, NULL);
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_create((thread_startfunc_t) one, NULL);
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_unlock(2);
}

int main() {
    if (thread_create((thread_startfunc_t) two, NULL) == -1) {
      cout << "Error\n";
    }
    thread_libinit((thread_startfunc_t) init, NULL);
    return 0;
}
