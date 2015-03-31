/**
 * @file os.c
 *
 * @brief A Real Time Operating System
 *
 * Our implementation of the operating system described by Mantis Cheng in os.h.
 *
 * @author Scott Craig
 * @author Justin Tanner
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "kernel.h"
#include "error_code.h"


#define CYCLES_PER_MS (TICK_CYCLES / TICK)
#define HALF_MS (TICK_CYCLES / (TICK << 1))

/* Needed for memset */
/* #include <string.h> */

/** @brief main function provided by user application. The first task to run. */
extern int r_main();

/** The task descriptor of the currently RUNNING task. */
static task_descriptor_t* cur_task = NULL;
static periodic_task_metadata_t* cur_per_metadata = NULL;
static uint16_t periodic_task_remticks = 0;

/** Since this is a "full-served" model, the kernel is executing using its own stack.
 * this variable is used to store the kernels stack pointer */
static volatile uint16_t kernel_sp;

/** This table contains all task descriptors, regardless of state, plus idler (+1). */
static task_descriptor_t task_desc[MAXPROCESS + 1];

static periodic_task_metadata_t periodic_task_desc[MAXPERIODICPRO];

/** The special "idle task" at the end of the descriptors array. */
static task_descriptor_t* idle_task = &task_desc[MAXPROCESS];

/** The current kernel request. */
static volatile kernel_request_t kernel_request;

/** Arguments for Task_Create() request. */
static volatile create_args_t kernel_request_create_args;

static volatile periodic_task_metadata_t kernel_period_create_meta;

static volatile int kernel_preemption_disabled = 0;

/** Return value for Task_Create() request. */
static volatile int kernel_request_retval;

/** Return value for Service_Init() request. */
static volatile SERVICE * kernel_request_service_init_retval;

/** Used to hold a pointer to the service that we want to subscribe or publish to */
static volatile SERVICE * kernel_request_service_descriptor;

/** Holds a reference to the location that data will be written to for the service */
static volatile int16_t * kernel_request_service_sub_data;

/** Holds the value to be published to a service */
static volatile int16_t kernel_request_service_pub_data;

/** Number of tasks created so far */
static task_queue_t dead_pool_queue;

static periodic_task_queue_t periodic_dead_pool_queue;

/** The ready queue for RR tasks. Their scheduling is round-robin. */
static task_queue_t roundrobin_task_queue;

/** The queue of periodic tasks which are ordered by next execution time */
static periodic_task_queue_t periodic_task_queue;

/** The ready queue for SYSTEM tasks. Their scheduling is first come, first served. */
static task_queue_t system_task_queue;

static SERVICE service_list[MAXSERVICES];
static uint8_t num_services = 0;

/** Error message used in OS_Abort() */
static uint8_t volatile error_msg = ERR_RUN_0_USER_CALLED_OS_ABORT;

/** Ticks since we first started the OS */
static uint16_t volatile ticks_from_start = 0;
static uint16_t volatile current_timer_val = 0;

/* Forward declarations */
/* kernel */
static void kernel_main_loop(void);
static void kernel_dispatch(void);
static void kernel_handle_request(void);
/* context switching */
static void exit_kernel(void) __attribute((noinline, naked));
static void enter_kernel(void) __attribute((noinline, naked));

#ifdef __cplusplus
extern "C" {
#endif
void TIMER1_COMPA_vect(void) __attribute__ ((signal, naked));
#ifdef __cplusplus
}
#endif

static int kernel_create_task();
static void kernel_terminate_task(void);

static void kernel_service_init();
static void kernel_service_sub();
static void kernel_service_pub();

/* queues */
static void periodic_enqueue(periodic_task_queue_t* queue_ptr, periodic_task_metadata_t* to_add);
static periodic_task_metadata_t* periodic_dequeue(periodic_task_queue_t* queue_ptr);

static void enqueue(task_queue_t* queue_ptr, task_descriptor_t* to_add);
static void budgequeue(task_queue_t* queue_ptr, task_descriptor_t* to_add);
static task_descriptor_t* dequeue(task_queue_t* queue_ptr);

static void kernel_update_ticker(void);
static void idle (void);
static void _delay_25ms(void);

/*
 * FUNCTIONS
 */
/**
 *  @brief The idle task does nothing but busy loop.
 */
static void idle (void)
{
    for(;;)
    {};
}


/**
 * @fn kernel_main_loop
 *
 * @brief The heart of the RTOS, the main loop where the kernel is entered and exited.
 *
 * The complete function is:
 *
 *  Loop
 *<ol><li>Select and dispatch a process to run</li>
 *<li>Exit the kernel (The loop is left and re-entered here.)</li>
 *<li>Handle the request from the process that was running.</li>
 *<li>End loop, go to 1.</li>
 *</ol>
 */
static void kernel_main_loop(void)
{
    for(;;)
    {
        kernel_dispatch();
		kernel_preemption_disabled = 0; //Disable it after each call to OS
        exit_kernel();

        /* if this task makes a system call, or is interrupted,
         * the thread of control will return to here. */

        kernel_handle_request();
    }
}

int periodic_task_ready()
{
	if(periodic_task_queue.head != NULL
		&& periodic_task_queue.head != periodic_task_queue.tail
		&& periodic_task_queue.head->next ==
			periodic_task_queue.head->nextT->next)
	{
		//There are two tasks scheduled to run at the same time!
		error_msg = ERR_RUN_5_PERIODIC_TASKS_SCHEDULED_AST;
		OS_Abort();
	}

	return periodic_task_queue.head != NULL &&
		periodic_task_queue.head->next == ticks_from_start;
}

/**
 * @fn kernel_dispatch
 *
 *@brief The second part of the scheduler.
 *
 * Chooses the next task to run.
 *
 */
static void kernel_dispatch(void)
{
    /* If the current state is RUNNING, then select it to run again.
     * kernel_handle_request() has already determined it should be selected.
     */
    if (cur_task == NULL)
    {
		error_msg = ERR_RUN_7_RTOS_INTERNAL_ERROR;
		OS_Abort();
        return;
    }
    if(cur_task->state != RUNNING || (cur_task == idle_task && !kernel_preemption_disabled))
    {
		if(system_task_queue.head != NULL)
        {
            cur_task = (task_descriptor_t*)dequeue(&system_task_queue);
        }
		//Else if a period tasks is ready...
        else if(periodic_task_ready())
        {
            /* Keep running the current PERIODIC task. */
			cur_per_metadata = periodic_dequeue(&periodic_task_queue);
            cur_task = cur_per_metadata->task;
			periodic_task_remticks = cur_per_metadata->wcet;
        }
		//Else if, use the time to complete round robin.
        else if(roundrobin_task_queue.head != NULL)
        {
            cur_task = (task_descriptor_t*)dequeue(&roundrobin_task_queue);
        }
        else
        {
            /* No task available, so idle. */
            cur_task = idle_task;
        }
        cur_task->state = RUNNING;
    }
}

/** Returns non-zero if the current task should be preempted */
static int kernel_should_preempt()
{
	if(kernel_preemption_disabled == 1)
	{
		return 0;
	}
	if ( system_task_queue.head != NULL && cur_task->priority != SYSTEM )
	{
		return 1;
	}
	if (periodic_task_ready() && cur_task->priority > PERIODIC)
	{
		//If a periodic task if ready and we are a lower priority (greater value)
		//we want to preempt.
		return 1;
	}
	if (roundrobin_task_queue.head != NULL && cur_task->priority > ROUND_ROBIN)
	{
		//If we are idle task, and there is in round robin, relingquish.
		return 1;
	}
	return 0;

}

/**
 * @fn kernel_handle_request
 *
 *@brief The first part of the scheduler.
 *
 * Perform some action based on the system call or timer tick.
 * Perhaps place the current process in a ready or waiting queue.
 */
static void kernel_handle_request(void)
{
   switch(kernel_request)
    {
    case NONE:
        /* Should not happen. */
        break;

    case TIMER_EXPIRED:
        kernel_update_ticker();

        /* Round robin tasks get pre-empted on every tick. */
        if(cur_task->priority == ROUND_ROBIN && cur_task->state == RUNNING)
        {
            cur_task->state = READY;
            enqueue(&roundrobin_task_queue, cur_task);
        }
        break;

    case TASK_CREATE:
        kernel_request_retval = kernel_create_task();

        /* Check if new task has higher priority, and that it wasn't an ISR
         * making the request.
         */

        if(!kernel_request_retval && kernel_should_preempt())
        {
			cur_task->state = READY;
			//If we have been paused and are round robin, enqueue at the front
			//to run next time.
            if(cur_task->priority == ROUND_ROBIN)
            {
                budgequeue(&roundrobin_task_queue, cur_task);
            }
			else if(cur_task->priority == PERIODIC)
			{
				//If we are a periodic which as been pre-empted, place us back in the
				//waiting queue without updating our next stamp, so we remain at the front.
				periodic_enqueue(&periodic_task_queue, cur_task->periodic_desc);
			}
        }
		else if(kernel_request_retval == 1)
		{
			//Too many tasks.
			error_msg = ERR_RUN_1_TOO_MANY_TASKS;
			OS_Abort();

		}
		else if(kernel_request_retval == 2)
		{
			//Too many periodic tasks.
			error_msg = ERR_RUN_2_TOO_MANY_PERIODIC_TASKS;
			OS_Abort();
		}

        break;

    case TASK_TERMINATE:
		if(cur_task != idle_task)
		{
        	kernel_terminate_task();
		}
        break;

    case TASK_NEXT:
		cur_task->state = READY;
		switch(cur_task->priority)
		{
			case SYSTEM:
				enqueue(&system_task_queue, cur_task);
				break;

			case PERIODIC:
				//If we did execute in time
				cur_per_metadata->next += cur_per_metadata->period;
				periodic_enqueue(&periodic_task_queue, cur_per_metadata);
				break;

			case ROUND_ROBIN:
				//If the task calls Task_Next() it is yielding, and is moved to the back.
				enqueue(&roundrobin_task_queue, cur_task);
				break;

			default: /* idle_task */
				break;
		}
        break;

    case TASK_GET_ARG:
        /* Should not happen. Handled in task itself. */
        break;

    case SERVICE_INIT:
        kernel_service_init();
        break;

    case SERVICE_SUB:
        kernel_service_sub();
        break;

    case SERVICE_PUB:
        kernel_service_pub();
		if(kernel_should_preempt())
		{
			cur_task->state = READY;
			switch(cur_task->priority)
			{
				case SYSTEM:
				enqueue(&system_task_queue, cur_task);
				break;

				case PERIODIC:
				//Assume we did not finish out run.
				periodic_enqueue(&periodic_task_queue, cur_per_metadata);
				break;

				case ROUND_ROBIN:
				//If the task calls Task_Next() it is yielding, and is moved to the back.
				budgequeue(&roundrobin_task_queue, cur_task);
				break;

				default: /* idle_task */
				break;
			}
		}
        break;

    default:
        /* Should never happen */
        error_msg = 2; // TODO: FIXME //ERR_RUN_8_RTOS_INTERNAL_ERROR;
        OS_Abort();
        break;
    }

    kernel_request = NONE;
}


/*
 * Context switching
 */
/**
 * It is important to keep the order of context saving and restoring exactly
 * in reverse. Also, when a new task is created, it is important to
 * initialize its "initial" context in the same order as a saved context.
 *
 * Save r31 and SREG on stack, disable interrupts, then save
 * the rest of the registers on the stack. In the locations this macro
 * is used, the interrupts need to be disabled, or they already are disabled.
 */
#define    SAVE_CTX_TOP()       asm volatile (\
    "push   r31             \n\t"\
    "in     r31,	0X3C	\n\t"\
    "push   r31             \n\t"\
    "in     r31,	__SREG__    \n\t"\
    "cli                    \n\t"::); /* Disable interrupt */

#define STACK_SREG_SET_I_BIT()    asm volatile (\
    "ori    r31, 0x80        \n\t"::);

#define    SAVE_CTX_BOTTOM()       asm volatile (\
    "push   r31             \n\t"\
    "push   r30             \n\t"\
    "push   r29             \n\t"\
    "push   r28             \n\t"\
    "push   r27             \n\t"\
    "push   r26             \n\t"\
    "push   r25             \n\t"\
    "push   r24             \n\t"\
    "push   r23             \n\t"\
    "push   r22             \n\t"\
    "push   r21             \n\t"\
    "push   r20             \n\t"\
    "push   r19             \n\t"\
    "push   r18             \n\t"\
    "push   r17             \n\t"\
    "push   r16             \n\t"\
    "push   r15             \n\t"\
    "push   r14             \n\t"\
    "push   r13             \n\t"\
    "push   r12             \n\t"\
    "push   r11             \n\t"\
    "push   r10             \n\t"\
    "push   r9              \n\t"\
    "push   r8              \n\t"\
    "push   r7              \n\t"\
    "push   r6              \n\t"\
    "push   r5              \n\t"\
    "push   r4              \n\t"\
    "push   r3              \n\t"\
    "push   r2              \n\t"\
    "push   r1              \n\t"\
    "push   r0              \n\t"::);

/**
 * @brief Push all the registers and SREG onto the stack.
 */
#define    SAVE_CTX()    SAVE_CTX_TOP();SAVE_CTX_BOTTOM();

/**
 * @brief Pop all registers and the status register.
 */
#define    RESTORE_CTX()    asm volatile (\
    "pop    r0              \n\t"\
    "pop    r1              \n\t"\
    "pop    r2              \n\t"\
    "pop    r3              \n\t"\
    "pop    r4              \n\t"\
    "pop    r5              \n\t"\
    "pop    r6              \n\t"\
    "pop    r7              \n\t"\
    "pop    r8              \n\t"\
    "pop    r9              \n\t"\
    "pop    r10             \n\t"\
    "pop    r11             \n\t"\
    "pop    r12             \n\t"\
    "pop    r13             \n\t"\
    "pop    r14             \n\t"\
    "pop    r15             \n\t"\
    "pop    r16             \n\t"\
    "pop    r17             \n\t"\
    "pop    r18             \n\t"\
    "pop    r19             \n\t"\
    "pop    r20             \n\t"\
    "pop    r21             \n\t"\
    "pop    r22             \n\t"\
    "pop    r23             \n\t"\
    "pop    r24             \n\t"\
    "pop    r25             \n\t"\
    "pop    r26             \n\t"\
    "pop    r27             \n\t"\
    "pop    r28             \n\t"\
    "pop    r29             \n\t"\
    "pop    r30             \n\t"\
    "pop    r31             \n\t"\
	"out    __SREG__, r31   \n\t"\
	"pop	r31				\n\t"\
	"out	0X3C,	r31		\n\t"\
    "pop    r31             \n\t"::);


/**
 * @fn exit_kernel
 *
 * @brief The actual context switching code begins here.
 *
 * This function is called by the kernel. Upon entry, we are using
 * the kernel stack, on top of which is the address of the instruction
 * after the call to exit_kernel().
 *
 * Assumption: Our kernel is executed with interrupts already disabled.
 *
 * The "naked" attribute prevents the compiler from adding instructions
 * to save and restore register values. It also prevents an
 * automatic return instruction.
 */
static void exit_kernel(void)
{
    /*
     * The PC was pushed on the stack with the call to this function.
     * Now push on the I/O registers and the SREG as well.
     */
     SAVE_CTX();

    /*
     * The last piece of the context is the SP. Save it to a variable.
     */
    kernel_sp = SP;

    /*
     * Now restore the task's context, SP first.
     */
    SP = (uint16_t)(cur_task->sp);

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

    /*
     * return explicitly required as we are "naked".
     * Interrupts are enabled or disabled according to SREG
     * recovered from stack, so we don't want to explicitly
     * enable them here.
     *
     * The last piece of the context, the PC, is popped off the stack
     * with the ret instruction.
     */
    asm volatile ("ret\n"::);
}


/**
 * @fn enter_kernel
 *
 * @brief All system calls eventually enter here.
 *
 * Assumption: We are still executing on cur_task's stack.
 * The return address of the caller of enter_kernel() is on the
 * top of the stack.
 */
static void enter_kernel(void)
{
    /*
     * The PC was pushed on the stack with the call to this function.
     * Now push on the I/O registers and the SREG as well.
     */
    SAVE_CTX();

    /*
     * The last piece of the context is the SP. Save it to a variable.
     */
    cur_task->sp = (uint8_t*)SP;

    /*
     * Now restore the kernel's context, SP first.
     */
    SP = kernel_sp;

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

    /*
     * return explicitly required as we are "naked".
     *
     * The last piece of the context, the PC, is popped off the stack
     * with the ret instruction.
     */
    asm volatile ("ret\n"::);
}


/**
 * @fn TIMER1_COMPA_vect
 *
 * @brief The interrupt handler for output compare interrupts on Timer 1
 *
 * Used to enter the kernel when a tick expires.
 *
 * Assumption: We are still executing on the cur_task stack.
 * The return address inside the current task code is on the top of the stack.
 *
 * The "naked" attribute prevents the compiler from adding instructions
 * to save and restore register values. It also prevents an
 * automatic return instruction.
 */
void TIMER1_COMPA_vect(void)
{
	//PORTB ^= _BV(PB7);		// Arduino LED
    /*
     * Save the interrupted task's context on its stack,
     * and save the stack pointer.
     *
     * On the cur_task's stack, the registers and SREG are
     * saved in the right order, but we have to modify the stored value
     * of SREG. We know it should have interrupts enabled because this
     * ISR was able to execute, but it has interrupts disabled because
     * it was stored while this ISR was executing. So we set the bit (I = bit 7)
     * in the stored value.
     */
    SAVE_CTX_TOP();

    STACK_SREG_SET_I_BIT();

    SAVE_CTX_BOTTOM();

    cur_task->sp = (uint8_t*)SP;

    /*
     * Now that we already saved a copy of the stack pointer
     * for every context including the kernel, we can move to
     * the kernel stack and use it. We will restore it again later.
     */
    SP = kernel_sp;

    /*
     * Inform the kernel that this task was interrupted.
     */
    kernel_request = TIMER_EXPIRED;

    /*
     * Prepare for next tick interrupt.
     */
    OCR1A += TICK_CYCLES;

    /*
     * Restore the kernel context. (The stack pointer is restored again.)
     */
    SP = kernel_sp;

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

    /*
     * We use "ret" here, not "reti", because we do not want to
     * enable interrupts inside the kernel.
     * Explilictly required as we are "naked".
     *
     * The last piece of the context, the PC, is popped off the stack
     * with the ret instruction.
     */
    asm volatile ("ret\n"::);
}


/*
 * Tasks Functions
 */
/**
 *  @brief Kernel function to create a new task.
 *
 * When creating a new task, it is important to initialize its stack just like
 * it has called "enter_kernel()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 * @sa enter_kernel
 * @returns	0 if successful.
			1 if not processes available
			2 if no periodic processes are available.
 */
static int kernel_create_task()
{
    /* The new task. */
    task_descriptor_t *p;
	periodic_task_metadata_t *pt;
    uint8_t* stack_bottom;

    if (dead_pool_queue.head == NULL)
    {
        /* Too many tasks! */
        return 1;
    }

	if(kernel_request_create_args.priority == PERIODIC
		&& periodic_dead_pool_queue.head == NULL)
	{
		return 2; //Too many periodic tasks.
	}

	if(kernel_request_create_args.priority == IDLE)
	{
		p = idle_task;
	}
	else
	{
		//Find an unused descriptor.
	    p = dequeue(&dead_pool_queue);
	}

    stack_bottom = &(p->stack[WORKSPACE-1]);

    /* The stack grows down in memory, so the stack pointer is going to end up
     * pointing to the location 32 + 1 + 2 + 2 = 37 bytes above the bottom, to make
     * room for (from bottom to top):
     *   the address of Task_Terminate() to destroy the task if it ever returns,
     *   the address of the start of the task to "return" to the first time it runs,
     *   register 31,
     *   the stored SREG, and
     *   registers 30 to 0.
     */
    uint8_t* stack_top = stack_bottom - (32 + 1 + 2 + 2 + 3);

    /* Not necessary to clear the task descriptor. */
    /* memset(p,0,sizeof(task_descriptor_t)); */

    /* stack_top[0] is the byte above the stack.
     * stack_top[1] is r0. */
    stack_top[2] = (uint8_t) 0; /* r1 is the "zero" register. */
    /* stack_top[31] is r30. */
    stack_top[32] = (uint8_t) _BV(SREG_I); /* set SREG_I bit in stored SREG. */
    /* stack_top[33] is r31. */

    /* We are placing the address (16-bit) of the functions
     * onto the stack in reverse byte order (least significant first, followed
     * by most significant).  This is because the "return" assembly instructions
     * (ret and reti) pop addresses off in BIG ENDIAN (most sig. first, least sig.
     * second), even though the AT90 is LITTLE ENDIAN machine.
     */
	stack_top[35] = (uint8_t) 0;
    stack_top[36] = (uint8_t)((uint16_t)(kernel_request_create_args.f) >> 8);
    stack_top[37] = (uint8_t)(uint16_t)(kernel_request_create_args.f);
	stack_top[38] = (uint8_t) 0;
    stack_top[39] = (uint8_t)((uint16_t)Task_Terminate >> 8);
    stack_top[40] = (uint8_t)(uint16_t)Task_Terminate;

    /*
     * Make stack pointer point to cell above stack (the top).
     * Make room for 32 registers, SREG and two return addresses.
     */
    p->sp = stack_top;

    p->state = READY;
    p->arg = kernel_request_create_args.arg;
    p->priority = kernel_request_create_args.priority;

	switch(kernel_request_create_args.priority)
	{
		case PERIODIC:
			/* Enqueue the new task based on its */
			pt = periodic_dequeue(&periodic_dead_pool_queue);
			pt->next = (uint16_t)kernel_period_create_meta.next;
			pt->period = (uint16_t)kernel_period_create_meta.period;
			pt->wcet = (uint16_t)kernel_period_create_meta.wcet;
			pt->task = p;
			p->periodic_desc = pt;
			periodic_enqueue(&periodic_task_queue, pt);
			break;

		case SYSTEM:
    		/* Put SYSTEM and Round Robin tasks on a queue. */
			enqueue(&system_task_queue, p);
			break;

		case ROUND_ROBIN:
			/* Put SYSTEM and Round Robin tasks on a queue. */
			enqueue(&roundrobin_task_queue, p);
			break;

		default:
			/* idle task does not go in a queue */
			break;
	}
    return 0;
}


/**
 * @brief Kernel function to destroy the current task.
 */
static void kernel_terminate_task(void)
{
    /* deallocate all resources used by this task */
    cur_task->state = DEAD;
    if(cur_task->priority == PERIODIC && cur_task->periodic_desc != NULL)
    {
		//TODO: Remove from our construct.
		cur_task->periodic_desc->task = NULL;
		periodic_enqueue(&periodic_dead_pool_queue, cur_task->periodic_desc);
    }
    enqueue(&dead_pool_queue, cur_task);
}

/**
 * Initialize a service pointer, and set it to the
 * kernel_request_service_init_retval pointer. Set the
 * pointer to 0 to imply a failure
 */
static void kernel_service_init()
{
	if (num_services < MAXSERVICES)
	{
		kernel_request_service_init_retval = &(service_list[num_services]);
		num_services += 1;
	}
	else
	{
		error_msg = ERR_RUN_8_SERVICE_CAPACITY_REACHED;
		OS_Abort();
	}
}

/**
 * Subscribe a task to a given service
 */
static void kernel_service_sub()
{
	if (kernel_request_service_descriptor == NULL)
	{
		error_msg = ERR_RUN_9_INVALID_SERVICE;
        OS_Abort();
	}
    else if (cur_task->priority == PERIODIC)
    {
        error_msg = ERR_RUN_10_PERIODIC_SUBSCRIBE;
        OS_Abort();
    }
    else
    {
        SERVICE * s = (SERVICE *) kernel_request_service_descriptor;
        cur_task->data = (int16_t *) kernel_request_service_sub_data;
	    enqueue(&(s->task_queue), cur_task);

        // Block the task until someone publishes to the service
        cur_task->state = WAITING;
    }
}

/**
 * Publish a value to a service
 */
static void kernel_service_pub()
{
	if (kernel_request_service_descriptor == NULL)
	{
    	error_msg = ERR_RUN_9_INVALID_SERVICE;
    	OS_Abort();
	}
    else
    {
        SERVICE * s = (SERVICE *) kernel_request_service_descriptor;

        // Release the tasks! TODO: Place them in the expected ready queues
        task_descriptor_t * t = NULL;
        while (s->task_queue.head != NULL)
        {
            t = (task_descriptor_t *) dequeue(&(s->task_queue));
			*(t->data) = (int16_t) kernel_request_service_pub_data;
            t->state = READY;
            switch(t->priority)
            {
                case SYSTEM:
                    enqueue(&system_task_queue, t);
                    break;
                case PERIODIC:
                    // This should have never happened. Error out
                    error_msg = ERR_RUN_11_PERIODIC_FOUND_WHEN_PUBLISHING;
                    OS_Abort();
                    break;
                case ROUND_ROBIN:
                    enqueue(&roundrobin_task_queue, t);
                    break;
                default:
                    error_msg = ERR_RUN_12_TASK_WITHOUT_PRIORITY;
                    OS_Abort();
                    break;
            }
        }
    }
}

/*
 * Queue manipulation.
 */

/**	Enqueue the period task for triggering again. */
static void periodic_enqueue(periodic_task_queue_t* queue_ptr, periodic_task_metadata_t* to_add)
{
	periodic_task_metadata_t* r = NULL;
	periodic_task_metadata_t* q = NULL;

	if(queue_ptr->head == NULL)
	{
		queue_ptr->head = to_add;
		queue_ptr->tail = to_add;
		to_add->nextT = NULL;
	}
	else
	{
		//Insert into the non-empty list.
		r = queue_ptr->head;
		while(r != NULL)
		{
			if((to_add->next - ticks_from_start) < (r->next - ticks_from_start))
			{
				if(q != NULL)
				{
					q->nextT = to_add;
				}
				else
				{
					//we're inserting in the first position.
					queue_ptr->head = to_add;
				}

				to_add->nextT = r;
				return;
			}
			q = r;
			r = q->nextT;
		}
		//If we walk out of the loop, then we are inserting at the end of the list.
		queue_ptr->tail->nextT = to_add;
		queue_ptr->tail = to_add;
		to_add->nextT = NULL;
	}
}

/**
 * @brief Add a task the head of the queue
 *
 * @param queue_ptr the queue to insert in
 * @param task_to_add the task descriptor to add
 */
static void enqueue(task_queue_t* queue_ptr, task_descriptor_t* to_add)
{
    if(queue_ptr->head == NULL)
    {
        /* empty queue */
        queue_ptr->head = to_add;
        queue_ptr->tail = to_add;
    }
    else
    {
        /* put task at the back of the queue */
        queue_ptr->tail->next = to_add;
        queue_ptr->tail = to_add;
    }
}


static void budgequeue(task_queue_t* queue_ptr, task_descriptor_t* to_add)
{
	if(queue_ptr->head == NULL)
	{
		queue_ptr->head = queue_ptr->tail = to_add;
		to_add->next = NULL;
	}
	else
	{
		to_add->next = queue_ptr->head;
		queue_ptr->head = to_add;
	}
}


/**
 * @brief Pops head of queue and returns it.
 *
 * @param queue_ptr the queue to pop
 * @return the popped task descriptor
 */
static task_descriptor_t* dequeue(task_queue_t* queue_ptr)
{
    task_descriptor_t* task_ptr = queue_ptr->head;

	//If queue is not empty.
    if(queue_ptr->head != NULL)
    {
		if(queue_ptr->head == queue_ptr->tail)
		{
			//Last item in the queue.
			queue_ptr->head = queue_ptr->tail = NULL;
		}
		else
		{
			queue_ptr->head = queue_ptr->head->next;
		}
    }

    return task_ptr;
}

/**
 * @brief Pops an available periodic metadata structure and returns it.
 *
 * @return The available metadata, or NULL if none available.
 */
static periodic_task_metadata_t* periodic_dequeue(periodic_task_queue_t* queue_ptr)
{
	periodic_task_metadata_t* task_ptr = queue_ptr->head;

	//If queue is not empty.
	if(queue_ptr->head != NULL)
	{
		if(queue_ptr->head == queue_ptr->tail)
		{
			//Last item in the queue.
			queue_ptr->head = queue_ptr->tail = NULL;
		}
		else
		{
			queue_ptr->head = queue_ptr->head->nextT;
		}
	}

	return task_ptr;
}

/**
 * @brief Update the current time.
 *
 * Perhaps move to the next time slot of the PPP.
 */
static void kernel_update_ticker(void)
{
    /* PORTD ^= LED_D5_RED; */

	ticks_from_start += 1;
    current_timer_val = TCNT1;

    if(cur_task->priority == PERIODIC)
    {
		periodic_task_remticks--;
        if(periodic_task_remticks == 0)
        {
			error_msg = ERR_RUN_4_PERIODIC_TOOK_TOO_LONG;
			OS_Abort();
        }
    }
}

#undef SLOW_CLOCK //Uncomment for debugging.
#ifdef SLOW_CLOCK
/**
 * @brief For DEBUGGING to make the clock run slower
 *
 * Divide CLKI/O by 64 on timer 1 to run at 125 kHz  CS3[210] = 011
 * 1 MHz CS3[210] = 010
 */
static void kernel_slow_clock(void)
{
    TCCR1B &= ~(_BV(CS12) | _BV(CS10));
    TCCR1B |= (_BV(CS11));
}
#endif

/**
 * @brief Setup the RTOS and create main() as the first SYSTEM level task.
 *
 * Point of entry from the C runtime crt0.S.
 */
void kernel_init()
{
    int i;

    /* Set up the clocks */
    TCCR1B |= (_BV(CS11));

#ifdef SLOW_CLOCK
    kernel_slow_clock();
#endif

    /*
     * Initialize tasks lists for RR/SYS and PER, as well as dead pools to
	 * contain all but last task descriptor, and another for periodic meta datas.
	 */
    for (i = 0; i < MAXPROCESS; i++)
    {
        task_desc[i].state = DEAD;
		task_desc[i].next = &task_desc[i+1];
		task_desc[i].periodic_desc = NULL;
    }
	task_desc[i].next = NULL;
	task_desc[i-1].next = NULL; //Don't connect idle to tail.
	task_desc[i].state = DEAD;
	task_desc[i].periodic_desc = NULL;

	for (i = 0; i < MAXPERIODICPRO - 1; i ++)
	{
		periodic_task_desc[i].next = 0;
		periodic_task_desc[i].wcet = 0;
		periodic_task_desc[i].period = 0;
		periodic_task_desc[i].task = NULL;
		periodic_task_desc[i].nextT = &periodic_task_desc[i+1];
	}
	periodic_task_desc[i].task = NULL;
	periodic_task_desc[i].nextT = NULL;
	periodic_task_desc[i].next = 0;
	periodic_task_desc[i].wcet = 0;
	periodic_task_desc[i].period = 0;

	dead_pool_queue.head = &task_desc[0];
    dead_pool_queue.tail = &task_desc[MAXPROCESS-1]; //IDLE task not included.

	periodic_dead_pool_queue.head = &periodic_task_desc[0];
	periodic_dead_pool_queue.tail = &periodic_task_desc[MAXPERIODICPRO-1];

	periodic_task_queue.head = NULL;
	periodic_task_queue.tail = NULL;

	roundrobin_task_queue.head = NULL;
	roundrobin_task_queue.tail = NULL;

	system_task_queue.head = NULL;
	system_task_queue.tail = NULL;

	/* Create idle "task" */
    kernel_request_create_args.f = (voidfuncvoid_ptr)idle;
    kernel_request_create_args.priority = IDLE;
	kernel_request_create_args.arg = NULL;
    kernel_create_task();

    /* Create "main" task as SYSTEM level. This will be  */
    kernel_request_create_args.f = (voidfuncvoid_ptr)r_main;
    kernel_request_create_args.priority = SYSTEM;
	kernel_request_create_args.arg = NULL;
    kernel_create_task();

    /* First time through. Select "main" task to run first. */
    cur_task = task_desc;
    cur_task->state = RUNNING;
    dequeue(&system_task_queue);

    /* Set up Timer 1 Output Compare interrupt,the TICK clock. */
    TIMSK1 |= _BV(OCIE1A);
    OCR1A = TCNT1 + TICK_CYCLES;
    /* Clear flag. */
    TIFR1 = _BV(OCF1A);
}

/**
 *  @brief Delay function adapted from <util/delay.h>
 */
static void _delay_25ms(void)
{
    //uint16_t i;

    /* 4 * 50000 CPU cycles = 25 ms */
    //asm volatile ("1: sbiw %0,1" "\n\tbrne 1b" : "=w" (i) : "0" (50000));
    _delay_ms(25);
}

/** @brief Abort the execution of this RTOS due to an unrecoverable erorr.
 */
void OS_Abort(void)
{
    uint8_t i, j;
    uint8_t flashes, mask;

    Disable_Interrupt();

    /* Initialize port for output */
    // DDRH = LED_RED_MASK | LED_GREEN_MASK;
    DDRB = LED_RED_MASK | LED_GREEN_MASK;

    if(error_msg < ERR_RUN_0_USER_CALLED_OS_ABORT)
    {
        flashes = error_msg + 1;
        mask = LED_GREEN_MASK;
    }
    else
    {
        flashes = error_msg - ERR_RUN_0_USER_CALLED_OS_ABORT;
        mask = LED_RED_MASK;
    }

    for(;;)
    {
        PORTB = (uint8_t)(LED_RED_MASK | LED_GREEN_MASK);

        for(i = 0; i < 100; ++i)
        {
               _delay_25ms();
        }

        PORTB = (uint8_t) 0;

        for(i = 0; i < 40; ++i)
        {
               _delay_25ms();
        }

        for(j = 0; j < flashes; ++j)
        {
            PORTB = mask;

            for(i = 0; i < 10; ++i)
            {
                _delay_25ms();
            }

            PORTB = (uint8_t) 0;

            for(i = 0; i < 10; ++i)
            {
                _delay_25ms();
            }
        }

        for(i = 0; i < 20; ++i)
        {
            _delay_25ms();
        }
    }
}




/**
  * @brief The calling task gives up its share of the processor voluntarily.
  */
void Task_Next()
{
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = TASK_NEXT;
    enter_kernel();

    SREG = sreg;
}


/**
  * @brief The calling task terminates itself.
  */
void Task_Terminate()
{
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = TASK_TERMINATE;
    enter_kernel();

    SREG = sreg;
}


/** @brief Retrieve the assigned parameter.
 */
int Task_GetArg(void)
{
	//Direct fetch, no need to enter kernel.
    return cur_task->arg;
}

/**
 * @param f  a parameterless function to be created as a process instance
 * @param arg an integer argument to be assigned to this process instance
 * @param priority assigned scheduling level: SYSTEM, PERIODIC or ROUND_ROBIN
 * @param name assigned PERIODIC process name
 * @return 0 if Successful, non-zero otherwise.
 *
 *  A new process  is created to execute the parameterless
 *  function @a f with an initial parameter @a arg, which is retrieved
 *  by a call to Task_GetArg().  If a new process cannot be
 *  created, a non-zero value will be returned.
 *  The created process will belong to its scheduling @a priority.
 */
int kernel_create_helper(void (*f)(void), int arg, task_priority_t priority)
{
    int retval;
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request_create_args.f = (voidfuncvoid_ptr)f;
    kernel_request_create_args.arg = arg;
    kernel_request_create_args.priority = priority;

    kernel_request = TASK_CREATE;
    enter_kernel();

    retval = kernel_request_retval;
    SREG = sreg;

    return retval;
}

int8_t Task_Create_System(void (*f)(void), int16_t arg)
{
	return kernel_create_helper(f, arg, SYSTEM);
}

int8_t Task_Create_RR(void (*f)(void), int16_t arg)
{
	return kernel_create_helper(f, arg, ROUND_ROBIN);
}

int8_t Task_Create_Periodic(void(*f)(void), int16_t arg, uint16_t period, uint16_t wcet, uint16_t start)
{
	if(period == 0 || period <= wcet)
	{
		//If period is zero, or the period is shorter that the
		//worst case, thats an invalid configuration.
		error_msg = ERR_RUN_3_PERIODIC_INVALID_CONFIGURATION;
		OS_Abort();
	}

	kernel_period_create_meta.period = period;
	kernel_period_create_meta.next = start;
	kernel_period_create_meta.wcet = wcet;

	return kernel_create_helper(f, arg, PERIODIC);
}

/**
 * \return a non-NULL SERVICE descriptor if successful; NULL otherwise.
 *
 *  Initialize a new, non-NULL SERVICE descriptor.
 */
SERVICE* Service_Init()
{
	SERVICE * new_service_ptr;
	uint8_t sreg;

	sreg = SREG;
	Disable_Interrupt();

	kernel_request = SERVICE_INIT;
	enter_kernel();

	new_service_ptr = (SERVICE *) kernel_request_service_init_retval;

	SREG = sreg;
	return new_service_ptr;
}

/**
 * \param s an Service descriptor
 * \param v pointer to memory where the received value will be written
 * Add tasks to the service's queue of subscribed tasks
 */
void Service_Subscribe( SERVICE *s, int16_t *v )
{
	uint8_t sreg;

	sreg = SREG;
	Disable_Interrupt();

	kernel_request_service_descriptor = s;
    kernel_request_service_sub_data = v;
	kernel_request = SERVICE_SUB;
	enter_kernel();

	SREG = sreg;
}

/**
 * Publish a message to be seen by all tasks subscribed to a service
 */
void Service_Publish( SERVICE *s, int16_t v )
{
    uint8_t sreg;
    sreg = SREG;
    Disable_Interrupt();

    kernel_request_service_descriptor = s;
    kernel_request_service_pub_data = v;
    kernel_request = SERVICE_PUB;
    enter_kernel();

    SREG = sreg;
}

void OS_DisablePreemption()
{
	kernel_preemption_disabled = 1;
}

uint16_t Now()
{
//    return ticks_from_start * TICK + (TCNT1 + HALF_MS) / (CYCLES_PER_MS)
    return ticks_from_start * TICK + ((TCNT1 - current_timer_val)/(F_CPU/TIMER_PRESCALER/1000));
}

/**
 * Runtime entry point into the program; just start the RTOS.
 * The application layer must define r_main() for its entry point,
 * and will be called after the OS is initialized.
 */
int main()
{
    //EnableProfileSample1();
	kernel_init();
    //DisableProfileSample1();
	kernel_main_loop();
	return 0;
}
