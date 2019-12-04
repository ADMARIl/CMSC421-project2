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
//#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <zconf.h>
#include <stdbool.h>
//#include <rwlock.h>
//#include <cred.h>

/*
struct mailBox_node {
    unsigned char* msg;
    struct mailBox_node* next;
};

struct mailbox {
    int numMessages;
    struct mailBox_node *head;
    // create a read write lock for the specific mailbox
    // rwlock_t mailLock = __RW_LOCK_UNLOCKED(mailLock);;
};
*/

typedef struct syscall_entry {
    struct skipList_node *head;
    int numProcesses;
} syscall_entry;

struct skipList_node {
    //struct list_head link;
    unsigned long process_id;
    unsigned int towerHeight;
    struct skipList_node** next;
    // hypothetically I don't need these for this implementation
    /*struct mailbox* mBox;
    pid_t * accessList;*/
    // int numUsers;
    // create a read write lock for a node on the skip list
    // rwlock_t slNodeLock = __RW_LOCK_UNLOCKED(slNodeLock);;


};

unsigned int MAX_SL_SIZE = 0;
unsigned int PROB = 20000;
unsigned int SL_SIZE = 0;
bool INIT_STATE = false;
struct skipList_node *SL_HEAD;
struct skipList_node *SL_TAIL;

//
// provided random generation functions
//
static unsigned int next_random = 9001;

static unsigned int generate_random_int(void) {
    next_random = next_random * 1103515245 + 12345;
    return (next_random / 65536) % 32768;
}

int mbx421_init(unsigned int ptrs, unsigned int prob) {
    //seed_random((unsigned int)time(NULL));
    if (ptrs < 1 || prob < 1)
        return EINVAL;
    // set the globals to their new values
    MAX_SL_SIZE = ptrs+1;
    PROB = prob;

    //rwlock_init(&SL_LOCK, 0, NULL);
    // seed random

    // create the head and tail skip list nodes
    SL_TAIL = malloc(sizeof(struct skipList_node));
    SL_HEAD = malloc(sizeof(struct skipList_node));
    // set values to something we know is impossible
    SL_HEAD->process_id = 0;
    SL_TAIL->process_id = 0;
    // make them the max size so we can assign pointers correctly later
    SL_HEAD->towerHeight = MAX_SL_SIZE;
    SL_TAIL->towerHeight = MAX_SL_SIZE;
    // dynamically allocating nodes
    SL_HEAD->next = malloc(MAX_SL_SIZE * sizeof(struct skipList_node));
    SL_TAIL->next = malloc(sizeof(struct skipList_node));
    SL_TAIL->next[0] = NULL;

    // assign the next node of head to tail since there's nothing in the sl yet
    int i = 0;
    for (i = 0; i < ptrs; i++) {
        SL_HEAD->next[i] = SL_TAIL;
    }

    // set mailbox to null for now since we aren't using them yet
    SL_HEAD->mBox = NULL;
    SL_TAIL->mBox = NULL;

    // create empty access control list
    SL_HEAD->accessList = NULL;
    SL_TAIL->accessList = NULL;

    SL_HEAD->numUsers = 0;
    SL_TAIL->numUsers = 0;

    // initialize locks
    //SL_HEAD->slNodeLock = __RW_LOCK_UNLOCKED(slNodeLock);
    // tell the world that the skiplist is ready to roll
    INIT_STATE = true;
    return 0;
}

int mbx421_shutdown() {
    // check if mailbox system has been initialized
    if (INIT_STATE == false)
        return ENODEV;
    // since the node "towers" are only pointers to themselves, we can traverse along the
    // bottom to delete them all, so set our starting node to the first node on level 0
    struct skipList_node *currNode = SL_HEAD->next[0];

    // loop through level 0 until we hit the tail
    while (SL_HEAD->next[0] != SL_TAIL) {
        // move us forward so we don't lose our pointers
        SL_HEAD->next[0] = currNode->next[0];
        // free all the dynamically allocated stuff in the node
        free(currNode->next);
        free(currNode->accessList);
        // free mailbox memory
        struct mailBox_node *currMboxNode = currNode->mBox->head;
        while (currMboxNode != NULL) {
            struct mailBox_node *tempNode = currMboxNode->next;
            free(currMboxNode->msg);
            free(currMboxNode);
            currMboxNode = tempNode;

        }
        // free remaining memory
        free(currNode->mBox);
        free(currNode);

        // check to see if we are at the end yet
        if (SL_HEAD->next[0] != SL_TAIL) {
            currNode = currNode->next[0];
        }
    }
    // free our beginning nodes
    free(SL_TAIL->next);
    free(SL_TAIL);
    free(SL_HEAD->next);
    free(SL_HEAD);

    return 0;
}

int mbx421_create(unsigned long id) {
    // check if root
    /*uid_t uid = current_uid().val;
    uid_t euid = current_euid().val;
    if (uid > 0)
        return EPERM;*/
    // check if mailbox system has been initialized
    if (INIT_STATE == false)
        return ENODEV;
    // various vars to keep track of skipList parameters

    // lock

    unsigned int currLevel = MAX_SL_SIZE - 1;
    struct skipList_node *currNode = SL_HEAD;
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
        while (currNode->next[currLevel]->process_id < id && currNode->next[currLevel] != SL_TAIL) {
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
            ranNum = generate_random_int();
        } else {
            ceil = true;
        }
    }

    // assign the pointers ahead and behind
    struct skipList_node *newNode = malloc(sizeof(struct skipList_node));
    newNode->process_id = id;
    newNode->towerHeight = newHeight;
    // set up empty mailbox
    newNode->mBox = malloc(sizeof(struct mailbox));
    newNode->mBox->numMessages = 0;
    // initialize linked list
    newNode->mBox->head = malloc(sizeof(struct mailBox_node));
    newNode->mBox->head->next = NULL;
    newNode->mBox->head->msg = NULL;

    newNode->accessList = NULL;
    newNode->numUsers = 0;

    newNode->next = malloc(newHeight * sizeof(struct skipList_node));

    // do pointer surgery to rebuild associations
    for (i = 0; i <= newHeight-1; i++) {
        newNode->next[i] = nodes[i]->next[i];
        nodes[i]->next[i] = newNode;
    }
    // increment size if we need to
    if (newHeight-1 > SL_SIZE) {
        SL_SIZE = newHeight-1;
    }

    // UNLOCK

    free(nodes);

    return 0;
}

int mbx421_destroy(unsigned long id) {
    // check if root
    /*uid_t uid = current_uid().val;
    uid_t euid = current_euid().val;
    if (uid > 0)
        return EPERM;*/
    // check if mailbox system has been initialized
    if (INIT_STATE == false)
        return ENODEV;
    // check if exists first
    if (id < 0)
        return ENOENT;
    else {
        // various vars to keep track of skipList parameters

        // LOCK

        unsigned int currLevel = SL_SIZE;
        unsigned int targetHeight = 0;
        struct skipList_node *currNode = SL_HEAD;
        struct skipList_node **nodes = malloc(SL_SIZE * sizeof(struct skipList_node *) * 2);
        // traverse through each level at a time
        int i = 0;
        for (i = SL_SIZE; i >= 0; i--) {
            // check if we aren't at the bottom yet
            if (currLevel > 0) {
                currLevel--;
                targetHeight++;
            }
            // keep a history of everything as we go down
            nodes[i] = currNode;
            // loop to find anything to the right that isn't a tail
            while (currNode->next[currLevel]->process_id < id && currNode->next[currLevel] != SL_TAIL) {
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
            struct mailBox_node *currMboxNode = currNode->mBox->head;
            while (currMboxNode != NULL) {
                struct mailBox_node *tempNode = currMboxNode->next;
                free(currMboxNode->msg);
                free(currMboxNode);
                currMboxNode = tempNode;

            }
            free(currNode->accessList);
            free(currNode->mBox);
            free(currNode);
            free(nodes);
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

    return 0;
}

unsigned long sbx421_unblock(pid_t proc, unsigned long nr) {

    return 0;
}

unsigned long sbx421_count(pid_t proc, unsigned long nr) {

    return 0;
}

/*
SYSCALL_DEFINE2(sbx421_block, pid_t, proc, unsigned long, nr) {
return 0;
}

SYCALL_DEFINE2(sbx421_unblock, pid_t, proc, unsigned long, nr) {
return 0;
}

SYSCALL_DEFINE2(sbx421_count, pid_t, proc, unsigned long, nr) {
return 0;
}*/

#endif