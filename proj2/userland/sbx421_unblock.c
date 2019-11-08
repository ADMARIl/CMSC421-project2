/*
 * File: sbx421_unblock.c
 * Created by Andrew Ingson (aings1@umbc.edu)
 * Date: 11/8/19
 * CMSC 421 (Principles of Operating Systems) proj2
 * 
 * Userland wrapper code sbx421_unblock syscall
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zconf.h>
#include <errno.h>
#include <linux/kernel.h>
#include <sys/syscall.h>

#define sys_sbx421_unblock 435

int main(int argc, char *argv[]) {
    printf("Userland sbx421_unblock\n");
    return 0;
}