/*
 * =====================================================================================
 *
 *       Filename:  evl.c
 *
 *    Description:  Implementation file for event loop component
 *
 *        Version:  1.0
 *        Created:  03/10/2015 07:32:15 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam (jeng), jenglamlow@gmail.com
 *   Organization:  Malaysia
 *
 * =====================================================================================
 */
/*---------------------------------------------------------------------------*/
/* Includes                                                                  */
/*---------------------------------------------------------------------------*/

#include "evl.h"

/* Third Party Libraries */
#include "driverlib/interrupt.h"

/*---------------------------------------------------------------------------*/
/* Configuration                                                             */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Constants                                                                 */
/*---------------------------------------------------------------------------*/

#define MAX_EVENT (256u)

/*---------------------------------------------------------------------------*/
/* Private types                                                             */
/*---------------------------------------------------------------------------*/

/* Data structure for an event. */
typedef struct
{
    /* Event Callback Handle */
    evl_cb_handle_t         handle;

    /* Event callback function */
    evl_cb_t                evl_cb;

    /* Event index (To differentiate the index of same callback) */
    uint16_t                index;

    /* Shared between ISR and non-ISR context. */
    /* Schedule flag */
    volatile bool           is_scheduled;

    /* Next Event Handle */
    volatile uint16_t       next_handle;

    /* Previous Event Handle */
    volatile uint16_t       prev_handle;
} evt_t;

/*---------------------------------------------------------------------------*/
/* Private data                                                              */
/*---------------------------------------------------------------------------*/

/* Shared between ISR and non-ISR context. */
/* Handle for the first event in queue. */
static volatile uint16_t head_handle;

/* Handle for the last event in queue. */
static volatile uint16_t tail_handle;

/* Number of events in queue. */
static volatile uint16_t size;

/* Array of events. */
static evt_t event_array[MAX_EVENT];

/* Array to store free list of event index. */
static evl_cb_handle_t free_list[MAX_EVENT];

/* Current index for free list of events. */
static evl_cb_handle_t free_list_index;

/* For breaking the run loop in unit test. */
static bool terminate;

/*---------------------------------------------------------------------------*/
/* Helper functions                                                          */
/*---------------------------------------------------------------------------*/

/* Get current number of events in queue. */
static uint16_t get_queue_size(void)
{
    return size;
}

/* 
 * Add an event to the end of queue. 
 * Caller is responsible for ISR safety. 
 */
static void push_queue(evt_t* evt)
{
    ASSERT(get_queue_size() < MAX_EVENT);
    ASSERT(evt != NULL);

    /* Check whether event queue contains any pending event */
    if (get_queue_size() > 0u)
    {
        /* Set the previous event next handle to point to current handle*/
        event_array[tail_handle].next_handle = evt->handle;

        /* Set the current event previous handle to zero */
        evt->prev_handle = tail_handle;

        /* Update tail handle */
        tail_handle = evt->handle;
    }
    else
    {
        /* If queue size is empty */

        /* Set the previous handle to zero (no pending event before) */
        evt->prev_handle = 0u;

        /* head = tail = evt_handle (Update) */
        head_handle = evt->handle;
        tail_handle = head_handle;
    }
    
    /* Set next handle to zero (No event after) */
    evt->next_handle = 0u;
    evt->is_scheduled = true;
    size++;
}

/* 
 * Remove an event at the beginning of queue. 
 * Caller is responsible for ISR safety. 
 */
static evt_t* pop_queue(void)
{
    evt_t* evt_head = NULL;

    if (get_queue_size() > 0u)
    {
        /* Get event handle from head */
        evt_head = &(event_array[head_handle]);

        /* Update head handle to point to next handle */
        head_handle = evt_head->next_handle;

        /* Clear head event handle (already service) */
        event_array[head_handle].prev_handle = 0u;
        evt_head->next_handle = 0u;
        evt_head->prev_handle = 0u;
        evt_head->is_scheduled = false;

        size--;
    }

    return evt_head;
}


/*---------------------------------------------------------------------------*/
/* IRQ handlers                                                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Services                                                                  */
/*---------------------------------------------------------------------------*/

/*
 * Starts the main event loop of the software. 
 */
static void evl_run(void)
{
    const evt_t* evl;

    do
    {
        IntMasterDisable();
        evl = pop_queue();
        IntMasterEnable();

        if (evl != NULL)
        {
            /* Invoke event loop callback */
            evl->evl_cb(evl->index);
        }

    } while (!terminate);
}

/*
 * To allocate a callback handle used to schedule callbacks from ISR context. 
 */
static evl_cb_handle_t evl_cb_alloc(evl_cb_t evl_cb,
                                    uint8_t index)
{
    ASSERT(free_list_index < MAX_EVENT);
    ASSERT(evl_cb != NULL);

    /* Get callback handle from free list array */
    evl_cb_handle_t evl_cb_handle = free_list[free_list_index++];
    
    /* Subcribe callback and input index for event*/ 
    evt_t *evt = &(event_array[evl_cb_handle]);
    evt->evl_cb = evl_cb;
    evt->index = index;

    return evl_cb_handle;
}

/* 
 * To schedule a call to callback function associated with the specified
 * callback handle. This function is used by the driver components and 
 * must only be called from ISR context.
 */
static void evl_schedule(evl_cb_handle_t evl_cb_handle)
{
    ASSERT(evl_cb_handle < MAX_EVENT);

    evt_t* evt;

    ASSERT(evt != NULL);

    if (evl_cb_handle < MAX_EVENT)
    {
        /* Get event object using callback handle */
        evt = &(event_array[evl_cb_handle]); 

        /* Check if event is scheduled */
        if (!(evt->is_scheduled))
        {
            push_queue(evt);
        }
    }
}

/* 
 * To cancel a previously scheduled event.
 * The caller is responsible for ISR safety.
 */
static void evl_cancel(evl_cb_handle_t evl_cb_handle)
{
    ASSERT(evl_cb_handle < MAX_EVENT);

    evt_t* evt;

    ASSERT(evt != NULL);

    evt = &(event_array[evl_cb_handle]);

    if (evt->is_scheduled)
    {
        /* if callback handle is at the first of the queue */
        if (head_handle == evl_cb_handle)
        {
            /* Update head handle point to the next handle */
            head_handle = evt->next_handle;

            /* Clear the new head handle previous handle */
            event_array[head_handle].prev_handle = 0u;
        }
        /* if callback handle is at the end of the queue */
        else if (tail_handle == evl_cb_handle)
        {
            /* Update tail handle to point to second last handle */ 
            tail_handle = evt->prev_handle;

            /* Clear the new tail handle next handle */
            event_array[tail_handle].next_handle = 0u;
        }
        /* In between head and tail index */
        else
        {
            /* Update the linked list of the event */
            event_array[evt->prev_handle].next_handle = 
                                            evt->next_handle;
            event_array[evt->next_handle].prev_handle = 
                                            evt->prev_handle;
        }

        evt->is_scheduled = false;
        evt->prev_handle = 0u;
        evt->next_handle = 0u;

        size--;
    }
}

/* To terminate the run loop for unit testing. */
static void event_terminate(void)
{
    terminate = true;
}

/*---------------------------------------------------------------------------*/
/* Initialisation                                                            */
/*---------------------------------------------------------------------------*/

void evl_init(evl_services_t*   evl_services)
{
    uint32_t i;

    evl_services->run = evl_run;
    evl_services->cb_alloc = evl_cb_alloc;
    evl_services->schedule = evl_schedule;
    evl_services->cancel = evl_cancel;
    evl_services->terminate = event_terminate;

    for (i = 0; i < MAX_EVENT; i++)
    {
        event_array[i].handle = i;
        event_array[i].next_handle = 0u;
        event_array[i].prev_handle = 0u;
        event_array[i].is_scheduled = false;
        free_list[i] = i;
    }

    head_handle = 0u;
    tail_handle = 0u;
    size = 0u;
    terminate = false;

    /* 0 not used. */
    free_list_index = 1u; 
}
