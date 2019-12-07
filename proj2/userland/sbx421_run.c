/*
 * File: sbx421_run.c
 * Created by Andrew Ingson (aings1@umbc.edu)
 * Date: 11/8/19
 * CMSC 421 (Principles of Operating Systems) proj2
 * 
 * Code to run any other application on system with credentials of another user
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zconf.h>
#include <errno.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>


#define sys_sbx421_block 434
#define sys_sbx421_unblock 435
#define sys_sbx421_count 436

int main(int argc, char *argv[]) {
    printf("Userland sbx421_run\n");

    uid_t uid = getuid();
    uid_t euid = geteuid();

    if (uid > 0 || euid > 0) {
        return -EPERM;
    }


    return 0;
}