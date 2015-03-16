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

static const unsigned long uart_base[3] = 
{
    UART0_BASE,
    UART1_BASE,
    UART2_BASE
};

static const unsigned long uart_int[3] = 
{
    INT_UART0,
    INT_UART1,
    INT_UART2
};

static const unsigned long uart_peripheral[3] =
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

    /* Receive Buffer Array */
    uint8_t rx_buffer[RX_BUFFER_SIZE];

    /* Transmit Buffer Array */
    uint8_t tx_buffer[TX_BUFFER_SIZE];

    /* Flag to indicate whether transmit operation is restart/completed */
    volatile bool tx_restart;

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

static void uart_overrun_assertion_cb(uint32_t ix)
{
    /* Trigger Assertion */
}


/*----------------------------------------------------------------------------*/
/* IRQ handlers                                                               */
/*----------------------------------------------------------------------------*/

/* Generic UART RX IRQ handler */
static void uart_irq(uart_instance_t uart_instance)
{
    uart_state_t *state = &uart_state[uart_instance];

    /* Get UART Status Flag */

    /* Check for overrun error */

    /* check rx or tx interrupt */
    /* RXNE */
#if 0
    if ((uart_sr_reg & HWA_USART_SR_RXNE_BIT) > 0u)
    {
        /* Get write index to the RX buffer and fill with available data */
        uint32_t wix = state->rx_write_ix;

        state->rx_buf[wix] = (uint8_t)hwa->read_reg32(dev, HWA_USART_DR_OFFSET);
        wix++;
        if (wix == RX_BUFFER_SIZE)
        {
            wix = 0;
        }
        state->rx_write_ix = wix;

        /* Schedule a callback */
        evl->schedule(state->evl_rx.handle);
    }

    /* TXE */
    if ((uart_sr_reg & HWA_USART_SR_TXE_BIT) > 0u)
    {
        uint32_t wix = state->tx_write_ix;
        uint32_t rix = state->tx_read_ix;

        if (rix!= wix)
        {
            set_rts_bit(uart_instance, true);

            hwa->write_reg32(dev,
                            HWA_USART_DR_OFFSET,
                            state->tx_buf[rix]);
            rix++;
            if (rix == TX_BUFFER_SIZE)
            {
                rix = 0;
            }

            state->tx_restart = false;
            state->tx_read_ix = rix;
        }
        else
        {
            state->tx_restart = true;

            /* Disable TX interrupt */
            hwa->clear_bits_reg32(dev,
                                 HWA_USART_CR1_OFFSET,
                                 HWA_USART_CR1_TXEIE_BIT);
        }
    }
#endif
}

/*----------------------------------------------------------------------------*/
/* Services                                                                   */
/*----------------------------------------------------------------------------*/

static void uart_open(uart_instance_t          uart_instance,
                      uart_data_available_cb_t data_available_cb)
{
    uart_state_t *state = &uart_state[uart_instance];

    /* Initialize the state */
    state->data_available_cb = data_available_cb;

    /* Allocate event loop callback */

    /* Allocate event loop for overrun assertion */

    /* IO initialisation */
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
    ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Enable UART Peripheral */
    ROM_SysCtlPeripheralEnable(uart_peripheral[1]);

    ROM_UARTConfigSetExpClk(uart_base[1], SysCtlClockGet(), 115200,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_WLEN_8));
    
    ROM_UARTClockSourceSet(UART1_BASE, UART_CLOCK_SYSTEM);

    /* Enable NVIC setting */

    /* Enable UART */
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

    /* Disable RX interrupt */

    
    RingBufRead(&state->rx_ringbuf_obj, buffer, buffer_size);

    /* Enable the RX interrupt */
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

    /* Disable the transmit interrupt */

    RingBufWrite(&state->tx_ringbuf_obj, buffer, buffer_size);

    if (state->tx_restart)
    {
        state->tx_restart = false;
    }

    /* Enable the transmit interrupt */
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

        uart_state[i].tx_restart = false;
    }
}
