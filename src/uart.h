/*
 * =====================================================================================
 *
 *       Filename:  uart.h
 *
 *    Description:  Header file for UART component
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

#ifndef UART_H_
#define UART_H_

/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/

/* Local includes */
#include "lib.h"

/*----------------------------------------------------------------------------*/
/* Types                                                                      */
/*----------------------------------------------------------------------------*/

/* UART driver list */
typedef enum {
    UART_0,
    UART_CMD,
    UART_2,
} uart_instance_t;

/*----------------------------------------------------------------------------*/
/* Event call-backs                                                           */
/*----------------------------------------------------------------------------*/

/* Signal that there is data available to read */
typedef void (*uart_data_available_cb_t)(void);

/*----------------------------------------------------------------------------*/
/* Services                                                                   */
/*----------------------------------------------------------------------------*/

void uart_open(uart_instance_t          uart_instance,
               uart_data_available_cb_t uart_data_available_cb);

void uart_close(uart_instance_t uart_instance);

void uart_read(uart_instance_t   uart_instance,
               uint8_t           *buffer,
               uint32_t          buffer_size);

void uart_write(uart_instance_t  uart_instance,
                uint8_t          *buffer,
                uint32_t         buffer_size);

void uart_task(void);

/*----------------------------------------------------------------------------*/
/* Initialisation                                                             */
/*----------------------------------------------------------------------------*/

/* UART driver component initialization */
void uart_init(void);

#endif
