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

/* Third Party Library */
#include "inc/hw_ints.h"
#include "driverlib/uart.h"
#include "ringbuf.h"

/*----------------------------------------------------------------------------*/
/* Configuration                                                              */
/*----------------------------------------------------------------------------*/

/* Number of UART Driver */
#define UART_COUNT          (1U)

/* UART Transmit Buffer Size */
#define RX_BUFFER_SIZE      (512U)

/* UART Receive Buffer Size */
#define TX_BUFFER_SIZE      (512U)

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

static const uint32_t uart_base[3] = 
{
    UART0_BASE,
    UART1_BASE,
    UART2_BASE
};

static const uint32_t uart_int[3] = 
{
    INT_UART0,
    INT_UART1,
    INT_UART2
};

static const uint32_t uart_peripheral[3] =
{
    SYSCTL_PERIPH_UART0, 
    SYSCTL_PERIPH_UART1,
    SYSCTL_PERIPH_UART2
};

/*----------------------------------------------------------------------------*/
/* Private types                                                              */
/*----------------------------------------------------------------------------*/

/* State per UART device */
typedef struct {

    /* Struture encapsulating ring buffer for tx and rx */
    tRingBufObject tx_ringbuf_obj;
    tRingBufObject rx_ringbuf_obj;

    /* Store UART Instance */
    uart_instance_t instance;

    /* Receive Buffer Array */
    uint8_t rx_buffer[RX_BUFFER_SIZE];

    /* Transmit Buffer Array */
    uint8_t tx_buffer[TX_BUFFER_SIZE];

    /* Client data available callback */
    uart_data_available_cb_t data_available_cb;

} uart_state_t;

/*----------------------------------------------------------------------------*/
/* Private data                                                               */
/*----------------------------------------------------------------------------*/

/* Per-UART state */
static uart_state_t uart_state[UART_COUNT];

/*----------------------------------------------------------------------------*/
/* Helper functions                                                           */
/*----------------------------------------------------------------------------*/

/* Event loop UART receive callback function */
static void uart_rx_evl_cb(uint32_t ix)
{

}

static void uart_overrun_assertion_cb(void)
{
    /* Trigger Assertion */
}

static void uart_transmit(uart_state_t* state)
{
    uint32_t base = uart_base[state->instance];
    uint8_t write_byte;

    /* Check whether tx buffer contain any data */
    if (!RingBufEmpty(&state->tx_ringbuf_obj))
    {
        /* Disable UART interrupt */
        ROM_IntDisable(uart_int[state->instance]);

        while(ROM_UARTSpaceAvail(base) && !RingBufEmpty(&state->tx_ringbuf_obj))
        {
            RingBufRead(&state->tx_ringbuf_obj, &write_byte, 1);

            ROM_UARTCharPutNonBlocking(base, write_byte);
        }

        /* Enable UART interrupt */
        ROM_IntEnable(uart_int[state->instance]);
    }
}


/*----------------------------------------------------------------------------*/
/* IRQ handlers                                                               */
/*----------------------------------------------------------------------------*/

/* Generic UART RX IRQ handler */
static void uart_irq(uart_instance_t uart_instance)
{
    uint32_t status;
    uart_state_t *state = &uart_state[uart_instance];
    uint32_t base = uart_base[uart_instance];
    uint8_t read_byte;

    /* Get UART Status Flag */
    status = ROM_UARTIntStatus(uart_base[uart_instance], true);

    /* Clear Interrupt source */
    ROM_UARTIntClear(uart_base[uart_instance], status);

    /* RX Interrupt */
    if (status & UART_INT_RX)
    {
        /* Get all the available characters from the UART */
        while(ROM_UARTCharsAvail(base))
        {
            /* Read a character */
            read_byte = ROM_UARTCharGetNonBlocking(base);

            /* Check if ring buffer is full */
            if(!RingBufFull(&state->rx_ringbuf_obj))
            {
                /* Store read byte in ring buffer */
                RingBufWrite(&state->rx_ringbuf_obj, &read_byte, 1);
            }
        }
    }

    /* TX interrupt */
    if (status & UART_INT_TX)
    {
        /* Move as many byte into TX FIFO */
        uart_transmit(state);

        /* Disable transmit interrupt if tx buffer is empty */  
        if (!RingBufEmpty(&state->tx_ringbuf_obj))
        {
            ROM_UARTIntDisable(base, UART_INT_TX);
        }
    }
}

/*----------------------------------------------------------------------------*/
/* Services                                                                   */
/*----------------------------------------------------------------------------*/

static void uart_open(uart_instance_t          uart_instance)
{
    ASSERT(uart_instance < UART_COUNT);

    uart_state_t *state = &uart_state[uart_instance];
    uint32_t base = uart_base[uart_instance];
    
    /* UART State Initialization */
    state->instance = uart_instance;

    /* Allocate event loop callback */
    /* Allocate event loop for overrun assertion */

    /* IO initialisation */
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
    ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Enable UART Peripheral */
    ROM_SysCtlPeripheralEnable(uart_peripheral[uart_instance]);

    ROM_UARTConfigSetExpClk(base, SysCtlClockGet(), 115200,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_WLEN_8));
    
    /* Set UART FIFO Level */
    ROM_UARTFIFOLevelSet(base, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    /* UART Interrupt Setting */
    ROM_UARTIntDisable(base, 0xFFFFFFFF);
    ROM_UARTIntEnable(base, UART_INT_RX);
    ROM_IntEnable(uart_int[1]);

    /* Enable UART */
    ROM_UARTEnable(uart_base[1]);

    ROM_UARTClockSourceSet(uart_base[1], UART_CLOCK_SYSTEM);
}

static void uart_read(uart_instance_t   uart_instance,
                      uint8_t           *buffer,
                      uint32_t          buffer_size)
{
    ASSERT(buffer != 0);
    ASSERT(buffer_size > 0u);
    ASSERT(uart_instance < UART_COUNT);

    uint32_t read_count = 0u;

    uart_state_t *state = &uart_state[uart_instance];
    uint32_t base = uart_base[uart_instance];

    /* Disable RX interrupt */
    ROM_UARTIntDisable(base, UART_INT_RX);
    
    RingBufRead(&state->rx_ringbuf_obj, buffer, buffer_size);

    /* Enable the RX interrupt */
    ROM_UARTIntEnable(base, UART_INT_RX);
}

static void uart_write(uart_instance_t  uart_instance,
                       uint8_t          *buffer,
                       uint32_t         buffer_size)
{
    ASSERT(buffer != 0);
    ASSERT(buffer_size > 0u);
    ASSERT(uart_instance < UART_COUNT);

    uint32_t write_count = 0u;

    uart_state_t *state = &uart_state[uart_instance];
    uint32_t base = uart_base[uart_instance];

    /* Disable the transmit interrupt */
    ROM_UARTIntDisable(base, UART_INT_TX);

    RingBufWrite(&state->tx_ringbuf_obj, buffer, buffer_size);

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

    uart_write(UART_USB, (const uint8_t*)buf, size);
#endif
}

/*----------------------------------------------------------------------------*/
/* Initialisation                                                             */
/*----------------------------------------------------------------------------*/

void uart_init(uart_services_t          *uart_services)
{
    /* UART component initialization */
    uart_services->open = uart_open;
    uart_services->read = uart_read;
    uart_services->write = uart_write;
    uart_services->print = uart_print;

    uint8_t i;
    for (i = 0; i < UART_COUNT; i++)
    {
        RingBufInit(&uart_state[i].tx_ringbuf_obj, 
                    uart_state[i].tx_buffer,
                    sizeof(uart_state[i].tx_buffer));

        RingBufInit(&uart_state[i].rx_ringbuf_obj, 
                    uart_state[i].rx_buffer,
                    sizeof(uart_state[i].rx_buffer));

    }
}
