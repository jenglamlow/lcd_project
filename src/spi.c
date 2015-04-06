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
#include "ringbuf.h"

/* Third party libraries include */
#include "driverlib/ssi.h"

/* Local includes */

/*-----------------------------------------------------------------------------
 *  Configurations
 *-----------------------------------------------------------------------------*/

#define RX_BUFFER_SIZE  16
#define TX_BUFFER_SIZE  1024

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/* SPI internal state */
typedef enum
{
    SPI_READY = 0,
    SPI_BUSY
} spi_state_t;


/* Info per SSI device */
typedef struct
{
    /* Internal State of SSI */
    spi_state_t state;

    /* Store SSI Instance */
    spi_instance_t instance;

    /* Receive Event Handle */
    evl_cb_handle_t evl_tx_handle;

#if USE_INTERRUPT
    /* Ring Buffer for TX & RX */
    tRingBufObject tx_ringbuf_obj;
    tRingBufObject rx_ringbuf_obj;

    /* Receive Buffer Array */
    uint8_t rx_buffer[RX_BUFFER_SIZE];

    /* Transmit Buffer Array */
    uint8_t tx_buffer[TX_BUFFER_SIZE];
#endif

    /* Client data available callback */
    spi_tx_cb_t tx_cb;

} spi_info_t;

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

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

/* SSI Interrupt */
static const uint32_t ssi_int[] =
{
    INT_SSI0,
    INT_SSI1,
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

/* Event Loop Services */
static evl_services_t *evl;

/* Per-SPI info */
static spi_info_t spi_info[SPI_COUNT];

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

/**
 * SPI Transmint compete callback
 * @param ix    SPI instance
 */
static void spi_tx_evl_cb(uint8_t ix)
{
    spi_info_t *info = &spi_info[ix];

    /* Callback to notify upper layer sending data completed */
    info->tx_cb();
}

/*-----------------------------------------------------------------------------
 *  IRQ Handler
 *-----------------------------------------------------------------------------*/
#if USE_INTERRUPT
/**
 * SPI Generic IRQ Handler
 * @param spi_instance  SPI instance
 */
static void spi_irq(spi_instance_t spi_instance)
{
    uint32_t status;

    spi_info_t *info = &spi_info[spi_instance];
    uint32_t base = ssi_base[spi_instance];
    unsigned long read_byte;
    uint8_t write_byte;
    uint8_t count;

    /* Get spi Status Flag */
    status = ROM_SSIIntStatus(ssi_base[spi_instance], true);

    /* Clear Interrupt source */
    ROM_SSIIntClear(ssi_base[spi_instance], status);

    /* If receive FIFO or receive timeout triggered */
    if (status & (SSI_RXFF | SSI_RXTO))
    {
        while (1)
        {
            /* TODO: current read_byte is dummy read */
            if (ROM_SSIDataGetNonBlocking(base, &read_byte) == 0)
            {
                /* Break loop if FIFO is empty */
                break;
            }
        }

        /* If nothing to transmit */
        if (RingBufEmpty(&info->tx_ringbuf_obj))
        {
            /* TODO: expected read scenario */
            /* If transmit completed, disable RX interrupt */
            SSIIntDisable(base, SSI_RXFF | SSI_RXTO);

            /* Set state to READY */
            info->state = SPI_READY;

            /* Schedule callback */
            evl->schedule(info->evl_tx_handle);
        }

    }

    /* If transmit FIFO triggered */
    if(status & SSI_TXFF)
    {
        for (count = 0; count < 4; count++)
        {
            /* Check if there is data to be transmitted */
            if (!RingBufEmpty(&info->tx_ringbuf_obj))
            {
                /* Read the available data from ring buffer */
                RingBufRead(&info->tx_ringbuf_obj, &write_byte, 1);

                if (SSIDataPutNonBlocking(base, write_byte) == 0)
                {
                    /* Break loop if FIFO is full */
                    break;
                }
            }
            /* TODO: tranmsit dummy data for expected read */
            /* No more data to be transmitted */
            else
            {
                /* Disable Transmit interrupt */
                ROM_SSIIntDisable(base, SSI_TXFF);

                break;
            }
        }
    }
}
#endif

/**
 * SSI0 Interrupt Handler
 */
void SSI0IntHandler(void)
{
#if USE_INTERRUPT
    spi_irq(SPI_TFT);
#endif
}

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/**
 * SPI initialise GPIO and hardware peripheral and register transmit complete
 * callback
 * @param spi_instance  SPI instance
 * @param spi_tx_cb     SPI transmit complete callback
 */
static void spi_open(spi_instance_t spi_instance,
                     spi_tx_cb_t    spi_tx_cb)
{
    ASSERT(spi_instance < SPI_COUNT);
    ASSERT(spi_tx_cb != NULL);

    uint32_t base = ssi_base[spi_instance];
    
    /* SSI Info Initialise */
    spi_info_t *info = &spi_info[spi_instance];

    /* spi Info Initialization */
    info->instance = spi_instance;

    /* Subscribe data available callback */
    info->tx_cb = spi_tx_cb;

    /* Initialise SSI peripheral */
    ROM_SysCtlPeripheralEnable(ssi_peripheral[spi_instance]);

    /* Disable SSI module */ 
    ROM_SSIDisable(base);
    
    /* Enable GPIO port */ 
    ROM_SysCtlPeripheralEnable(ssi_peripheral_gpio[spi_instance]);

    /* Configure PORT pin muxing as SSI peripheral function */  
    ROM_GPIOPinConfigure(ssi_gpio_config[spi_instance][0]);
    ROM_GPIOPinConfigure(ssi_gpio_config[spi_instance][1]);
    ROM_GPIOPinConfigure(ssi_gpio_config[spi_instance][2]);
    ROM_GPIOPinConfigure(ssi_gpio_config[spi_instance][3]);

    /* Configure the GPIO settings for SSI pins */
    ROM_GPIOPinTypeSSI(ssi_gpio_port[spi_instance],
                       ssi_gpio_pin[spi_instance]);

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

#if USE_INTERRUPT
    /* Enable Interrupt for SSI */
    ROM_SSIIntDisable(base, 0xFFFFFFFF);

    ROM_IntEnable(ssi_int[spi_instance]);
#endif
    
    /* Clear any residual data */
    unsigned long dummy_read_buffer[8];
    while(ROM_SSIDataGetNonBlocking(base, 
                                    &dummy_read_buffer[0]));

    /* Enable SSI module */
    ROM_SSIEnable(base);
}

/**
 * Close SPI Module
 * @param spi_instance  SPI instance
 */
static void spi_close(spi_instance_t spi_instance)
{
    ASSERT(spi_instance < SPI_COUNT);

    /* SSI module map based on spi instance */
    ROM_SSIIntDisable(ssi_base[spi_instance], SSI_TXFF);
    ROM_SSIDisable(ssi_base[spi_instance]);
}

/**
 * Write data to SPI (Blocking)
 * @param spi_instance  SPI instance
 * @param data          Write data byte
 */
static void spi_write(spi_instance_t spi_instance,
                      uint8_t        data)
{
    ASSERT(spi_instance < SPI_COUNT);

    uint32_t base = ssi_base[spi_instance];

    /* Wait until SPI is ready */
    while(SSIBusy(base));
    while(spi_info[spi_instance].state != SPI_READY);

    /* Write Data to SSI */
    ROM_SSIDataPut(base, (uint8_t)data);

    /* Get dummy data from SSI */
    unsigned long rx_data;
    ROM_SSIDataGet(base, &rx_data);
}


#if USE_INTERRUPT
/**
 * Write data to SPI (Non-Blocking)
 * @param spi_instance  SPI instance
 * @param data          Write data byte
 * @param data_size     Write data size
 */
static void spi_write_non_blocking(spi_instance_t spi_instance,
                                   uint8_t        *data,
                                   uint32_t       data_size)
{
    ASSERT(spi_instance < SPI_COUNT);
    ASSERT(data_size > 0);
    ASSERT(data != NULL);

    spi_info_t *info = &spi_info[spi_instance];
    uint32_t base = ssi_base[spi_instance];

    if (data_size > 0)
    {
        info->state = SPI_BUSY;

        /* Store the data into ring buffer */
        RingBufWrite(&info->tx_ringbuf_obj, data, data_size);

        /* Enable SSI interrupt */
        ROM_SSIIntEnable(base, SSI_TXFF | SSI_RXFF | SSI_RXTO);
    }
}
#endif

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

/**
 * SPI services initialisation
 * @param spi_services  SPI services
 * @param evl_services  Event loop services
 */
void spi_init(spi_services_t        *spi_services,
              evl_services_t        *evl_services)
{
    static bool is_alloc = false;

    evl = evl_services;

    /* SPI services initialisation */
    spi_services->open = spi_open;
    spi_services->close = spi_close;
    spi_services->write = spi_write;
#if USE_INTERRUPT
    spi_services->write_non_blocking = spi_write_non_blocking;
#endif

    uint8_t i;
    
    /* SPI internal info initialisation */
    for (i = 0; i < SPI_COUNT; i++)
    {
        spi_info[i].state = SPI_READY;

#if USE_INTERRUPT
        RingBufInit(&spi_info[i].tx_ringbuf_obj,
                    &spi_info[i].tx_buffer[0],
                    sizeof(spi_info[i].tx_buffer));

        RingBufInit(&spi_info[i].rx_ringbuf_obj,
                    &spi_info[i].rx_buffer[0],
                    sizeof(spi_info[i].rx_buffer));
#endif

        /* Allocate callback for event loop */
        if (!is_alloc)
        {
            spi_info[i].evl_tx_handle = evl->cb_alloc(spi_tx_evl_cb, i);
        }
    }

    is_alloc = true;
}
