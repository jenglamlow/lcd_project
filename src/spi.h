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

/* Local includes */
#include "lib.h"
#include "evl.h"

/*-----------------------------------------------------------------------------
 *  Constants
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Types
 *-----------------------------------------------------------------------------*/

/* SPI instance type */
typedef enum
{
    SPI_TFT = 0,
    SPI_SD_CARD,
    SPI_COUNT
} spi_instance_t;

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

typedef void (*spi_tx_cb_t)(void);

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/* SPI services function pointer */
typedef struct
{
    void (*open)(spi_instance_t spi_instance,
                 spi_tx_cb_t    spi_tx_cb);

    void (*close)(spi_instance_t spi_instance);

    void (*write)(spi_instance_t spi_instance, uint8_t data);
    
    void (*write_non_blocking)(spi_instance_t spi_instance,
                               uint8_t        data);

} spi_services_t;

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

void spi_init(spi_services_t        *spi_services,
              evl_services_t        *evl_services);

#endif
