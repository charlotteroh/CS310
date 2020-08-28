// test case 2
// memory crash
#include "thread.h"
#include <fstream>
#include <stdlib.h>
#include <iostream>
using namespace std;

void func(void* arg){
	int i = 1;
}
void init(void *arg) {
	for (int i = 0; i<1000; i++){
		if (thread_create(func, arg) == -1) {
			cout << "Thread not created\n";
		};
		thread_yield();
	}
}

int main(int argc, char *argv[]) {
	thread_libinit((thread_startfunc_t) init, (void*) NULL);
	exit(0);
	return 1;
}
