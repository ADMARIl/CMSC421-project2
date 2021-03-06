/*
 * File: proj2Proto.h
 * Created by Andrew Ingson (aings1@umbc.edu)
 * Date: 12/2/19
 * CMSC 421 (Principles of Operating Systems) proj2
 * 
 * Prototype code for project 2 syscalls
 * 
 */

#ifndef PROJ2_PROTO_H
#define PROJ2_PROTO_H

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <zconf.h>
#include <stdbool.h>
//#include <rwlock.h>
//#include <cred.h>


// TODO: Move these struct definitions to /include so we can do external variables

typedef struct syscall_entry {
    struct skipList_node *sl_head;
    struct skipList_node *sl_tail;
    int numProcesses;
    bool init_state;
    unsigned int sl_size;
} syscall_entry;

struct skipList_node {
    //struct list_head link;
    pid_t process_id;
    unsigned int towerHeight;
    int blockCount;
    struct skipList_node** next;

};

unsigned int MAX_SL_SIZE = 10;
unsigned int PROB = 15000;
unsigned long NUM_SYS_CALLS = 437;
bool INIT_STATE = false;

syscall_entry *SC_ARR[437];

//
// provided random generation functions
//
static unsigned int next_random = 9001;

static unsigned int generate_random_int(void) {
    next_random = next_random * 1103515245 + 12345;
    return (next_random / 65536) % 32768;
}

// TODO: Why is the return from void thing important?

int sbx_init() {
    if (INIT_STATE) {
        return 0;
    } else {
        // initialize the syscall array
        int i;
        for (i = 0; i < 437; i++) {
            SC_ARR[i] = NULL;
        }
        INIT_STATE = true;
        return 0;
    }
}

int skipList_create(unsigned long sysID, pid_t id) {
    sbx_init();
    // check if root
    /*uid_t uid = current_uid().val;
    uid_t euid = current_euid().val;
    if (uid > 0)
        return EPERM;*/
    // check if pid is good first
    if (id < 0)
        return ENOENT;
    // check if sysID is in range
    if (sysID < 0 || sysID > NUM_SYS_CALLS)
        return ENODEV;
    // check if mailbox system has been initialized
    if (INIT_STATE == false)
        return ENODEV;
    // various vars to keep track of skipList parameters

    // lock

    if (SC_ARR[sysID] == NULL) {
        SC_ARR[sysID] = malloc(sizeof(syscall_entry));
        // create the head and tail skip list nodes
        SC_ARR[sysID]->sl_head = malloc(sizeof(struct skipList_node));
        SC_ARR[sysID]->sl_tail = malloc(sizeof(struct skipList_node));
        // set values to something we know is impossible
        SC_ARR[sysID]->sl_tail->process_id = -1;
        SC_ARR[sysID]->sl_head->process_id = -1;
        //SL_TAIL->process_id = 0;
        // make them the max size so we can assign pointers correctly later
        SC_ARR[sysID]->sl_tail->towerHeight = MAX_SL_SIZE;
        SC_ARR[sysID]->sl_head->towerHeight = MAX_SL_SIZE;
        // dynamically allocating nodes
        SC_ARR[sysID]->sl_head->next = malloc(MAX_SL_SIZE * sizeof(struct skipList_node));
        SC_ARR[sysID]->sl_tail->next = malloc(sizeof(struct skipList_node));
        SC_ARR[sysID]->sl_tail->next[0] = NULL;
        // assign the next node of head to tail since there's nothing in the sl yet
        int i = 0;
        for (i = 0; i < MAX_SL_SIZE; i++) {
            SC_ARR[sysID]->sl_head->next[i] = SC_ARR[sysID]->sl_tail;
        }

        SC_ARR[sysID]->sl_head->blockCount = 0;
        SC_ARR[sysID]->numProcesses = 0;
        SC_ARR[sysID]->init_state = true;
    }

    unsigned int currLevel = MAX_SL_SIZE - 1;
    struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
    struct skipList_node **nodes = malloc(MAX_SL_SIZE * sizeof(struct skipList_node *));
    // loop through levels to find target key
    int i = 0;
    for (i = MAX_SL_SIZE-1; i >= 0; i--) {
        // check if we aren't at the bottom yet
        if (currLevel > 0)
            currLevel--;
        // keep a history of everything as we go down
        nodes[i] = currNode;
        // loop to find anything to the right that isn't a tail
        while (currNode->next[currLevel] != SC_ARR[sysID]->sl_tail && currNode->next[currLevel]->process_id < id) {
            currNode = currNode->next[currLevel];
        }
    }

    // check if key already exists
    if (currNode->next[0]->process_id == id) {
        free(nodes);
        return EEXIST;
    }

    // figure out how high we need to go
    unsigned int newHeight = 1;
    bool ceil = false;

    // figure out how high we need to go
    while (newHeight < MAX_SL_SIZE && !ceil) {
        // make sure we are very random
        // seed_random(time(NULL));

        unsigned int ranNum = generate_random_int();
        if ((32767 - ranNum) > PROB) {
            newHeight++;
            //ranNum = generate_random_int();
        } else {
            ceil = true;
        }
    }

    // assign the pointers ahead and behind
    struct skipList_node *newNode = malloc(sizeof(struct skipList_node));
    newNode->process_id = id;
    newNode->towerHeight = newHeight;

    newNode->next = malloc(newHeight * sizeof(struct skipList_node));

    // do pointer surgery to rebuild associations
    for (i = 0; i <= newHeight-1; i++) {
        newNode->next[i] = nodes[i]->next[i];
        nodes[i]->next[i] = newNode;
    }
    // increment size if we need to
    if (newHeight-1 > SC_ARR[sysID]->sl_size) {
        SC_ARR[sysID]->sl_size = newHeight-1;
    }

    SC_ARR[sysID]->numProcesses++;
    // UNLOCK

    free(nodes);

    return 0;
}

static int skipList_print(unsigned long sysID) {
    // check if sysID is in range
    if (sysID < 0 || sysID > NUM_SYS_CALLS)
        return ENODEV;
    // check if the main system has been initialized
    if (!INIT_STATE)
        return ENODEV;
    // check if our specific sandbox has been initialized
    if (SC_ARR[sysID] == NULL)
        return ENODEV;
    printf("-------- Sandbox %lu -------- \n", sysID);
    printf("Currently Blocking %d processes \n", SC_ARR[sysID]->numProcesses);
    // loop through all the levels of the list so we can print out everything
    int i = 0;
    for (i = 0; i < MAX_SL_SIZE-1; i++) {
        printf("Level ");
        printf("%d", i);
        printf("      ");
        // set current pointer to head node
        struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
        while (currNode->next[i]->process_id > -1) {
            // move to next node and print it
            currNode = currNode->next[i];
            printf("%d ", currNode->process_id);

        }
        printf("\n");
    }
    return 0;
}

int skipList_destroy(unsigned long sysID, pid_t id) {
    // check if root
    /*uid_t uid = current_uid().val;
    uid_t euid = current_euid().val;
    if (uid > 0)
        return EPERM;*/
    // check if pid is good first
    if (id < 0)
        return ENOENT;
    // check if sysID is in range
    if (sysID < 0 || sysID > NUM_SYS_CALLS)
        return ENODEV;
    // check if the main system has been initialized
    if (!INIT_STATE)
        return ENODEV;
    // check if our specific sandbox has been initialized
    if (SC_ARR[sysID] == NULL)
        return ENODEV;

    else {
        // various vars to keep track of skipList parameters

        // LOCK

        unsigned int currLevel = SC_ARR[sysID]->sl_size;
        unsigned int targetHeight = 0;
        struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
        struct skipList_node **nodes = malloc(MAX_SL_SIZE * sizeof(struct skipList_node *) );
        // traverse through each level at a time
        int i = 0;
        for (i = SC_ARR[sysID]->sl_size; i >= 0; i--) {
            // check if we aren't at the bottom yet
            if (currLevel > 0) {
                currLevel--;
                targetHeight++;
            }

            // keep a history of everything as we go down
            nodes[i] = currNode;
            // loop to find anything to the right that isn't a tail
            while (currNode->next[currLevel]->process_id < id && currNode->next[currLevel] != SC_ARR[sysID]->sl_tail) {
                currNode = currNode->next[currLevel];
            }
        }
        // node to keep track of data to help us re-stitch the list later
        currNode = currNode->next[currLevel];
        if (currNode->process_id == id) {
            // restitch changed pointers
            for (i = 0; i < currNode->towerHeight; i++)
                nodes[i]->next[i] = currNode->next[i];
            // free dynamically allocated stuff
            //free(nodes);
            free(currNode->next);
            free(currNode);
            free(nodes);

            SC_ARR[sysID]->numProcesses--;

            return 0;
        }
            // return error if mailbox doesnt exist
        else {
            free(nodes);
            return ENOENT;
        }

        // UNLOCK

    }

}

int skipList_search(unsigned long sysID, pid_t id) {
    // check if pid is good first
    if (id < 0)
        return ENOENT;
    // check if the main system has been initialized
    if (!INIT_STATE)
        return 0;
    // check if our specific sandbox has been initialized
    if (SC_ARR[sysID] == NULL)
        return ENODEV;
    else {
        // various vars to keep track of skipList parameters

        // LOCK

        unsigned int currLevel = SC_ARR[sysID]->sl_size;
        unsigned int targetHeight = 0;
        struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
        struct skipList_node **nodes = malloc(SC_ARR[sysID]->sl_size * sizeof(struct skipList_node *) * 2);
        // traverse through each level at a time
        int i = 0;
        for (i = SC_ARR[sysID]->sl_size; i >= 0; i--) {
            // check if we aren't at the bottom yet
            if (currLevel > 0) {
                currLevel--;
                targetHeight++;
            }
            // keep a history of everything as we go down
            nodes[i] = currNode;
            // loop to find anything to the right that isn't a tail
            while (currNode->next[currLevel]->process_id < id && currNode->next[currLevel] != SC_ARR[sysID]->sl_tail) {
                currNode = currNode->next[currLevel];
            }
        }
        // node to keep track of data to help us re-stitch the list later
        currNode = currNode->next[currLevel];
        if (currNode->process_id == id) {
            return 0;
        }
            // return error if mailbox doesnt exist
        else {
            free(nodes);
            return ENOENT;
        }

        // UNLOCK

    }
}

// skipList data structure functions
unsigned long sbx421_block(pid_t proc, unsigned long nr) {
    // TODO: change uid and pid method for kernel use

    int uid = getuid();
    pid_t pid = getpid();

    uid = 0;

    if (proc > 0 && uid == 0) {
        // Do this if the process id is greater than 0 and the USER is root

        // if the perms checkout, add them to the respective sandbox
        return skipList_create(nr, proc);
    } else if (proc == 0) {
        // If the process is 0, only the calling process can block a call for itself
        return skipList_create(nr, pid);
    } else {
        // if the security conditions aren't met return and error
        return EACCES;
    }

}

unsigned long sbx421_unblock(pid_t proc, unsigned long nr) {
    // TODO: Change uid method for kernel

    int uid = getuid();

    uid = 0;

    if (uid == 0) {
        return skipList_destroy(nr, proc);
    } else {
        return EPERM;
    }
}

unsigned long sbx421_count(pid_t proc, unsigned long nr) {
    // TODO: Change the uid method for kernel
    int uid = getuid();
    // check if the proc is in range
    if (proc < 1) {
        return ENOENT;
    }
    // check if we are root
    else if (uid == 0) {
        // check if pid is good first
        if (proc < 0)
            return ENOENT;
        // check if the main system has been initialized
        if (!INIT_STATE)
            return ENODEV;
        // check if our specific sandbox has been initialized
        if (SC_ARR[nr]->init_state == false)
            return ENODEV;
        else {
            // various vars to keep track of skipList parameters

            // LOCK

            unsigned int currLevel = SC_ARR[nr]->sl_size;
            unsigned int targetHeight = 0;
            struct skipList_node *currNode = SC_ARR[nr]->sl_head;
            struct skipList_node **nodes = malloc(SC_ARR[nr]->sl_size * sizeof(struct skipList_node *) * 2);
            // traverse through each level at a time
            int i = 0;
            for (i = SC_ARR[nr]->sl_size; i >= 0; i--) {
                // check if we aren't at the bottom yet
                if (currLevel > 0) {
                    currLevel--;
                    targetHeight++;
                }
                // keep a history of everything as we go down
                nodes[i] = currNode;
                // loop to find anything to the right that isn't a tail
                while (currNode->next[currLevel]->process_id < proc && currNode->next[currLevel] != SC_ARR[nr]->sl_tail) {
                    currNode = currNode->next[currLevel];
                }
            }
            // node to keep track of data to help us re-stitch the list later
            currNode = currNode->next[currLevel];
            if (currNode->process_id == proc) {
                return currNode->blockCount;
            }
                // return error if mailbox doesnt exist
            else {
                free(nodes);
                return ENOENT;
            }

            // UNLOCK

        }
    }
    return 0;
}

/*
SYSCALL_DEFINE2(sbx421_block, pid_t, proc, unsigned long, nr) {
return 0;
}

SYSCALL_DEFINE2(sbx421_unblock, pid_t, proc, unsigned long, nr) {
return 0;
}

SYSCALL_DEFINE2(sbx421_count, pid_t, proc, unsigned long, nr) {
return 0;
}*/

#endif