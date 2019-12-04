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

    syscall_entry *syscallArray[437];
    for (int i =0; i < 437; i++) {
        syscallArray[i] = NULL;
    }

    return 0;
}
