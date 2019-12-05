/*
 * File: proj2Proto_block.c
 * Created by Andrew Ingson (aings1@umbc.edu)
 * Date: 12/5/19
 * CMSC 421 (Principles of Operating Systems) proj2
 * 
 * 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../misc/proj2Proto.h"

int main(int argc, char *argv[]) {
    printf("Userland sbx421_block\n");

    sbx_init();
    // read from command line args and cast them to the right data types
    pid_t pid = atol(argv[1]);
    unsigned long sysID =  atol(argv[2]);

    // run the syscall with those args
    skipList_print(sysID);
    return 0;
}