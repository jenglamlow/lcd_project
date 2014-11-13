/*
 * main.c
 *
 * Main Source file
 *
 */

/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/

/* Third party libraries include */
#include <stdint.h>
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

/* Local includes */

/*----------------------------------------------------------------------------*/
/* Configuration                                                              */
/*----------------------------------------------------------------------------*/

#define LED_RED GPIO_PIN_1
#define LED_BLUE GPIO_PIN_2
#define LED_GREEN GPIO_PIN_3

/*----------------------------------------------------------------------------*/
/* Private Types                                                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Private Data                                                               */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Helper Functions                                                           */
/*----------------------------------------------------------------------------*/

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

/*----------------------------------------------------------------------------*/
/* Event call-backs                                                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* IRQ Handlers                                                               */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/* Services                                                                   */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/* Main Routine                                                               */
/*----------------------------------------------------------------------------*/

int main()
{
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    unsigned long clock = SysCtlClockGet();
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_RED|LED_BLUE|LED_GREEN);

    while (1)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_RED);
        SysCtlDelay(5000000);
        GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, 0); 
        SysCtlDelay(5000000);
    }
}

