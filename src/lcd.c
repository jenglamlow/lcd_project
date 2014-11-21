/*
 * =====================================================================================
 *
 *       Filename:  lcd.c
 *
 *    Description:  LCD 240 x 320 module implementation file
 *
 *        Version:  1.0
 *        Created:  11/15/2014 11:00:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam (jeng), jenglamlow@gmail.com
 *   Organization:  Malaysia
 *
 * =====================================================================================
 */


/*-----------------------------------------------------------------------------
 *  Include
 *-----------------------------------------------------------------------------*/
/* Third party libraries include */

/* Local includes */
#include "lcd.h"

/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

static spi_services_t spi;


/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

static void lcd_hw_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_3,
                 0x00);

    /* GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_2); */
    /* GPIOPinWrite(GPIO_PORTE_BASE, */
    /*              GPIO_PIN_2, */
    /*              0x00); */
}

static void lcd_set_dc_pin(void)
{
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_3,
                 GPIO_PIN_3);
}


static void lcd_clear_dc_pin(void)
{
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_3,
                 0x00);
}

static void lcd_write_command(uint8_t data)
{
    lcd_clear_dc_pin();

    spi.write(SPI_LCD,data);
}

static void lcd_write_data(uint8_t data)
{
    lcd_set_dc_pin();

    spi.write(SPI_LCD,data);
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

static void lcd_open(void)
{
    lcd_hw_init();

}

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/
void lcd_init(lcd_services_t *lcd_services,
              spi_services_t *spi_services)
{
    /* lcd_services->start = lcd_start; */
    lcd_services->open = lcd_open;
    spi = *spi_services;
}

