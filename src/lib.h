/*
 * =====================================================================================
 *
 *       Filename:  lib.h
 *
 *    Description:  Header file containing all the necessity include file
 *
 *        Version:  1.0
 *        Created:  11/15/2014 08:22:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam (jeng), jenglamlow@gmail.com
 *   Organization:  Malaysia
 *
 * =====================================================================================
 */

#ifndef LIB_H
#define LIB_H

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/

/* Standard Types */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Stellaris library */
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

/* Stellaris utility library */

/* Helper library */
#include "utilities.h"

#endif
