/*
 * =====================================================================================
 *
 *       Filename:  setting.h
 *
 *    Description:  Global Setting Definition
 *
 *        Version:  1.0
 *        Created:  05/12/2015 01:54:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef SETTING_H
#define SETTING_H

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/
/* Third party libraries include */

/* Local includes */
#include "lib.h"

/*-----------------------------------------------------------------------------
 *  Constants
 *-----------------------------------------------------------------------------*/

/* UART */
/* UART Transmit Buffer Size */
#define UART_RX_BUFFER_SIZE      (4096U)

/* UART Receive Buffer Size */
#define UART_TX_BUFFER_SIZE      (512U)

/* UART Baudrate */
#define UART_BAUD_RATE           (460800U)

/* UART Command Port */
#define UART_CMD_1


/* SSI Speed Definition */
#define SSI_SPEED               (25000000U)


#endif

