/*
 * spi.h
 *
 * Header file
 *
 */

#ifndef SPI_H
#define SPI_H

/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/

/* Third party libraries include */
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"

/* Local includes */

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

#define CONSTANT_1 (1U)

/*----------------------------------------------------------------------------*/
/* Types                                                                      */
/*----------------------------------------------------------------------------*/

typedef struct
{
} spi_struct_t;

/*----------------------------------------------------------------------------*/
/* Event call-backs                                                           */
/*----------------------------------------------------------------------------*/

typedef void (*spi_cb_t)(void);

/*----------------------------------------------------------------------------*/
/* Services                                                                   */
/*----------------------------------------------------------------------------*/

typedef struct
{
    void (*open)(void);

} spi_services_t;

/*----------------------------------------------------------------------------*/
/* Initialisation                                                             */
/*----------------------------------------------------------------------------*/

void spi_init(spi_services_t *spi_services);

#endif
