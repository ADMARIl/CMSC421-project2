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
#include <linux/init.h>
#include <linux/spinlock.h>

// TODO: Move these struct definitions to /include so we can do external variables

typedef struct syscall_entry {
    struct skipList_node *sl_head;
    struct skipList_node *sl_tail;
    int numProcesses;
    bool init_state;
    unsigned int sl_size;
    rwlock_t rwlock;
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

static int sbx_init(void) {
    printk("Begin sc arr initialization");
    if (INIT_STATE) {
        return 0;
    } else {
        // initialize the syscall array
        int i;
        for (i = 0; i < 437; i++) {
            SC_ARR[i] = NULL;
            // init lock
        }
        INIT_STATE = true;
        printk("Initialization complete");
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
    printk("begin lock checks");
    if (id < 0)
        return ENOENT;
    // check if sysID is in range
    if (sysID < 0 || sysID > NUM_SYS_CALLS)
        return ENODEV;
    // check if mailbox system has been initialized
    if (INIT_STATE == false)
        return ENODEV;
    // various vars to keep track of skipList parameters

    // write lock
    printk("1");

    printk("2");
    if (SC_ARR[sysID] == NULL) {
        // allocate space for the entry
        SC_ARR[sysID] = kmalloc(sizeof(syscall_entry), GFP_KERNEL);
        // init lock
        rwlock_init(&SC_ARR[sysID]->rwlock);

        write_lock(&SC_ARR[sysID]->rwlock);
        // create the head and tail skip list nodes
        SC_ARR[sysID]->sl_head = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
        SC_ARR[sysID]->sl_tail = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
        // set values to something we know is impossible
        SC_ARR[sysID]->sl_tail->process_id = -1;
        SC_ARR[sysID]->sl_head->process_id = -1;
        //SL_TAIL->process_id = 0;
        // make them the max size so we can assign pointers correctly later
        SC_ARR[sysID]->sl_tail->towerHeight = MAX_SL_SIZE;
        SC_ARR[sysID]->sl_head->towerHeight = MAX_SL_SIZE;
        // dynamically allocating nodes
        SC_ARR[sysID]->sl_head->next = kmalloc(MAX_SL_SIZE * sizeof(struct skipList_node), GFP_KERNEL);
        SC_ARR[sysID]->sl_tail->next = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
        SC_ARR[sysID]->sl_tail->next[0] = NULL;
        // assign the next node of head to tail since there's nothing in the sl yet
        int i = 0;
        for (i = 0; i < MAX_SL_SIZE; i++) {
            SC_ARR[sysID]->sl_head->next[i] = SC_ARR[sysID]->sl_tail;
        }

        SC_ARR[sysID]->sl_head->blockCount = 0;
        SC_ARR[sysID]->numProcesses = 0;
        SC_ARR[sysID]->init_state = true;
        SC_ARR[sysID]->sl_size = 0;

        write_unlock(&SC_ARR[sysID]->rwlock);
    }


    read_lock(&SC_ARR[sysID]->rwlock);
    unsigned int currLevel = MAX_SL_SIZE - 1;
    struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
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
        while (currNode->next[currLevel] != SC_ARR[sysID]->sl_tail && currNode->next[currLevel]->process_id < id) {
            currNode = currNode->next[currLevel];
        }
    }

    // check if key already exists
    if (currNode->next[0]->process_id == id) {
        kfree(nodes);
        // make sure we always unlock so we don't get stuck later
        read_unlock(&SC_ARR[sysID]->rwlock);
        return EEXIST;
    }
    read_unlock(&SC_ARR[sysID]->rwlock);
    // switch to writer lock
    write_lock(&SC_ARR[sysID]->rwlock);

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
    struct skipList_node *newNode = kmalloc(sizeof(struct skipList_node), GFP_KERNEL);
    newNode->process_id = id;
    newNode->towerHeight = newHeight;

    newNode->next = kmalloc(newHeight * sizeof(struct skipList_node), GFP_KERNEL);

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
    // WRITE UNLOCK
    write_unlock(&SC_ARR[sysID]->rwlock);

    kfree(nodes);

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

    // lock
    read_lock(&SC_ARR[sysID]->rwlock);
    printk("-------- Sandbox %lu -------- \n", sysID);
    printk("Currently Blocking %d processes \n", SC_ARR[sysID]->numProcesses);
    // loop through all the levels of the list so we can print out everything
    int i = 0;
    for (i = 0; i < MAX_SL_SIZE-1; i++) {
        printk("Level ");
        printk("%d", i);
        printk("      ");
        // set current pointer to head node
        struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
        while (currNode->next[i]->process_id > -1) {
            // move to next node and print it
            currNode = currNode->next[i];
            printk("%d ", currNode->process_id);

        }
        printk("\n");
    }
    read_unlock(&SC_ARR[sysID]->rwlock);
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
        printk("Made it past state check");
        // various vars to keep track of skipList parameters

        // LOCK
        read_lock(&SC_ARR[sysID]->rwlock);

        unsigned int currLevel = SC_ARR[sysID]->sl_size;
        unsigned int targetHeight = 0;
        struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
        //printk("1");
        struct skipList_node **nodes = kmalloc(MAX_SL_SIZE * sizeof(struct skipList_node *) , GFP_KERNEL);
        //printk("2");
        // traverse through each level at a time
        int i = 0;
        for (i = SC_ARR[sysID]->sl_size; i >= 0; i--) {
            //printk("B %d", i);
            // check if we aren't at the bottom yet
            if (currLevel > 0) {
                currLevel--;
                targetHeight++;
            }
            // keep a history of everything as we go down
            nodes[i] = currNode;
            // loop to find anything to the right that isn't a tail
            //printk("M %d", i);
            while (currNode->next[currLevel]->process_id < id && currNode->next[currLevel] != SC_ARR[sysID]->sl_tail) {
                currNode = currNode->next[currLevel];
            }
            //printk("E %d", i);
        }
        read_unlock(&SC_ARR[sysID]->rwlock);
        // switch from read to write
        write_lock(&SC_ARR[sysID]->rwlock);
        printk("Successfully traversed skipList");
        // node to keep track of data to help us re-stitch the list later
        currNode = currNode->next[currLevel];
        if (currNode->process_id == id) {
            // restitch changed pointers
            for (i = 0; i < currNode->towerHeight; i++)
                nodes[i]->next[i] = currNode->next[i];
            // kfree dynamically allocated stuff
            //free(nodes);
            kfree(currNode->next);
            kfree(currNode);
            kfree(nodes);

            printk("Freed things correctly");

            SC_ARR[sysID]->numProcesses--;
            write_unlock(&SC_ARR[sysID]->rwlock);
            return 0;
        }
            // return error if mailbox doesnt exist
        else {
            kfree(nodes);
            printk("Freed in print statement");
            write_unlock(&SC_ARR[sysID]->rwlock);
            return ENOENT;
        }
    }

}

int skipList_search(unsigned long sysID, pid_t id) {
    // check if pid is good first
    if (id < 0)
        return ENOENT;
    // check if the main system has been initialized
    if (!INIT_STATE)
        return ENODEV;
    // check if our specific sandbox has been initialized
    if (SC_ARR[sysID] == NULL)
        return ENODEV;
    else {
        // various vars to keep track of skipList parameters

        // LOCK
        read_lock(&SC_ARR[sysID]->rwlock);
        unsigned int currLevel = SC_ARR[sysID]->sl_size;
        unsigned int targetHeight = 0;
        struct skipList_node *currNode = SC_ARR[sysID]->sl_head;
        struct skipList_node **nodes = kmalloc(MAX_SL_SIZE * sizeof(struct skipList_node *), GFP_KERNEL);
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
            read_unlock(&SC_ARR[sysID]->rwlock);
            return 0;
        }
            // return error if mailbox doesnt exist
        else {
            kfree(nodes);
            read_unlock(&SC_ARR[sysID]->rwlock);
            return ENOENT;
        }

        // UNLOCK

    }
}

// TODO: Why is the return from void thing important?

// skipList data structure functions

SYSCALL_DEFINE2(sbx421_block, pid_t, proc, unsigned long, nr) {

    uid_t uid = current_uid().val;
    pid_t pid = current->pid;

    if (proc > 0 && uid == 0) {
        // Do this if the process id is greater than 0 and the USER is root

        // if the perms checkout, add them to the respective sandbox
        long returnVal = skipList_create(nr, proc);
        skipList_print(nr);
        return returnVal;
    } else if (proc == 0) {
        // If the process is 0, only the calling process can block a call for itself
         long returnVal = skipList_create(nr, pid);
         skipList_print(nr);
         return returnVal;
    } else {
        // if the security conditions aren't met return and error
        return EACCES;
    }
}

SYSCALL_DEFINE2(sbx421_unblock, pid_t, proc, unsigned long, nr) {

    uid_t uid = current_uid().val;

    if (uid == 0) {
        long returnVal = skipList_destroy(nr, proc);
        skipList_print(nr);
        return returnVal;
    } else {
        return EPERM;
    }
}

SYSCALL_DEFINE2(sbx421_count, pid_t, proc, unsigned long, nr) {

    uid_t uid = current_uid().val;
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
            read_lock(&SC_ARR[nr]->rwlock);
            unsigned int currLevel = SC_ARR[nr]->sl_size;
            unsigned int targetHeight = 0;
            struct skipList_node *currNode = SC_ARR[nr]->sl_head;
            struct skipList_node **nodes = kmalloc(MAX_SL_SIZE * sizeof(struct skipList_node *), GFP_KERNEL);
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
                read_unlock(&SC_ARR[nr]->rwlock);
                return currNode->blockCount;
            }
                // return error if sandbox doesnt exist
            else {
                kfree(nodes);
                read_unlock(&SC_ARR[nr]->rwlock);
                return ENOENT;
            }

            // UNLOCK

        }
    }
    return 0;
}