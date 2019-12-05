/*
 * File: proj2Proto_driver.c
 * Created by Andrew Ingson (aings1@umbc.edu)
 * Date: 12/2/19
 * CMSC 421 (Principles of Operating Systems) proj2
 * 
 * Driver file for the project 2 prototype
 * 
 */

#include "proj2Proto.h"

int main() {
    printf("proj2Proto_driver.c\n\n");
    // initialize array of syscalls
    sbx_init();

    int pids[] = {5, 3, 2, 8, 14, 443, 80, 87, 22, 90, 56, 2};

    // add process ids to the sandboxes in different orders
    for (int i = 0; i < 12; i++) {
        skipList_create(10, pids[i]);
    }
    for (int i = 0; i < 12; i+=2) {
        skipList_create(414, pids[i]);
    }
    for (int i = 11; i >= 0; i--) {
        skipList_create(219, pids[i]);
    }

    skipList_create(100, 100);
    skipList_create(100, 200);
    skipList_create(100, 300);
    skipList_create(100, 10);
    skipList_create(100, 700);

    // test to see if search is working
    // should print DNF
    if (skipList_search(100, 201) == 0) {
        printf("Found %d\n", 201);
    } else {
        printf("DNF\n");
    }

    // print the current status of the sandboxes
    // TODO: Test if this does anything weird when the box isn't initialized
    skipList_print(10);
    skipList_print(100);
    skipList_print(219);
    skipList_print(414);

    skipList_destroy(100, 100);
    skipList_destroy(100, 200);
    //skipList_destroy(100,300);
    skipList_destroy(100, 10);
    skipList_destroy(100, 700);

    skipList_print(100);

    printf("\n\n @@@@@  Testing Primary Syscalls @@@@@ \n\n");

    // block test
    printf("\nBLOCK: 100 & 200 should appear if you are root. PID of process should appear regardless of UID\n");
    sbx421_block(100,100);
    sbx421_block(200,100);

    sbx421_block(0,100);

    skipList_print(100);

    // count test
    printf("\nCOUNT: should be 0 (if you are root?)\n");
    printf("%lu\n",sbx421_count(100, 100));

    // unblock test
    printf("\nUNBLOCK: 300 should not be here if you are root\n");
    sbx421_unblock(300,100);
    skipList_print(100);

    return 0;
}
