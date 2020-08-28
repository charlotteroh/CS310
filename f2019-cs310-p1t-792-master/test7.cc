// test case 7
// buggy broadcast
#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>
using namespace std;

void broad(void* arg) {
	if (thread_broadcast(1,1) < 0) {
		cout << "Broadcast fail";
	}
	if (thread_signal(1,1) < 0) {
		cout << "Signal fail";
	}
	exit(0);
}

void init(void* arg) {
  if (thread_create((thread_startfunc_t) broad, (void*) 100) < 0) {
    cout << "Create broadcast fail\n";
    exit(1);
  }
}

int main() {
	if (thread_libinit((thread_startfunc_t) init, (void *) 100)) {
		cout << "thread_libinit failed\n";
	}
}
