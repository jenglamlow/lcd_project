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
    UART_CMD,
} uart_instance_t;

/*----------------------------------------------------------------------------*/
/* Event call-backs                                                           */
/*----------------------------------------------------------------------------*/

/* Signal that there is data available to read */
typedef void (*uart_data_available_cb_t)(void);

/*----------------------------------------------------------------------------*/
/* Services                                                                   */
/*----------------------------------------------------------------------------*/

/* Function Pointer for UART component service */
typedef struct 
{
    /* Open UART communication */
    void (*open)(uart_instance_t          uart_instance);

    /* Close UART communication */
    void (*close)(uart_instance_t         uart_instance);

    /* Read data from UART */
    void (*read)(uart_instance_t    uart_instance,
                 uint8_t            *buffer,
                 uint32_t           buffer_size);

    /* Write data to UART */
    void (*write)(uart_instance_t       uart_instance,
                  uint8_t               *data,
                  uint32_t              data_size);

    uint32_t (*data_available)(uart_instance_t uart_instance);

    /* Print function similar as C printf - for debugging */
    void (*print)(const char *fmt, ...);

} uart_services_t;

/*----------------------------------------------------------------------------*/
/* Initialisation                                                             */
/*----------------------------------------------------------------------------*/

/* UART driver component initialization */
void uart_init(uart_services_t          *uart_services);

#endif