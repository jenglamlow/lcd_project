/*
 * =====================================================================================
 *
 *       Filename:  cmd_parser.h
 *
 *    Description:  Header file for command Parser for UART message received 
 *
 *        Version:  1.0
 *        Created:  02/03/2015 07:32:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam (jeng), jenglamlow@gmail.com
 *   Organization:  Malaysia
 *
 * =====================================================================================
 */
#ifndef CMD_PARSER_H
#define CMD_PARSER_H

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/
/* Third party libraries include */
#include "uart.h"
#include "tft.h"

/* Local includes */

/*-----------------------------------------------------------------------------
 *  Constants
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

typedef struct
{

    void (*process)(uint8_t byte);

    void (*start)(void);

    void (*stop)(void);
    
} cmd_parser_services_t;
/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

void cmd_parser_init(cmd_parser_services_t*   cmd_parser_services,
                     uart_services_t*         uart_services,
                     tft_services_t*          tft_services);

#endif

