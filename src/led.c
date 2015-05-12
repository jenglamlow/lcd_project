/*
 * =====================================================================================
 *
 *       Filename:  template.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/15/2014 01:51:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam
 *   Organization:  
 *
 * =====================================================================================
 */

/*-----------------------------------------------------------------------------
 *  Include
 *-----------------------------------------------------------------------------*/
/* Third party libraries include */
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"

/* Local includes */
#include "led.h"


/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

typedef enum
{
    LED_STATE_RED = 0,
    LED_STATE_RED_DELAY,
    LED_STATE_BLUE,
    LED_STATE_BLUE_DELAY,
} led_state_t;

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

static led_state_t state = LED_STATE_RED;
static uint32_t start_time;

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

void led_start(void)
{
    /* Enable and configure the GPIO port for the LED operation. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);
}

void led_task(void)
{
    uint32_t elapsed_time;

    switch (state)
    {
    case LED_STATE_RED:
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, RED_LED);
        start_time = HWREG(NVIC_ST_CURRENT);
        state = LED_STATE_RED_DELAY;
        break;

    case LED_STATE_RED_DELAY:
        elapsed_time = start_time-(HWREG(NVIC_ST_CURRENT) & 0x00FFFFFF);
        if (elapsed_time > 0x00FFFFFF)
            state = LED_STATE_BLUE;
        break;

    case LED_STATE_BLUE:
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, BLUE_LED);
        start_time = HWREG(NVIC_ST_CURRENT);
        state = LED_STATE_BLUE_DELAY;
        break;

    case LED_STATE_BLUE_DELAY:
        elapsed_time = start_time-(HWREG(NVIC_ST_CURRENT) & 0x00FFFFFF);
        if (elapsed_time > 0x00FFFFFF)
            state = LED_STATE_RED;
        break;
    }

}

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/
void led_init(void)
{

}

