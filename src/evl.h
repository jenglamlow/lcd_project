/*
 * =====================================================================================
 *
 *       Filename:  evl.h
 *
 *    Description:  Header file for event loop component
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

#ifndef EVL_H_
#define EVL_H_

/*---------------------------------------------------------------------------*/
/* Includes                                                                  */
/*---------------------------------------------------------------------------*/

/* Local includes */
#include "lib.h"

/*---------------------------------------------------------------------------*/
/* Types                                                                     */
/*---------------------------------------------------------------------------*/

/* Handle used for scheduling callbacks. */
typedef uint32_t evl_cb_handle_t;

/*---------------------------------------------------------------------------*/
/* Constants                                                                 */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Event callbacks                                                           */
/*---------------------------------------------------------------------------*/

/* Event loop callback in non-ISR context. */
typedef void (*evl_cb_t)(uint32_t index);

/*---------------------------------------------------------------------------*/
/* Services                                                                  */
/*---------------------------------------------------------------------------*/

typedef struct
{
    /* Start the main loop of the software. */
    void (*run)(void);

    /* Allocate a callback handle. */
    evl_cb_handle_t (*cb_alloc)(evl_cb_t evl_cb,
                                uint32_t index);

    /* Schedule a call to the callback function. */
    void (*schedule)(evl_cb_handle_t cb_handle);

    /* Cancel a previously scheduled event. */
    void (*cancel)(evl_cb_handle_t cb_handle);

    /* To configure the run loop to iterate only once upon each call. */
    void (*terminate)(void);

} evl_services_t;

/*---------------------------------------------------------------------------*/
/* Initialisation                                                            */
/*---------------------------------------------------------------------------*/

void evl_init(evl_services_t    *evl_services);

#endif
