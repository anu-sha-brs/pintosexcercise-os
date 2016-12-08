/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1



/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
	void getSlot(task_t task); /* task tries to use slot on the bus */
	void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
	void leaveSlot(task_t task); /* task release the slot */



/* initializes semaphores */ 
static struct semaphore slot;
static struct semaphore mutex;
static struct semaphore sender_priority_task;
static struct semaphore receiver_priority_task;
static struct semaphore sender_task;
static struct semaphore receiver_task;

static int current_direction = 0; 

void init_bus(void){ 
 
    random_init((unsigned int)123456789); 
    
    /*initialize task slots and mutex*/
    sema_init(&slot, BUS_CAPACITY);
    sema_init(&mutex,1);
    /*counting semaphores*/
    sema_init(&sender_priority_task,0);
    sema_init(&receiver_priority_task,0);
    sema_init(&sender_task,0);
    sema_init(&receiver_task,0);
    
}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
  /* local variables */
	unsigned int i;

	/* create sender priority task threads */
	for(i = 0; i < num_priority_send; i++)
		thread_create("task_priority_sender", 1, senderPriorityTask, NULL);
        
        /* create receiver priority task threads */
	for(i = 0; i < num_priority_receive; i++)
		thread_create("task_priority_receiver", 1, receiverPriorityTask, NULL);

        /* create sender task threads */
	for(i = 0; i < num_tasks_send; i++)
		thread_create("task_sender", 1, senderTask, NULL);

  
	/* create receiver task threads */
	for(i = 0; i < num_task_receive; i++)
		thread_create("task_receiver", 1, receiverTask, NULL);
}  


/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) 
{
    /* local variables */
	struct semaphore *task_list = NULL;

	/* save a pointer to our own waiting list */
	if(task.priority == HIGH) {
		if(task.direction == SENDER){
			task_list = &sender_priority_task;
                        
		} else {
			task_list = &receiver_priority_task;
                       
		}
	}
	
	else {
		if(task.direction == SENDER){
			task_list = &sender_task;
                        
		} else {
			task_list = &receiver_task;
                        

		}
	}

	sema_up(task_list);

	while(true) {

		/* enter critical section */
		sema_down(&mutex);
          
		/*
		 * if no one is on the bridge and we are a priority task or no priority task is waiting */

	if((slot.value == BUS_CAPACITY || current_direction == task.direction) && (slot.value > 0) && (task.priority == HIGH || (sender_priority_task.value +receiver_priority_task.value) == 0))
		{
                     
                        current_direction = task.direction;
		        /* decrement free task slots */
			sema_down(&slot);

			/* decrement our task waiting list */
			sema_down(task_list);

			/* leave critical section */
			sema_up(&mutex);

			break;
		}

		//to pick a new thread
		thread_yield();
	}
}


/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
 
}

/* task releases the slot */
void leaveSlot(task_t task) 
{
    
    sema_up(&slot);
    
}
