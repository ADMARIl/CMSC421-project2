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
    printf("proj2Proto_driver\n");

    //syscall_entry *syscallArray[437];
    for (int i =0; i < 437; i++) {
        SC_ARR[i] = NULL;
    }

    mbx421_create(100,100);
    mbx421_create(100,200);
    mbx421_create(100,300);
    mbx421_create(100,10);
    mbx421_create(100,700);

    skipList_print(100);

    mbx421_destroy(100,100);
    mbx421_destroy(100,200);
    //mbx421_destroy(100,300);
    mbx421_destroy(100,10);
    mbx421_destroy(100,700);

    skipList_print(100);

    return 0;
}
