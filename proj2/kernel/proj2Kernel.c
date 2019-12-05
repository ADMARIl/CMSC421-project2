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
#include <linux/zconf.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/rwlock.h>
#include <linux/time.h>
#include <linux/cred.h>

struct mailBox_node {
    unsigned char* msg;
    struct mailBox_node* next;
};

struct mailbox {
    int numMessages;
    struct mailBox_node *head;
    // create a read write lock for the specific mailbox
    // rwlock_t mailLock = __RW_LOCK_UNLOCKED(mailLock);;
    ;
};

struct skipList_node {
    //struct list_head link;
    unsigned long id;
    unsigned int towerHeight;
    struct skipList_node** next;
    struct mailbox* mBox;
    pid_t * accessList;
    int numUsers;
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

static void seed_random(unsigned int seed) {
    next_random = seed;
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
    SL_TAIL = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
    SL_HEAD = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
    // set values to something we know is impossible
    SL_HEAD->id = 0;
    SL_TAIL->id = 0;
    // make them the max size so we can assign pointers correctly later
    SL_HEAD->towerHeight = MAX_SL_SIZE;
    SL_TAIL->towerHeight = MAX_SL_SIZE;
    // dynamically allocating nodes
    SL_HEAD->next = kmalloc(MAX_SL_SIZE * sizeof(struct skipList_node), GFP_KERNEL);
    SL_TAIL->next = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
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
        // kfree all the dynamically allocated stuff in the node
        kfree(currNode->next);
        kfree(currNode->accessList);
        // kfree mailbox memory
        struct mailBox_node *currMboxNode = currNode->mBox->head;
        while (currMboxNode != NULL) {
            struct mailBox_node *tempNode = currMboxNode->next;
            kfree(currMboxNode->msg);
            kfree(currMboxNode);
            currMboxNode = tempNode;

        }
        // kfree remaining memory
        kfree(currNode->mBox);
        kfree(currNode);

        // check to see if we are at the end yet
        if (SL_HEAD->next[0] != SL_TAIL) {
            currNode = currNode->next[0];
        }
    }
    // kfree our beginning nodes
    kfree(SL_TAIL->next);
    kfree(SL_TAIL);
    kfree(SL_HEAD->next);
    kfree(SL_HEAD);

    return 0;
}

int mbx421_create(unsigned long id) {
    // check if root
    uid_t uid = current_uid().val;
    uid_t euid = current_euid().val;
    if (uid > 0)
        return EPERM;
    // check if mailbox system has been initialized
    if (INIT_STATE == false)
        return ENODEV;
    // various vars to keep track of skipList parameters

    // lock

    unsigned int currLevel = MAX_SL_SIZE - 1;
    struct skipList_node *currNode = SL_HEAD;
    struct skipList_node **nodes = kmalloc(MAX_SL_SIZE * sizeof(struct skipList_node *), GFP_KERNEL);
    // loop through levels to find target key
    int i = 0;
    for (i = MAX_SL_SIZE-1; i >= 0; i--) {
        // check if we aren't at the bottom yet
        if (currLevel > 0)
            currLevel--;
        // keep a history of everything as we go down
        nodes[i] = currNode;
        // loop to find anything to the right that isn't a tail
        while (currNode->next[currLevel]->id < id && currNode->next[currLevel] != SL_TAIL) {
            currNode = currNode->next[currLevel];
        }
    }

    // check if key already exists
    if (currNode->next[0]->id == id) {
        kfree(nodes);
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
    struct skipList_node *newNode = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
    newNode->id = id;
    newNode->towerHeight = newHeight;
    // set up empty mailbox
    newNode->mBox = kmalloc(sizeof(struct mailbox), GFP_KERNEL);
    newNode->mBox->numMessages = 0;
    // initialize linked list
    newNode->mBox->head = kmalloc(sizeof(struct mailBox_node), GFP_KERNEL);
    newNode->mBox->head->next = NULL;
    newNode->mBox->head->msg = NULL;

    newNode->accessList = NULL;
    newNode->numUsers = 0;

    newNode->next = kmalloc(newHeight * sizeof(struct skipList_node), GFP_KERNEL);

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

    kfree(nodes);

    return 0;
}

int mbx421_destroy(unsigned long id) {
    // check if root
    uid_t uid = current_uid().val;
    uid_t euid = current_euid().val;
    if (uid > 0)
        return EPERM;
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
        struct skipList_node **nodes = kmalloc(SL_SIZE * sizeof(struct skipList_node *) * 2, GFP_KERNEL);
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
            while (currNode->next[currLevel]->id < id && currNode->next[currLevel] != SL_TAIL) {
                currNode = currNode->next[currLevel];
            }
        }
        // node to keep track of data to help us re-stitch the list later
        currNode = currNode->next[currLevel];
        if (currNode->id == id) {
            // restitch changed pointers
            for (i = 0; i < currNode->towerHeight; i++)
                nodes[i]->next[i] = currNode->next[i];
            // kfree dynamically allocated stuff
            //kfree(nodes);
            kfree(currNode->next);
            struct mailBox_node *currMboxNode = currNode->mBox->head;
            while (currMboxNode != NULL) {
                struct mailBox_node *tempNode = currMboxNode->next;
                kfree(currMboxNode->msg);
                kfree(currMboxNode);
                currMboxNode = tempNode;

            }
            kfree(currNode->accessList);
            kfree(currNode->mBox);
            kfree(currNode);
            kfree(nodes);
            return 0;
        }
        // return error if mailbox doesnt exist
        else {
            kfree(nodes);
            return ENOENT;
        }

        // UNLOCK

    }

}


// skipList data structure functions

SYSCALL_DEFINE2(sbx421_block, pid_t, proc, unsigned long, nr) {
    return 0;
}

SYSCALL_DEFINE2(sbx421_unblock, pid_t, proc, unsigned long, nr) {
    return 0;
}

SYSCALL_DEFINE2(sbx421_count, pid_t, proc, unsigned long, nr) {
    return 0;
}