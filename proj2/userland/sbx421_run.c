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
#include <string.h>
#include <zconf.h>
#include <errno.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <pwd.h>


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

    // used the command line args
    char* file = argv[2];
    char* cmd = argv[3];

    FILE *dataFile = fopen(file, "r");
    // check to see if there was anything in the file
    if (dataFile == NULL)
        return -ENOENT;
    // make the char arrays to hold the incomming file data
    char pids [447];
    char sysIDs [447];

    // get all the data using fscanf
    fscanf(dataFile,"%446s",pids);

    // loop over the file data until we reach the end of file marker
    while(fscanf(dataFile, "%446s", pids) != EOF) {
        scanf((const char *) dataFile, "%s", sysIDs);
        // get the pid
        pid_t pid = atol(pids);
        long sysNum = atol(sysIDs);
        syscall(sys_sbx421_block, pid, sysIDs);
        //fscanf(dataFile, "%s", pids);
    }
    // closing the file because CMSC 201 said that is good practice
    fclose(dataFile);


    struct passwd *usr = getpwnam(argv[1]);



    return 0;
}