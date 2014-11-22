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

/*-----------------------------------------------------------------------------
 *  Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

void delay_us(uint32_t us);

void delay_ms(uint32_t ms);
#endif
