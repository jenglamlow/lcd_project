/*
 * =====================================================================================
 *
 *       Filename:  utils.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/15/2014 01:54:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef UTILS_H
#define UTILS_H

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/
/* Third party libraries include */

/* Local includes */
#include "lib.h"

/*-----------------------------------------------------------------------------
 *  Constants
 *-----------------------------------------------------------------------------*/

/* Function Macro */
/* Set bits of the register */
#define SET_BITS(reg, mask)         ROM_GPIOPinWrite(reg, mask, mask)

/* Clear bits of the register */
#define CLEAR_BITS(reg, mask)       ROM_GPIOPinWrite(reg, mask, 0x00)

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

/* For SysTick */ 
/* Hardcoded to 80Mhz clock frequency */
#define F_CPU 80000000

#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKHZ               100

#define SYSTICK_INT_PRIORITY    0x80

/*-----------------------------------------------------------------------------
 *  Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

uint16_t convert_to_word(uint8_t high_byte, 
                         uint8_t low_byte);

void delay_us(uint32_t us);

void delay_ms(uint32_t ms);
#endif
