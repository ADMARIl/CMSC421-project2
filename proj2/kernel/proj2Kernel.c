/*
 * File: proj2Kernel.c
 * Created by Andrew Ingson (aings1@umbc.edu)
 * Date: 11/8/19
 * CMSC 421 (Principles of Operating Systems) proj2
 *
 * Code for the 3 project 2 sys calls
 *
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(sbx421_block, pid_t, proc, unsigned long, nr) {
    return 0;
}

SYCALL_DEFINE2(sbx421_unblock, pid_t, proc, unsigned long, nr) {
    return 0;
}

SYSCALL_DEFINE2(sbx421_count, pid_t, proc, unsigned long, nr) {
    return 0;
}