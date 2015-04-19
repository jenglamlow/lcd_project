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

/*-----------------------------------------------------------------------------
 *  Constants
 *-----------------------------------------------------------------------------*/

#define USE_INTERRUPT   0

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

/**
 * SPI Transmit Complete Callback
 */
typedef void (*spi_tx_cb_t)(void);

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/
void spi_open(spi_instance_t spi_instance,
              spi_tx_cb_t    spi_tx_cb);

void spi_close(spi_instance_t spi_instance);

void spi_write(spi_instance_t spi_instance,
                      uint8_t        data);
/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

void spi_init(void);

#endif
