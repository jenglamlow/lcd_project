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

/*----------------------------------------------------------------------------*/
/* Configuration                                                              */
/*----------------------------------------------------------------------------*/

/* Number of UART Driver */
#define UART_COUNT          (3U)

/* UART Transmit Buffer Size */
#define RX_BUFFER_SIZE      (512U)

/* UART Receive Buffer Size */
#define TX_BUFFER_SIZE      (512U)

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Private types                                                              */
/*----------------------------------------------------------------------------*/

/* State per UART device */
typedef struct {
    /* RX circular buffer - shared between ISR & Non-ISR */
    /* Receive Buffer Write Index */
    volatile uint32_t rx_write_ix;

    /* Receive Buffer Read Index */
    volatile uint32_t rx_read_ix;

    /* Receive Buffer Array */
    volatile uint8_t rx_buf[RX_BUFFER_SIZE];

    /* TX circular buffer - shared between ISR & Non-ISR */
    /* Transmit Buffer Write Index */
    volatile uint32_t tx_write_ix;

    /* Transmit Buffer Read Index */
    volatile uint32_t tx_read_ix;

    /* Transmit Buffer Array */
    volatile uint8_t tx_buf[TX_BUFFER_SIZE];

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
    const uart_state_t *state = &uart_state[ix];

    uint32_t rix = state->rx_read_ix;
    uint32_t wix = state->rx_write_ix;

    while (rix != wix)
    {
        /* Call component client to signal data available */
        state->data_available_cb();

        rix = state->rx_read_ix;
        wix = state->rx_write_ix;
    }

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
    state->rx_read_ix = 0;
    state->rx_write_ix = 0;
    state->tx_read_ix = 0;
    state->tx_write_ix = 0;
    state->data_available_cb = data_available_cb;

    /* Allocate event loop callback */

    /* Allocate event loop for overrun assertion */

    /* Enable NVIC setting */

    /* Enable UART */
}

static uint32_t uart_read(uart_instance_t   uart_instance,
                          uint8_t           *buffer,
                          uint32_t          buffer_size)
{
    ASSERT(buffer != 0);
    ASSERT(buffer_size > 0u);

    uint32_t read_count = 0u;

    if (((uint8_t)uart_instance < UART_COUNT) && (buffer != 0))
    {
        uart_state_t *state = &uart_state[uart_instance];

        /* Disable RX interrupt */

        uint32_t rix = state->rx_read_ix;
        while ((rix != state->rx_write_ix) &&
               (read_count < buffer_size))
        {
            *(buffer) = state->rx_buf[rix];
            buffer++;
            rix++;
            if (rix == RX_BUFFER_SIZE)
            {
                rix = 0;
            }
            read_count++;
        }
        state->rx_read_ix = rix;

        /* Enable the RX interrupt */
    }
    return read_count;
}

static uint32_t uart_write(uart_instance_t  uart_instance,
                           const uint8_t    *buffer,
                           uint32_t         buffer_size)
{
    ASSERT(buffer != 0);
    ASSERT(buffer_size > 0u);

    uint32_t write_count = 0u;

    if (((uint8_t)uart_instance < UART_COUNT) && (buffer != 0))
    {
        uart_state_t *state = &uart_state[uart_instance];

        /* Disable the transmit interrupt */

        uint32_t wix = state->tx_write_ix;
        uint32_t rix = state->tx_read_ix;
        uint32_t next_ix;
        while (write_count < buffer_size)
        {
            next_ix = wix + 1u;
            if (next_ix == TX_BUFFER_SIZE)
            {
                next_ix = 0;
            }
            if (next_ix == rix)
            {
                /* No more room in TX buffer */
                break;
            }
            state->tx_buf[wix] = *buffer;
            buffer++;
            wix = next_ix;
            write_count++;
        }
        state->tx_write_ix = wix;


        if (state->tx_restart)
        {
          state->tx_restart = false;
        }

        /* Enable the transmit interrupt */
    }
    return write_count;
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
    uint8_t i;
    for (i = 0; i < UART_COUNT; i++)
    {
        uart_state[i].rx_write_ix = 0;
        uart_state[i].rx_read_ix = 0;
        uart_state[i].tx_write_ix = 0;
        uart_state[i].tx_read_ix = 0;
        uart_state[i].tx_restart = true;
        uart_state[i].data_available_cb = NULL;
    }
}
