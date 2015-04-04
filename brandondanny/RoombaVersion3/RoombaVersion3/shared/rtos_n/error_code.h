/**
 * @file   error_code.h
 *
 * @brief Error codes used in triggering OS_Abort().
 *        Green errors are initialization errors
 *        Red errors are run-time errors
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 *
 * @author Scott Craig
 * @author Justin Tanner
 */
#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

enum {

/** GREEN ERRORS -- Initialize time errors. */
    
ERR_1_INIT_FAILURE,

/** RED ERRORS -- Run time errors. */

/** User called OS_Abort() */
ERR_RUN_0_USER_CALLED_OS_ABORT,

/** Too many tasks created. Only allowed MAXPROCESS at any time.*/
ERR_RUN_1_TOO_MANY_TASKS,

ERR_RUN_2_TOO_MANY_PERIODIC_TASKS,

ERR_RUN_3_PERIODIC_INVALID_CONFIGURATION,

/** PERIODIC task still running at end of time slot. */
ERR_RUN_4_PERIODIC_TOOK_TOO_LONG,

/** Two periodic tasks were scheduled for the same time! */
ERR_RUN_5_PERIODIC_TASKS_SCHEDULED_AST, 

/** ISR made a request that only tasks are allowed. */
ERR_RUN_6_ILLEGAL_ISR_KERNEL_REQUEST,

/** RTOS Internal error in handling request. */
ERR_RUN_7_RTOS_INTERNAL_ERROR,

/** Max services reached */
ERR_RUN_8_SERVICE_CAPACITY_REACHED,

/* The service requested does not exist */
ERR_RUN_9_INVALID_SERVICE,

/* A periodic task subscribed to a service, this is not legal */
ERR_RUN_10_PERIODIC_SUBSCRIBE,

/* Found a periodic task when publishing to a service */
ERR_RUN_11_PERIODIC_FOUND_WHEN_PUBLISHING,

/* Task found without a priority in the service */
ERR_RUN_12_TASK_WITHOUT_PRIORITY

};


#endif
