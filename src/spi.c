/*
 * =====================================================================================
 *
 *       Filename:  spi.c
 *
 *    Description:  SPI services implementation file
 *
 *        Version:  1.0
 *        Created:  11/15/2014 02:06:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam (jeng), jenglamlow@gmail.com
 *   Organization:  Malaysia
 *
 * =====================================================================================
 */

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/
/* Local Includes */
#include "spi.h"

/* Third party libraries include */
#include "driverlib/ssi.h"

/* Local includes */

/*-----------------------------------------------------------------------------
 *  Configurations
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  IRQ Handler
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/**
 * @brief SPI initialise GPIO and hardware peripheral.
 *        Use PortA 
 */
static void spi_open(void)
{
    /* Initialise SSI0 peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    /* Initialise Port A for SSI0 peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    /* Configure PORTA pin muxing as SSI0 peripheral function */  
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);

    /* Configure the GPIO settings for SSI pins */
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 |
                   GPIO_PIN_2);

    /* SPI set using SSI0
     * Mode 0: Polariry 0 and Phase 0
     * Master Mode 
     * 10 Mhz communication speed
     * 8 bits data 
     */ 
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 10000000, 8);
}

/**
* @brief  Close SPI module
*/
static void spi_close(void)
{

}


/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

/**
 * @brief 
 *
 * @param spi_services
 */
void spi_init(spi_services_t *spi_services)
{
   spi_services->open = spi_open;
   spi_services->close = spi_close;
}

