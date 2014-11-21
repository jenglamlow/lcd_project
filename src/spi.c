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

typedef struct
{

} spi_state_t;

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

static spi_state_t spi_state[SPI_COUNT];

/* SPI instance index mapping */
static uint8_t spi_index_map[SPI_COUNT] = 
{
   0,    /* SPI_LCD - SSI0 */
   1     /* SPI_SD_CARD - SSI1 */
};

/* SSI Base map */
static const uint32_t ssi_base_map[] = 
{
    SSI0_BASE, SSI1_BASE, SSI2_BASE, SSI3_BASE
};

/* SSI Peripheral Map */
static const uint32_t ssi_peripheral_map[] = 
{
        SYSCTL_PERIPH_SSI0, SYSCTL_PERIPH_SSI1, SYSCTL_PERIPH_SSI2, SYSCTL_PERIPH_SSI3
};

/* SSI GPIO Configuration Map */
static const uint32_t ssi_gpio_config_map[][4] = 
{
    {GPIO_PA2_SSI0CLK, GPIO_PA3_SSI0FSS, GPIO_PA4_SSI0RX, GPIO_PA5_SSI0TX},
    {GPIO_PF2_SSI1CLK, GPIO_PF3_SSI1FSS, GPIO_PF0_SSI1RX, GPIO_PF1_SSI1TX},
    {GPIO_PB4_SSI2CLK, GPIO_PB5_SSI2FSS, GPIO_PB6_SSI2RX, GPIO_PB7_SSI2TX},
    {GPIO_PD0_SSI3CLK, GPIO_PD1_SSI3FSS, GPIO_PD2_SSI3RX, GPIO_PD3_SSI3TX}
};

/* SSI GPIO Port Map */
static const unsigned long ssi_gpio_port_map[] = 
{
    GPIO_PORTA_BASE, GPIO_PORTF_BASE, GPIO_PORTB_BASE, GPIO_PORTD_BASE
};

/* SSI GPIO Pin Map */
static const unsigned long ssi_gpio_pin_map[] = 
{
	GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5,
	GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
	GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
	GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
};

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

static void dma_init()
{

    /* Enable DMA for SSI0 TX */
    SSIDMAEnable(SSI0_BASE, SSI_DMA_TX);

    /* DMA channel control */
    /* uDMAChannelControlSet(UDMA_CHANNEL_SSI0TX, */ 
    /*                       UDMA_SIZE_8| UDMA_SRC_INC_NONE| UDMA_DST_INC| */
    /*                       UDMA_ARB_4); */

    /* uDMAChannelTransferSet(UDMA_CHANNEL_SSI0TX, */
    /*                        UDMA_MODE_BASIC, */
    /*                        (void*)&data) */

    /* Enable uDMA channel for SSI0TX */
    uDMAChannelEnable(UDMA_CHANNEL_SSI0TX);

    /* IntEnable(INT_SSI0); */
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
static void spi_open(spi_instance_t spi_instance)
{
    /* SSI module map based on spi instance */
    uint8_t ssi_module = spi_index_map[spi_instance];

    /* SPI state instance */
    spi_state_t *state = &spi_state[spi_instance];

    /* Initialise SSI peripheral */
    ROM_SysCtlPeripheralEnable(ssi_peripheral_map[ssi_module]);

    /* Disable SSI module */ 
    ROM_SSIDisable(ssi_base_map[ssi_module]);

    /* Configure PORT pin muxing as SSI peripheral function */  
    ROM_GPIOPinConfigure(ssi_gpio_config_map[ssi_module][0]);
    ROM_GPIOPinConfigure(ssi_gpio_config_map[ssi_module][1]);
    ROM_GPIOPinConfigure(ssi_gpio_config_map[ssi_module][2]);
    ROM_GPIOPinConfigure(ssi_gpio_config_map[ssi_module][3]);

    /* Configure the GPIO settings for SSI pins */
    ROM_GPIOPinTypeSSI(ssi_gpio_port_map[ssi_module], 
                       ssi_gpio_pin_map[ssi_module]);

    /* Set Clock Source for SSI */
    ROM_SSIClockSourceSet(ssi_base_map[ssi_module], SSI_CLOCK_SYSTEM);

    /* 
     * SPI set using SSI
     * Mode 0: Polariry 0 and Phase 0
     * Master Mode 
     * 10 Mhz communication speed
     * 8 bits data 
     */ 
    ROM_SSIConfigSetExpClk(ssi_base_map[ssi_module], 
                           SysCtlClockGet(), 
                           SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, 
                           10000000, 
                           8);

    /* Enable SSI module */
    ROM_SSIEnable(ssi_base_map[ssi_module]);
    
    /* Clear any residual data */
    uint64_t dummy_read_buffer[8];
    while(ROM_SSIDataGetNonBlocking(ssi_base_map[ssi_module], 
                                    &dummy_read_buffer[0]));
}


/**
* @brief  Close SPI module
*/
static void spi_close(spi_instance_t spi_instance)
{
    SSIDisable(spi_index_map[spi_instance]);
}

static void spi_write(spi_instance_t spi_instance,
                      uint8_t data)
{
    /* SSI module map based on spi instance */
    uint8_t ssi_module = spi_index_map[spi_instance];

    /* Write Data to SSI */
    ROM_SSIDataPut(ssi_base_map[ssi_module], (uint8_t)data);

    /* Get Data from SSI */
    uint32_t rx_data;
    ROM_SSIDataGet(ssi_base_map[ssi_module], &rx_data);
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
   spi_services->write = spi_write;
}

