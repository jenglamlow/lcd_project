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
#include "driverlib/udma.h"

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

static void dma_init()
{

    /* Enable DMA for SSI0 TX */
    SSIDMAEnable(SSI0_BASE, SSI_DMA_TX);

    /* DMA channel control */
    uDMAChannelControlSet(UDMA_CHANNEL_SSI0TX, 
                          UDMA_SIZE_8| UDMA_SRC_INC_NONE| UDMA_DST_INC|
                          UDMA_ARB_4);

    /* uDMAChannelTransferSet(UDMA_CHANNEL_SSI0TX, */
    /*                        UDMA_MODE_BASIC, */
    /*                        (void*)&data) */
}
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
    /* read buffer (8 bytes FIFO size) */
    uint16_t dummy_read_buffer[8];

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
    GPIOPinTypeSSI(GPIO_PORTA_BASE, 
                   GPIO_PIN_5 |GPIO_PIN_4 | GPIO_PIN_3 |GPIO_PIN_2);

    /* 
     * SPI set using SSI0
     * Mode 0: Polariry 0 and Phase 0
     * Master Mode 
     * 10 Mhz communication speed
     * 8 bits data 
     */ 
    SSIConfigSetExpClk(SSI0_BASE, 
                       SysCtlClockGet(), 
                       SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 
                       10000000, 
                       8);

    /* Enable SSI0 module */
    SSIEnable(SSI0_BASE);
    
    /* Clear any residual data */
    while(SSIDataGetNonBlocking(SSI0_BASE, &dummy_read_buffer[0]));

}

/**
* @brief  Close SPI module
*/
static void spi_close(void)
{
    SSIDisable(SSI0_BASE);
}

static void spi_write(uint8_t* data, uint32_t size)
{

}


/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

/**
 * @brief SPI services initialisation
 *
 * @param spi_services
 */
void spi_init(spi_services_t *spi_services)
{
   spi_services->open = spi_open;
   spi_services->close = spi_close;
}

