/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Main Source File for LCD project 
 *
 *        Version:  1.0
 *        Created:  11/15/2014 02:05:36 PM
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

/* Third party libraries include */
#include "lib.h"

/* Local includes */
#include "spi.h"
#include "tft.h"
#include "uart.h"
#include "cmd_parser.h"

#include "uartstdio.h"
#include "cmdline.h"

/*-----------------------------------------------------------------------------
 *  Configurations
 *-----------------------------------------------------------------------------*/

#define UART_BUFFER_SIZE  (256U)

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

typedef enum
{
    STATE_IDLE = 0,
    STATE_BUSY,
    STATE_COUNT
} state_t;

typedef enum
{
    EVENT_RECEIVE_APP_DATA = 0,
    EVENT_LCD_DONE,
    EVENT_COUNT
} event_t;

typedef struct
{
    state_t state;
    event_t event;
    
    char uart_rx_buffer[UART_BUFFER_SIZE];
} main_info_t;

typedef void (*action_t)(main_info_t *info);

/* Structure for Transition Table */
typedef struct
{
    state_t    next_state;
    action_t   action;
} state_transition_t;

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/
/* Service Initialization */
static spi_services_t spi;
static tft_services_t tft;
static uart_services_t uart;
static cmd_parser_services_t cmd_parser;

static main_info_t main_info;

static void send_tft(main_info_t *info);
static void nop(main_info_t *info);

static const state_transition_t 
transition_table[STATE_COUNT][EVENT_COUNT] =
{
    /* STATE_IDLE */
    {
        /* EVENT_RECEIVE_APP_DATA, */  {STATE_BUSY,   send_tft},
        /* EVENT_LCD_DONE, */          {STATE_IDLE,   nop},
    },
    /* STATE_BUSY */
    {
        /* EVENT_RECEIVE_APP_DATA, */  {STATE_BUSY,   nop},
        /* EVENT_LCD_DONE, */          {STATE_IDLE,   nop},
    }
};

int process_cmd1(int argc, char *argv[]);
int process_cmd2(int argc, char *argv[]);
int process_cmd3(int argc, char *argv[]);

const tCmdLineEntry g_sCmdTable[] = 
{
    {"cmd1", process_cmd1, "cmd1 HelpFile"},
    {"cmd2", process_cmd2, "cmd2 HelpFile"},
    {"cmd3", process_cmd3, "cmd3 HelpFile"},
    {0, 0, 0}
};

int apa = 4;
/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

/* Error assertion definition when DEBUG is defined */
#ifdef DEBUG
void __error__(char *pcFilename, unsigned long ulLine)
{
  //
  // Something horrible happened! You need to look
  // at file "pcFilename" at line "ulLine" to see
  // what error is being reported.
  //
  while(1)
  {
  }
}
#endif

/**
 * @brief  Peripheral initialisation
 */
static void peripheral_init(void)
{
    tft.open();
}
/**
 * @brief  Component services initialisation
 */
static void service_init(void)
{
    /* Initialize SPI Component */
    spi_init(&spi);
    tft_init(&tft, &spi);
    uart_init(&uart);
    cmd_parser_init(&cmd_parser, &tft);
}

/**
 * @brief  Initalize CPU clock speed
 */
static void cpu_clock_init(void)
{
    /* Set System Clock to 80Mhz */
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 |
                       SYSCTL_USE_PLL |
                       SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    /* SysTick is set for delay_ms and delay_us */
    ROM_SysTickPeriodSet(F_CPU / SYSTICKHZ);
    ROM_SysTickEnable();
    ROM_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);
}

static void main_info_init(main_info_t *info)
{
    info->state = STATE_IDLE;
    info->event = EVENT_RECEIVE_APP_DATA;

    memset(&info->uart_rx_buffer[0], 0, UART_BUFFER_SIZE);
}

/* State Machine Helper Function */
static void update_state(main_info_t    *info,
                         state_t        state)
{
    ASSERT(info != NULL);
    ASSERT(state < STATE_COUNT);

    info->state = state;
}

static void update_event(main_info_t    *info,
                         event_t        event)
{
    ASSERT(info != NULL);
    ASSERT(event < EVENT_COUNT);

    info->event = event;
}

static void inject_event(main_info_t   *info,
                         event_t        event)
{
    state_t state = info->state;
    state_t next_state;
    
    /* Update Event */
    update_event(info, event);

    /* Get Next State from Transition Table and Update State */
    next_state = transition_table[state][event].next_state;
    update_state(info, next_state);

    transition_table[state][event].action(info);
}

int process_cmd1(int argc, char *argv[])
{
    UARTprintf("CMD1-OK %d\r\n", argc);

    tft.fill_rectangle(100, 100, 50, 50, GREEN);

    return 0;
}

int process_cmd2(int argc, char *argv[])
{
    UARTprintf("CMD2-OK %d\r\n", argc);

    tft.draw_string("abc", 0, 170, 3, BLACK, GREEN);

    return 0;
}
int process_cmd3(int argc, char *argv[])
{
    return 0;
}


static void nop(main_info_t *info)
{
    /* Do Nothing */
}

static void send_tft(main_info_t *info)
{
}

static void command_parser(main_info_t* info)
{
    int uart_rx_data_num = UARTRxBytesAvail();

    if (uart_rx_data_num > 0)
    {
        UARTgets(&info->uart_rx_buffer[0], uart_rx_data_num);
    }

    /* Parse the received UART data */
    int command_result;
    command_result = CmdLineProcess(&info->uart_rx_buffer[0]);

    if (command_result == CMDLINE_BAD_CMD)
        UARTprintf("Bad Command\r\n");
        /* inject_event(&main_info, EVENT_LCD_DONE); */
}

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  IRQ Handler
 *-------------------------)---------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Main Routine
 *-----------------------------------------------------------------------------*/
int main(void)
{
    apa  = 7;
    uint8_t read_byte;
    uint8_t write_data[] = "12345";

    main_info_init(&main_info);
    service_init();
    cpu_clock_init();
    peripheral_init();

    uart.open(UART_CMD);
    

    tft.test(); 
    
    while(1)
    {
        /* tft.running_animation(); */

#if 0
        if (UARTPeek('\r') != -1)
        {
            /* inject_event(&main_info, EVENT_RECEIVE_APP_DATA); */
            /* UARTIntDisable(UART1_BASE, UART_INT_RX); */
            command_parser(&main_info);
            /* UARTIntEnable(UART1_BASE, UART_INT_RX); */
        }
#endif

        while (uart.data_available(UART_CMD))
        {
            uart.read(UART_CMD, &read_byte, 1);

            cmd_parser.process(read_byte);
        }
        delay_ms(1000);
        /* uart.write(UART_CMD, &write_data[0], sizeof(write_data)); */
    }
}
