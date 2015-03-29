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

/* State per SSI device */
typedef struct {

    /* Store UART Instance */
    spi_instance_t instance;

    /* Receive Event Handle */
    evl_cb_handle_t evl_tx_handle;

    /* Client data available callback */
    spi_tx_cb_t tx_cb;

} spi_state_t;

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

/* SPI instance index mapping */
static const uint8_t spi_index[SPI_COUNT] = 
{
   0,    /* SPI_TFT - SSI0 */
   1     /* SPI_SD_CARD - SSI1 */
};

/* SSI Base map */
static const uint32_t ssi_base[] = 
{
    SSI0_BASE, SSI1_BASE, SSI2_BASE, SSI3_BASE
};

/* SSI Peripheral GPIO Map */
static const uint32_t ssi_peripheral_gpio[] = 
{
    SYSCTL_PERIPH_GPIOA, SYSCTL_PERIPH_GPIOF, SYSCTL_PERIPH_GPIOB, SYSCTL_PERIPH_GPIOD
};

/* SSI Peripheral Map */
static const uint32_t ssi_peripheral[] = 
{
    SYSCTL_PERIPH_SSI0, SYSCTL_PERIPH_SSI1, SYSCTL_PERIPH_SSI2, SYSCTL_PERIPH_SSI3
};

/* SSI GPIO Configuration Map */
static const uint32_t ssi_gpio_config[][4] = 
{
    {GPIO_PA2_SSI0CLK, GPIO_PA3_SSI0FSS, GPIO_PA4_SSI0RX, GPIO_PA5_SSI0TX},
    {GPIO_PF2_SSI1CLK, GPIO_PF3_SSI1FSS, GPIO_PF0_SSI1RX, GPIO_PF1_SSI1TX},
    {GPIO_PB4_SSI2CLK, GPIO_PB5_SSI2FSS, GPIO_PB6_SSI2RX, GPIO_PB7_SSI2TX},
    {GPIO_PD0_SSI3CLK, GPIO_PD1_SSI3FSS, GPIO_PD2_SSI3RX, GPIO_PD3_SSI3TX}
};

/* SSI GPIO Port Map */
static const uint32_t ssi_gpio_port[] = 
{
    GPIO_PORTA_BASE, GPIO_PORTF_BASE, GPIO_PORTB_BASE, GPIO_PORTD_BASE
};

/* SSI GPIO Pin Map */
static const uint32_t ssi_gpio_pin[] = 
{
    GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5,
    GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
    GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
    GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
};

/* SSI Interrupt Map */
static const uint32_t ssi_int[] = 
{
    INT_SSI0,
    INT_SSI1,
};

static evl_services_t *evl;

static spi_state_t spi_state[SPI_COUNT];

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

static void spi_tx_evl_cb(uint8_t ix)
{

}

/*-----------------------------------------------------------------------------
 *  IRQ Handler
 *-----------------------------------------------------------------------------*/

/* Generic SPI IRQ handler */
static void spi_irq(spi_instance_t spi_instance)
{
    uint32_t status;
    spi_state_t *state = &spi_state[spi_instance];
    uint32_t base = ssi_base[spi_instance];
    uint8_t read_byte;

    /* Get UART Status Flag */
    status = ROM_UARTIntStatus(ssi_base[spi_instance], true);

    /* Clear Interrupt source */
    ROM_UARTIntClear(ssi_base[spi_instance], status);

    if (status & (SSI_RXFF | SSI_RXTO))
    {

    }
}

void SSI0IntHandler(void)
{
    spi_irq(SPI_TFT);
}

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/**
 * @brief SPI initialise GPIO and hardware peripheral.
 *        Use PortA 
 */
static void spi_open(spi_instance_t spi_instance,
                     spi_tx_cb_t    spi_tx_cb)
{
    /* SSI module map based on spi instance */
    uint8_t ssi_module = spi_index[spi_instance];
    uint32_t base = ssi_base[spi_instance];
    
    /* SSI State Initialise */
    spi_state_t *state = &spi_state[spi_instance];

    /* UART State Initialization */
    state->instance = spi_instance;

    /* Subscribe data available callback */
    state->tx_cb = spi_tx_cb;

    /* Initialise SSI peripheral */
    ROM_SysCtlPeripheralEnable(ssi_peripheral[ssi_module]);

    /* Disable SSI module */ 
    ROM_SSIDisable(base);
    
    /* Enable GPIO port */ 
    ROM_SysCtlPeripheralEnable(ssi_peripheral_gpio[ssi_module]);

    /* Configure PORT pin muxing as SSI peripheral function */  
    ROM_GPIOPinConfigure(ssi_gpio_config[ssi_module][0]);
    ROM_GPIOPinConfigure(ssi_gpio_config[ssi_module][1]);
    ROM_GPIOPinConfigure(ssi_gpio_config[ssi_module][2]);
    ROM_GPIOPinConfigure(ssi_gpio_config[ssi_module][3]);

    /* Configure the GPIO settings for SSI pins */
    ROM_GPIOPinTypeSSI(ssi_gpio_port[ssi_module], 
                       ssi_gpio_pin[ssi_module]);

    /* Set Clock Source for SSI */
    ROM_SSIClockSourceSet(base, SSI_CLOCK_SYSTEM);

    /* 
     * SPI set using SSI
     * Mode 0: Polariry 0 and Phase 0
     * Master Mode 
     * 10 Mhz communication speed
     * 8 bits data 
     */ 
    ROM_SSIConfigSetExpClk(base, 
                           SysCtlClockGet(), 
                           SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, 
                           25000000, 
                           8);

    /* Enable Interrupt for SSI */
    /* ROM_SSIIntDisable(base, 0xFFFFFFFF); */
    /* ROM_SSIIntEnable(base, SSI_TXFF | SSI_RXFF | SSI_RXTO); */
    /* ROM_IntEnable(ssi_int_map[ssi_module]); */
    
    /* Clear any residual data */
    unsigned long dummy_read_buffer[8];
    while(ROM_SSIDataGetNonBlocking(base, 
                                    &dummy_read_buffer[0]));

    /* Enable SSI module */
    ROM_SSIEnable(base);

}


/**
* @brief  Close SPI module
*/
static void spi_close(spi_instance_t spi_instance)
{
    /* SSI module map based on spi instance */
    uint8_t ssi_module = spi_index[spi_instance];

    ROM_SSIIntDisable(ssi_base[ssi_module], SSI_TXFF);
    ROM_SSIDisable(ssi_base[ssi_module]);
    SSIIntUnregister(ssi_base[ssi_module]);
}

static void spi_write(spi_instance_t spi_instance,
                      uint8_t        data)
{
    /* SSI module map based on spi instance */
    uint8_t ssi_module = spi_index[spi_instance];

    /* Write Data to SSI */
    ROM_SSIDataPut(ssi_base[ssi_module], (uint8_t)data);

    /* Get Data from SSI */
    unsigned long rx_data;
    ROM_SSIDataGet(ssi_base[ssi_module], &rx_data);
}


static void spi_write_non_blocking(spi_instance_t spi_instance,
                                   uint8_t        data)
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
void spi_init(spi_services_t        *spi_services,
              evl_services_t        *evl_services)
{
    static bool is_alloc = false;

    evl = evl_services;

    spi_services->open = spi_open;
    spi_services->close = spi_close;
    spi_services->write = spi_write;
    spi_services->write_non_blocking = spi_write_non_blocking;


    uint8_t i;
    
    for (i = 0; i < SPI_COUNT; i++)
    {
        /* Allocate callback for event loop */
        if (!is_alloc)
        {
            spi_state[i].evl_tx_handle = evl->cb_alloc(spi_tx_evl_cb, i);
        }
    }

    is_alloc = true;
}

