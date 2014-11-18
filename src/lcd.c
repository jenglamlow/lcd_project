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

static lcd_hw_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_3,
                 0x00);
}

static lcd_set_dc_pin(void)
{
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_3,
                 GPIO_PIN_3);
}


static lcd_clear_dc_pin(void)
{
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_3,
                 0x00);
}

static lcd_write_command(uint8_t data)
{
    lcd_clear_dc_pin();

    spi.write(SPI_LCD,data);
}

static lcd_write_data(uint8_t data)
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

    lcd_write_command(0xEF);
    lcd_write_data(0x03);
    lcd_write_data(0x80);
    lcd_write_data(0x02);

    lcd_write_command(0xCF);  
    lcd_write_data(0x00); 
    lcd_write_data(0XC1); 
    lcd_write_data(0X30); 

    lcd_write_command(0xED);  
    lcd_write_data(0x64); 
    lcd_write_data(0x03); 
    lcd_write_data(0X12); 
    lcd_write_data(0X81); 

    lcd_write_command(0xE8);  
    lcd_write_data(0x85); 
    lcd_write_data(0x00); 
    lcd_write_data(0x78); 

    lcd_write_command(0xCB);  
    lcd_write_data(0x39); 
    lcd_write_data(0x2C); 
    lcd_write_data(0x00); 
    lcd_write_data(0x34); 
    lcd_write_data(0x02); 

    lcd_write_command(0xF7);  
    lcd_write_data(0x20); 

    lcd_write_command(0xEA);  
    lcd_write_data(0x00); 
    lcd_write_data(0x00); 

    lcd_write_command(0xC0);    //Power control 
    lcd_write_data(0x23);   //VRH[5:0] 

    lcd_write_command(0xC1);    //Power control 
    lcd_write_data(0x10);   //SAP[2:0];BT[3:0] 

    lcd_write_command(0xC5);    //VCM control 
    lcd_write_data(0x3e); //�Աȶȵ���
    lcd_write_data(0x28); 

    lcd_write_command(0xC7);    //VCM control2 
    lcd_write_data(0x86);  //--

    lcd_write_command(0x36);    // Memory Access Control 
    lcd_write_data(0x40 | 0x08);

    lcd_write_command(0x3A);    
    lcd_write_data(0x55); 

    lcd_write_command(0xB1);    
    lcd_write_data(0x00);  
    lcd_write_data(0x18); 

    lcd_write_command(0xB6);    // Display Function Control 
    lcd_write_data(0x08); 
    lcd_write_data(0x82);
    lcd_write_data(0x27);  

    lcd_write_command(0xF2);    // 3Gamma Function Disable 
    lcd_write_data(0x00); 

    lcd_write_command(0x26);    //Gamma curve selected 
    lcd_write_data(0x01); 

    lcd_write_command(0xE0);    //Set Gamma 
    lcd_write_data(0x0F); 
    lcd_write_data(0x31); 
    lcd_write_data(0x2B); 
    lcd_write_data(0x0C); 
    lcd_write_data(0x0E); 
    lcd_write_data(0x08); 
    lcd_write_data(0x4E); 
    lcd_write_data(0xF1); 
    lcd_write_data(0x37); 
    lcd_write_data(0x07); 
    lcd_write_data(0x10); 
    lcd_write_data(0x03); 
    lcd_write_data(0x0E); 
    lcd_write_data(0x09); 
    lcd_write_data(0x00); 

   lcd_write_command(0xE0);    //Set Gamma 
    lcd_write_data(0x00); 
    lcd_write_data(0x0E); 
    lcd_write_data(0x14); 
    lcd_write_data(0x03); 
    lcd_write_data(0x11); 
    lcd_write_data(0x07); 
    lcd_write_data(0x31); 
    lcd_write_data(0xC1); 
    lcd_write_data(0x48); 
    lcd_write_data(0x08); 
    lcd_write_data(0x0F); 
    lcd_write_data(0x0C); 
    lcd_write_data(0x31); 
    lcd_write_data(0x36); 
    lcd_write_data(0x0F); 

    lcd_write_command(0x11);    //Exit Sleep 
    SysCtlDelay(120);           
    lcd_write_command(0x29);    //Display on 


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

