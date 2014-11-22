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
#include "lcd.h"

/*-----------------------------------------------------------------------------
 *  Configurations
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/
static spi_services_t spi;
static lcd_services_t lcd;

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
    lcd.open();
}
/**
 * @brief  Component services initialisation
 */
static void service_init(void)
{
    /* Initialize SPI Component */
    spi_init(&spi);
    lcd_init(&lcd, &spi);
}

/**
 * @brief  Initalize CPU clock speed
 */
static void cpu_clock_init(void)
{
    /* Set Clock to 80Mhz */
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 |
                       SYSCTL_USE_PLL |
                       SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
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

/*-----------------------------------------------------------------------------
 *  Main Routine
 *-----------------------------------------------------------------------------*/
int main()
{
    service_init();
    cpu_clock_init();
    peripheral_init();

    lcd.fill_area(10,10, 100, 100, BLUE);
    while(1)
    {
    }
}

