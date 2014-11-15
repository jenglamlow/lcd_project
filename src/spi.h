/*
 * =====================================================================================
 *
 *       Filename:  spi.h
 *
 *    Description:  SPI services header file
 *
 *        Version:  1.0
 *        Created:  11/15/2014 02:07:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam (jeng), jenglamlow@gmail.com
 *   Organization:  Malaysia
 *
 * =====================================================================================
 */

#ifndef SPI_H
#define SPI_H

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/

/* Third party libraries include */
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"

/* Local includes */

/*-----------------------------------------------------------------------------
 *  Constants
 *-----------------------------------------------------------------------------*/
#define CONSTANT_1 (1U)

/*-----------------------------------------------------------------------------
 *  Types
 *-----------------------------------------------------------------------------*/
typedef struct
{
} spi_struct_t;

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/
typedef void (*spi_cb_t)(void);

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/
typedef struct
{
    void (*open)(void);

} spi_services_t;

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/
void spi_init(spi_services_t *spi_services);

#endif