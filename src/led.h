/*
 * =====================================================================================
 *
 *       Filename:  template.h
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

#ifndef LED_H
#define LED_H

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/
/* Third party libraries include */

/* Local includes */
#include "lib.h"


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

void led_start(void);
void led_task(void);


/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/
void led_init(void);

#endif
