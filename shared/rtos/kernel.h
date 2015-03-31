/**
 * @file   kernel.h
 *
 * @brief kernel data structures and constants used in os.cpp
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 * SPRING 2015
 *
 * @author Scott Craig
 * @author Justin Tanner
 */
#ifndef __KERNEL_H__
#define __KERNEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include "os.h"

#define WORKSPACE 256
/** Disable default prescaler to make processor speed 8 MHz. */

#define Disable_Interrupt()    asm volatile ("cli"::)
#define Enable_Interrupt()     asm volatile ("sei"::)

/** The RTOS timer's prescaler divisor */
#define TIMER_PRESCALER 8

/** The number of clock cycles in one "tick" or 5 ms */
#define TICK_CYCLES     (((F_CPU / TIMER_PRESCALER) / 1000) * TICK)

/** LEDs for OS_Abort() */
#define LED_RED_MASK    (uint8_t)(_BV(PB7))

/** LEDs for OS_Abort() */
#define LED_GREEN_MASK    (uint8_t)(_BV(PB7))


/* Typedefs and data structures. */

typedef void (*voidfuncvoid_ptr) (void);      /* pointer to void f(void) */

/**
 * @brief This is the set of states that a task can be in at any given time.
 */
typedef enum
{
    DEAD = 0,  /*The Task is Dead*/
    RUNNING,   /*Task is currently running as main task*/
    READY,     /*Task is ready to execute*/
    WAITING    /*Task is waiting on a synchronization construct */
}
task_state_t;

/**
 * @brief This is the set of kernel requests, i.e., a request code for each system call.
 */
typedef enum
{
    NONE = 0,
    TIMER_EXPIRED,

    TASK_CREATE,
    TASK_TERMINATE,
    TASK_NEXT,
    TASK_GET_ARG,

	SERVICE_INIT,
	SERVICE_SUB,
	SERVICE_PUB
}
kernel_request_t;

typedef enum
{
	SYSTEM = 0,
	PERIODIC = 1,
	ROUND_ROBIN = 2,
	IDLE = -1
}
task_priority_t;

/**
 * @brief The arguments required to create a task.
 */
typedef struct
{
    /** The code the new task is to run.*/
    voidfuncvoid_ptr f;
    /** A new task may be created with an argument that it can retrieve later. */
    int arg; //TODO: Convert to Void*
    /** Priority of the new task: ROUND_ROBIN, PERIODIC, SYSTEM */
    task_priority_t priority;
}
create_args_t;

typedef struct ptd_metadata_struct periodic_task_metadata_t;
struct ptd_metadata_struct
{
	struct td_struct* task;
	uint16_t period; //period in 5ms ticks.
	uint16_t wcet;   //worst case execution time in ticks.
	uint16_t next;   //Next/first time to fire.
	struct ptd_metadata_struct* nextT;
} ;


/**
 * @brief All the data needed to describe the task, including its context.
 */
typedef struct td_struct
{
    /** The stack used by the task. SP points in here when task is RUNNING. */
    uint8_t stack[WORKSPACE];
    /** A variable to save the hardware SP into when the task is suspended. */
    uint8_t* volatile sp;   /* stack pointer into the "workSpace" */

	task_priority_t priority;
	periodic_task_metadata_t* periodic_desc;

    /** The state of the task in this descriptor. */
    task_state_t state;
    /** The argument passed to Task_Create for this task. */
    int arg;

    /* A pointer to where a task expects their data to be published */
    int16_t * data;

	struct td_struct* next;
} task_descriptor_t;


/**
 * @brief Contains pointers to head and tail of a linked list.
 */
typedef struct
{
	task_descriptor_t* head;
	task_descriptor_t* tail;
}
task_queue_t;

typedef struct
{
	periodic_task_metadata_t* head;
	periodic_task_metadata_t* tail;
}
periodic_task_queue_t;

/**
 * The basic service struct. Contains a queue_t struct that
 * holds all tasks subscribed to it.
 */
struct service
{
	task_queue_t task_queue;
	task_queue_t data_queue;
};

#ifdef __cplusplus
}
#endif

#endif

