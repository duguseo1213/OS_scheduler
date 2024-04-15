/**********************************************************************
 * Copyright (c) 2019-2022
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;


/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks. Do not modify it
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}





#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

	/* Let's pick a new process to run next */
pick_next:
	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);
		
		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the process to run next */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};


/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	struct process *next = NULL;
	int min=10000;
	struct process *temp,*n,*temptemp;
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	if (!list_empty(&readyqueue)) 
	{
		//next = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry_safe(temp,n,&readyqueue,list)
		{
			if(temp->lifespan<min)
			{
				min=temp->lifespan;
				next=temp;
			}
		}
		
		list_del_init(&next->list);
		
	}
	
	/**
	 * Implement your own SJF scheduler here.
	 */
	return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule,		 /* TODO: Assign sjf_schedule()
								to this function pointer to activate
								SJF in the simulation system */
};


/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/
static struct process *srtf_schedule(void)
{
	struct process *next = NULL;
	struct process *temp,*n,*temptemp;
	
	int remain_min=-1;

	if (!list_empty(&readyqueue)) 
	{
		remain_min=10000;
		

		list_for_each_entry_safe(temp,n,&readyqueue,list)
		{
			if((temp->lifespan)-(temp->age)<remain_min)
			{
				remain_min=(temp->lifespan)-(temp->age);
				next=temp;
			}
		}
	}
	
	if (!current || current->status == PROCESS_WAIT) 
	{

		list_del_init(&next->list);
		return next;
	}
	
	if((current->lifespan)-(current->age)!=0 && (current->lifespan)-(current->age)<remain_min)
	{
		return current;
	}
	
	if (!list_empty(&readyqueue)) 
	{
		if((current->lifespan)-(current->age)!=0)
		{
			list_add(&current->list,&readyqueue);
		}
		list_del_init(&next->list);
		return next;
	}
	return NULL;
}
struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = srtf_schedule,
	/* You need to check the newly created processes to implement SRTF.
	 * You may use @forked() callback to mark newly created processes
	 */

	/* Obviously, you should implement srtf_schedule() and attach it here */
	
};


/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process *rr_schedule(void)
{
	struct process *next = NULL;

	if (!current || current->status == PROCESS_WAIT) 
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_del_init(&next->list);
		return next;
	}


	if(current->lifespan != current->age)
	{
		list_add_tail(&current->list,&readyqueue);
	}
	if(!list_empty(&readyqueue)) 
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_del_init(&next->list);
		return next;
	}
	return next;
}
struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule= rr_schedule,
	/* Obviously, you should implement rr_schedule() and attach it here */
};
bool prio_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}
	current->status = PROCESS_WAIT;
	struct process *temp,*n,*temptemp;
	int check=0;
	list_for_each_entry_safe_reverse(temp,n,&r->waitqueue,list)
	{
		
		if(temp->prio < current->prio)
		{
			temptemp=temp;
			check++;
		}
	}
	if(check==0) list_add_tail(&current->list, &r->waitqueue);
	else list_add_tail(&current->list, &temptemp->list);
	return false;
	
}
void prio_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;

	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		assert(waiter->status == PROCESS_WAIT);


		list_del_init(&waiter->list);

		waiter->status = PROCESS_READY;

		list_add_tail(&waiter->list, &readyqueue);
	}

}
static struct process *prio_schedule(void)
{
	struct process *next = NULL;
	struct process *temp,*n;
	unsigned int prio_max=0;

	//dump_status();
	

	if (!list_empty(&readyqueue)) 
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry_safe(temp,n,&readyqueue,list)
		{
			if(temp->prio > prio_max)
			{
				prio_max = temp->prio;
				next=temp;
			}
		}
	}

	

	if (!current || current->status == PROCESS_WAIT) 
	{
		
		list_del_init(&next->list);
		
		return next;
	}
	


	if((current->lifespan)-(current->age)!=0 && (current->prio > prio_max || (current->prio==0 && list_empty(&readyqueue))))
	{
		return current;
	}
	
	if (!list_empty(&readyqueue)) 
	{
		
		if((current->lifespan)-(current->age)!=0)
		{
			list_add_tail(&current->list,&readyqueue);
		}
		list_del_init(&next->list);
		return next;
	}
	return NULL;

}
/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
struct scheduler prio_scheduler = {
	.name = "Priority",
	/**
	 * Implement your own acqure/release function to make the priority
	 * scheduler correct.
	 */
	.acquire = prio_acquire,
	.release = prio_release,
	.schedule=prio_schedule,
	/* Implement your own prio_schedule() and attach it here */
};

static struct process *pa_schedule(void)
{
	struct process *next = NULL;
	struct process *temp,*n;
	unsigned int prio_max=0;

	if (!list_empty(&readyqueue)) 
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry_safe(temp,n,&readyqueue,list)
		{
			if(temp->prio > prio_max)
			{
				prio_max = temp->prio;
				next=temp;
			}
			if (temp->prio<=MAX_PRIO) temp->prio=temp->prio+1;

		}
	}
	
	if (!current || current->status == PROCESS_WAIT) 
	{
		
		list_del_init(&next->list);
		
		return next;
	}
	
	if((current->lifespan)-(current->age)!=0 && (current->prio > prio_max || (current->prio==0 && list_empty(&readyqueue))))
	{
		return current;
	}
	current->prio=current->prio+1;
	if (!list_empty(&readyqueue)) 
	{
		
		if((current->lifespan)-(current->age)!=0)
		{
			list_add_tail(&current->list,&readyqueue);
		}
		next->prio=next->prio_orig;
		list_del_init(&next->list);
		return next;
	}
	return NULL;
}
/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/
struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	.acquire = prio_acquire,
	.release = prio_release,
	.schedule=pa_schedule,
};
/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
bool pcp_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		current->prio=MAX_PRIO;
		r->owner = current;
		return true;
	}
	current->status = PROCESS_WAIT;
	struct process *temp,*n,*temptemp;
	int check=0;
	list_for_each_entry_safe_reverse(temp,n,&r->waitqueue,list)
	{
		
		if(temp->prio < current->prio)
		{
			temptemp=temp;
			check++;
		}
	}
	if(check==0) list_add_tail(&current->list, &r->waitqueue);
	else list_add_tail(&current->list, &temptemp->list);
	return false;
}
void pcp_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;
	current->prio=current->prio_orig;
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		assert(waiter->status == PROCESS_WAIT);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}
static struct process *pcp_schedule(void)
{
	struct process *next = NULL;
	struct process *temp,*n;
	unsigned int prio_max=0;
	if (!list_empty(&readyqueue)) 
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry_safe(temp,n,&readyqueue,list)
		{
			if(temp->prio > prio_max)
			{
				prio_max = temp->prio;
				next=temp;
			}
		}
	}
	if (!current || current->status == PROCESS_WAIT) 
	{
		
		list_del_init(&next->list);
		
		return next;
	}
	if((current->lifespan)-(current->age)!=0 && (current->prio > prio_max || (current->prio==0 && list_empty(&readyqueue))))
	{
		return current;
	}
	
	if (!list_empty(&readyqueue)) 
	{
		
		if((current->lifespan)-(current->age)!=0)
		{
			list_add_tail(&current->list,&readyqueue);
		}
		list_del_init(&next->list);
		return next;
	}
	return NULL;

}
struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.acquire = pcp_acquire,
	.release = pcp_release,
	.schedule=pcp_schedule,
};
/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
bool pip_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;
	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}
	if(r->owner->prio< current->prio) r->owner->prio=current->prio;
	current->status = PROCESS_WAIT;
	struct process *temp,*n,*temptemp;
	int check=0;
	list_for_each_entry_safe_reverse(temp,n,&r->waitqueue,list)
	{
		if(temp->prio < current->prio)
		{
			temptemp=temp;
			check++;
		}
	}
	if(check==0) list_add_tail(&current->list, &r->waitqueue);
	else list_add_tail(&current->list, &temptemp->list);
	return false;
}
void pip_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	assert(r->owner == current);
	r->owner = NULL;
	current->prio=current->prio_orig;
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		assert(waiter->status == PROCESS_WAIT);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}

}
static struct process *pip_schedule(void)
{
	struct process *next = NULL;
	struct process *temp,*n;
	unsigned int prio_max=0;
	if (!list_empty(&readyqueue)) 
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry_safe(temp,n,&readyqueue,list)
		{
			if(temp->prio > prio_max)
			{
				prio_max = temp->prio;
				next=temp;
			}
		}
	}
	if (!current || current->status == PROCESS_WAIT) 
	{
		list_del_init(&next->list);
		return next;
	}
	
	if((current->lifespan)-(current->age)!=0 && (current->prio > prio_max || (current->prio==0 && list_empty(&readyqueue))))
	{
		return current;
	}
	
	if (!list_empty(&readyqueue)) 
	{
		
		if((current->lifespan)-(current->age)!=0)
		{
			list_add_tail(&current->list,&readyqueue);
		}
		list_del_init(&next->list);
		return next;
	}
	return NULL;

}
struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	.acquire = pip_acquire,
	.release = pip_release,
	.schedule = pip_schedule,
};