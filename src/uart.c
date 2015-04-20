/*
 * =====================================================================================
 *
 *       Filename:  uart.c
 *
 *    Description:  Implementation file for UART component
 *
 *        Version:  1.0
 *        Created:  03/10/2015 07:32:15 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam (jeng), jenglamlow@gmail.com
 *   Organization:  Malaysia
 *
 * =====================================================================================
 */

/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/

/* Local includes */
#include "uart.h"
#include "ringbuf.h"

/* Third Party Library */
/* #include <stdarg.h> */
/* #include <stdio.h> */
#include "inc/hw_ints.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"

/*----------------------------------------------------------------------------*/
/* Configuration                                                              */
/*----------------------------------------------------------------------------*/

/* Number of UART Driver */
#define UART_COUNT          (3U)

/* UART Transmit Buffer Size */
#define RX_BUFFER_SIZE      (4096U)

/* UART Receive Buffer Size */
#define TX_BUFFER_SIZE      (512U)

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

/* UART base map */
static const uint32_t uart_base[UART_COUNT] = 
{
    UART0_BASE,
    UART1_BASE,
    UART2_BASE
};

/* UART interrupt map */
static const uint32_t uart_int[UART_COUNT] = 
{
    INT_UART0,
    INT_UART1,
    INT_UART2
};

/* UART peripheral map */
static const uint32_t uart_peripheral[UART_COUNT] =
{
    SYSCTL_PERIPH_UART0, 
    SYSCTL_PERIPH_UART1,
    SYSCTL_PERIPH_UART2
};

/*----------------------------------------------------------------------------*/
/* Private types                                                              */
/*----------------------------------------------------------------------------*/

/* Info per UART device */
typedef struct {

    /* Struture encapsulating ring buffer for tx and rx */
    tRingBufObject tx_ringbuf_obj;
    tRingBufObject rx_ringbuf_obj;

    /* Store UART Instance */
    uart_instance_t instance;

    /* UART receive flag */
    volatile bool rx_flag;

    /* Client data available callback */
    uart_data_available_cb_t data_available_cb;

} uart_info_t;

/*----------------------------------------------------------------------------*/
/* Private data                                                               */
/*----------------------------------------------------------------------------*/

/* Per-UART info */
static uart_info_t uart_info[UART_COUNT];

/* UART1 Receive Buffer Array */
uint8_t uart1_rx_buffer[RX_BUFFER_SIZE];

/* UART1 Transmit Buffer Array */
uint8_t uart1_tx_buffer[TX_BUFFER_SIZE];

/*----------------------------------------------------------------------------*/
/* Helper functions                                                           */
/*----------------------------------------------------------------------------*/

/**
 * @brief   UART transmit data and put into UART FIFO
 * @param   info    UART info
 */
static void uart_transmit(uart_info_t* info)
{
    uint32_t base = uart_base[info->instance];
    uint8_t write_byte;

    /* Check whether tx buffer contain any data */
    if (!RingBufEmpty(&info->tx_ringbuf_obj))
    {
        /* Disable UART interrupt */
        ROM_IntDisable(uart_int[info->instance]);

        while(ROM_UARTSpaceAvail(base) && !RingBufEmpty(&info->tx_ringbuf_obj))
        {
            RingBufRead(&info->tx_ringbuf_obj, &write_byte, 1);

            ROM_UARTCharPutNonBlocking(base, write_byte);
        }

        /* Enable UART interrupt */
        ROM_IntEnable(uart_int[info->instance]);
    }
}

/**
 * @brief   Return the number of data in the buffer
 * @param   uart_instance   UART Instance
 * @return  Number of data in the buffer
 */
static uint32_t uart_data_available(uart_instance_t uart_instance)
{
    ASSERT(uart_instance < UART_COUNT);

    uart_info_t *info = &uart_info[uart_instance];

    return RingBufUsed(&info->rx_ringbuf_obj);
}

/*----------------------------------------------------------------------------*/
/* Event Callback Function                                                    */
/*----------------------------------------------------------------------------*/

/**
 * @brief   UART receive task
 * @param   ix      UART instance
 */
static void uart_rx_task(uint8_t ix)
{
    uart_info_t *info = &uart_info[ix];

    if (uart_data_available(info->instance))
    {
        info->data_available_cb();
    }

    /* Reschedule the uart data available event if still contains data */
    if (uart_data_available(info->instance))
    {
        /* Ensure atomic process */
        IntMasterDisable();

        info->rx_flag = true;

        IntMasterEnable();
    }
    else
    {
        /* Ensure atomic process */
        IntMasterDisable();

        info->rx_flag = false;

        IntMasterEnable();
    }
}

/*----------------------------------------------------------------------------*/
/* IRQ handlers                                                               */
/*----------------------------------------------------------------------------*/

/**
 * @brief   Generic IRQ handler for UART
 * @param   uart_instance  UART Instance
 */
static void uart_irq(uart_instance_t uart_instance)
{
    uint32_t status;
    uart_info_t *info = &uart_info[uart_instance];
    uint32_t base = uart_base[uart_instance];
    uint8_t read_byte;

    /* Get UART Status Flag */
    status = ROM_UARTIntStatus(uart_base[uart_instance], true);

    /* Clear Interrupt source */
    ROM_UARTIntClear(uart_base[uart_instance], status);

    /* RX Interrupt */
    if (status & (UART_INT_RX | UART_INT_RT))
    {
        /* Get all the available characters from the UART */
        while(ROM_UARTCharsAvail(base))
        {
            /* Read a character */
            read_byte = ROM_UARTCharGetNonBlocking(base);

            /* Check if ring buffer is full */
            if(!RingBufFull(&info->rx_ringbuf_obj))
            {
                /* Store read byte in ring buffer */
                RingBufWrite(&info->rx_ringbuf_obj, &read_byte, 1);
            }
        }

        /* Schedule Receive Event */
        info->rx_flag = true;

    }

    /* TX interrupt */
    if (status & UART_INT_TX)
    {
        /* Move as many byte into TX FIFO */
        uart_transmit(info);

        /* Disable transmit interrupt if tx buffer is empty */  
        if (!RingBufEmpty(&info->tx_ringbuf_obj))
        {
            ROM_UARTIntDisable(base, UART_INT_TX);
        }
    }
}

/**
 * @brief   UART1 IRQ Handler
 */
void UART1IntHandler(void)
{
    uart_irq(UART_CMD);
}

/*----------------------------------------------------------------------------*/
/* Services                                                                   */
/*----------------------------------------------------------------------------*/

/**
 * @brief   Open UART module
 * @param   uart_instance           UART Instance
 * @param   uart_data_available_cb  UART callback when data available
 */
void uart_open(uart_instance_t          uart_instance,
               uart_data_available_cb_t uart_data_available_cb)
{
    ASSERT(uart_instance < UART_COUNT);
    ASSERT(uart_data_available_cb != NULL);

    uart_info_t *info = &uart_info[uart_instance];
    uint32_t base = uart_base[uart_instance];
    
    /* UART info Initialization */
    info->instance = uart_instance;

    /* Subscribe uart data available callback */
    info->data_available_cb = uart_data_available_cb;

    /* IO initialisation */
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
    ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Enable UART Peripheral */
    ROM_SysCtlPeripheralEnable(uart_peripheral[uart_instance]);

    ROM_UARTConfigSetExpClk(base, SysCtlClockGet(), 460800,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_WLEN_8));
    
    /* Set UART FIFO Level */
    ROM_UARTFIFOLevelSet(base, UART_FIFO_TX7_8, UART_FIFO_RX7_8);

    /* UART Interrupt Setting */
    ROM_UARTIntDisable(base, 0xFFFFFFFF);
    ROM_UARTIntEnable(base, UART_INT_RX | UART_INT_RT);
    ROM_IntEnable(uart_int[uart_instance]);

    /* Enable FIFO */
    ROM_UARTFIFOEnable(base);

    /* Enable UART */
    ROM_UARTEnable(base);

    /* Setting clock source for uart */
    ROM_UARTClockSourceSet(base, UART_CLOCK_SYSTEM);
}

/**
 * @brief   Close UART module
 * @param   uart_instance   UART Instance
 */
void uart_close(uart_instance_t uart_instance)
{
    ASSERT(uart_instance < UART_COUNT);

    uart_info_t *info = &uart_info[uart_instance];
    uint32_t base = uart_base[uart_instance];

    /* Disable FIFO */
    ROM_UARTFIFODisable(base);

    /* Disable UART INT */
    ROM_UARTIntDisable(base, 0xFFFFFFFF);

    /* Disable UART */
    ROM_UARTDisable(base);

    /* Empty ring buffer */
    RingBufFlush(&info->rx_ringbuf_obj);
    RingBufFlush(&info->tx_ringbuf_obj);
}

/**
 * @brief   Read UART data and store into buffer with size of buffer_size
 * @param   uart_instance   UART Instance
 * @param   buffer          Pointer to data array
 * @param   buffer_size     Size of data to be read
 */
void uart_read(uart_instance_t   uart_instance,
               uint8_t           *buffer,
               uint32_t          buffer_size)
{
    ASSERT(buffer != 0);
    ASSERT(buffer_size > 0u);
    ASSERT(uart_instance < UART_COUNT);

    uint32_t read_count = 0u;

    uart_info_t *info = &uart_info[uart_instance];
    uint32_t base = uart_base[uart_instance];

    /* Disable RX interrupt */
    ROM_UARTIntDisable(base, UART_INT_RX);
    
    RingBufRead(&info->rx_ringbuf_obj, buffer, buffer_size);

    /* Enable the RX interrupt */
    ROM_UARTIntEnable(base, UART_INT_RX);
}

/**
 * @brief   Write array with buffer_size to UART based on uart_instance
 * @param   uart_instance   UART Instance
 * @param   buffer          Pointer to data array
 * @param   buffer_size     Size of data to be written
 */
void uart_write(uart_instance_t  uart_instance,
                uint8_t          *buffer,
                uint32_t         buffer_size)
{
    ASSERT(buffer != 0);
    ASSERT(buffer_size > 0u);
    ASSERT(uart_instance < UART_COUNT);

    uint32_t write_count = 0u;

    uart_info_t *info = &uart_info[uart_instance];
    uint32_t base = uart_base[uart_instance];

    /* Disable the transmit interrupt */
    ROM_UARTIntDisable(base, UART_INT_TX);

    RingBufWrite(&info->tx_ringbuf_obj, buffer, buffer_size);
    uart_transmit(info);

    /* Enable the transmit interrupt */
    ROM_UARTIntEnable(base, UART_INT_TX);
}


/* Print function similar as C printf - for debugging */
void uart_print(const char *fmt, ...)
{
#if 0
    static char buf[TX_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uint32_t size = strlen(buf);

    uart_write(UART_CMD, buf, size);
#endif
}

/**
 * @brief   UART task (For task scheduler)
 */
void uart_task(void)
{
    uint8_t ix;
    uart_info_t *info;

    for(ix = 0; ix < UART_COUNT; ix++)
    {
        info = &uart_info[ix];

        /* Check whether there is data received */
        if(info->rx_flag)
        {
            /* Invoke UART receive task */
            uart_rx_task(ix);
        }
    }
}

/*----------------------------------------------------------------------------*/
/* Initialisation                                                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief   UART Initialisation
 */
void uart_init(void)
{
    uint8_t i;

    for (i = 0; i < UART_COUNT; i++)
    {
        uart_info[i].rx_flag = false;
    }

    RingBufInit(&uart_info[UART_CMD].tx_ringbuf_obj,
                &uart1_tx_buffer[0],
                sizeof(uart1_tx_buffer));

    RingBufInit(&uart_info[UART_CMD].rx_ringbuf_obj,
                &uart1_rx_buffer[0],
                sizeof(uart1_rx_buffer));
}
