// test case 3
// deli
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include "thread.h"
using namespace std;

// Enumerate function signatures
//int main(int argc, char *argv[]);
//void start(void *args);

// variables
struct order_info {
  int cid;
  int sid;
  order_info(){}
  order_info(int c, int s) {
    cid = c;
    sid = s;
  }
};

vector<order_info> board; // board

// locks
int board_lock = 1;
int cv_can_make = 2;
int cv_can_post = 3;

int cash_lock = 4;
int monitor = 5;

// completed orders
int* done;

int largest_num_of_orders = 0;
int cashier_size = 0;
int active_cashier = 0;
bool can_make = false;
bool ex = false;

const int cashiers = 10;

void sandwich_cashier(int* orders) {
  int last = -1;

  // lock when counting active cashiers
  thread_lock(cash_lock);
  last = active_cashier++;
  thread_unlock(cash_lock);

  // sandwich id and initialize last order id
  int sandwich = -1;
  done[last] = 1;

  // read order from file
  for (int i=0; i<5; i++) {
    sandwich = orders[i];
    // lock board
    thread_lock(board_lock);

    // while not done and still can make,
    // wait till previous order is completed by sandwich maker()
    while (done[last] == 0 || can_make) {
      thread_wait(board_lock, cv_can_post);
    }

    // post order to board
    board.push_back(order_info(last, sandwich));
    done[last] = 0;

    thread_lock(monitor);
    cout << "POSTED: cashier " << last << " sandwich " << sandwich << endl;
    thread_unlock(monitor);

    if (board.size() == largest_num_of_orders) {
      can_make = true;
      // send cv_can_make signal
      thread_signal(board_lock, cv_can_make);
    }

    thread_unlock(board_lock);
  }

  // wait for the last order
  // lock board lock
  thread_lock(board_lock);
  while (done[last] == 0) {
    thread_wait(board_lock, cv_can_post);
  }

  thread_unlock(board_lock);

  // cashiers are done
  // lock cashier lock while getting rid of cashiers
  thread_lock(cash_lock);
  if (largest_num_of_orders > --active_cashier) {

    // count number of cashiers
    largest_num_of_orders = active_cashier;

    // may exit if all cashiers are done
    if (active_cashier == 0) {
      ex = true;
    }

    // signal the board that cashier is empty
    // lock board while signaling
    thread_lock(board_lock);
    can_make = true;
    thread_signal(board_lock, cv_can_make);
    thread_unlock(board_lock);
  }
  // unlock cashier lock
  thread_unlock(cash_lock);

  // close file
  fclose(file);
}

void sandwich_make() {
  int last = -1;

  while (1) {
    int idx = 0;
    int big = 1000;
    order_info agenda;

    // lock board
    thread_lock(board_lock);
    // wait if can't make sandwich
    while (can_make == false) {
      thread_wait(board_lock, cv_can_make);
    }
    // check if can exit
    if (ex==true) break;

    // find closest sandwich of closest distance
    for (int i = 0; i < board.size(); i ++) {
      int distance = abs(board[i].sid - last);
      if (distance < big) {
        // update values
        idx = i;
        big = distance;
        agenda = board[i];
      }
    }

    // remove from board
    board.erase(board.begin() + idx);

    // last sandwich made
    last = agenda.sid;

    // update completed list
    done[agenda.cid] = 1;

    // lock monitor while printing
    thread_lock(monitor);
    cout << "READY: cashier " << agenda.cid << " sandwich " << agenda.sid << endl;
    thread_unlock(monitor);

    // broadcast to cashiers
    can_make = false;
    thread_broadcast(board_lock, cv_can_post);

    // unlock board lock
    thread_unlock(board_lock);
  }

  free(done);
}

void start(void) {
  // Code goes here
  //start_preemptions(true, true, 1);
  srand(1024);

  // allocate memory
  done = (int *) malloc(sizeof(int) * cashier_size);

  int** sandID = (int **) malloc(sizeof(int *) * cashiers);
  if (sandwichID == NULL) {
    return;
  }
  for (int i=0; i < cashiers; i ++) {
    sandID[i] = (int *) malloc(sizeof(int) * cashiers*50);
    if (sandID[i] == NULL) {
      return;
    }
    for (int j=0; j < cashiers*50; j ++) {
      sandID[i][j] = rand()%1000;
    }
  }

  // create threads for cashier, cashier starts from 2
  for (int i=0; i <= cashiers; i ++) {
    thread_create((thread_startfunc_t) sandwich_cashier, (void *) sandID[i]);
  }

  // create thread for sandwich maker
  thread_create((thread_startfunc_t) sandwich_make, NULL);
}

int main(int argc, char* argv[]) {

  // error case
  if (argc < 2) {
    cout << "Not enough inputs" << endl;
    return (0);
  }

  // initialize variables
  cashier_size = cashiers;
  active_cashier = 0;
  can_make = false;
  largest_num_of_orders = 50;
  ex = cashier_size <= 0;

  // edge cases
  if (largest_num_of_orders < cashier_size) {
    largest_num_of_orders = largest_num_of_orders;
  }
  else {
    largest_num_of_orders = cashier_size;
  }

  // initialize thread library
  thread_libinit((thread_startfunc_t) start, NULL);

  return 0;
}
