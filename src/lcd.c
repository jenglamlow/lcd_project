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

/* pin mapping for D/C lcd */ 
#define DC_PIN_BASE         GPIO_PORTE_BASE
#define DC_PIN              GPIO_PIN_2

/* Set D/C pin to high */
#define SET_DC_PIN          SET_BITS(DC_PIN_BASE, DC_PIN)

/* Set D/C pin to low */
#define CLEAR_DC_PIN        CLEAR_BITS(DC_PIN_BASE, DC_PIN)

/* pin mapping for RST lcd */ 
#define RST_PIN_BASE         GPIO_PORTE_BASE
#define RST_PIN              GPIO_PIN_3

/* Set RST pin to high */
#define SET_RST_PIN          SET_BITS(RST_PIN_BASE, RST_PIN)

/* Set RST pin to low */
#define CLEAR_RST_PIN        CLEAR_BITS(RST_PIN_BASE, RST_PIN)

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
    /* Enable PortE for RST & D/C PIN */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    /* Set D/C pin as output */
    GPIOPinTypeGPIOOutput(DC_PIN_BASE, DC_PIN);

    CLEAR_DC_PIN;

    /* Set RST pin as output */
    GPIOPinTypeGPIOOutput(RST_PIN_BASE, RST_PIN);

    CLEAR_RST_PIN;

    /* SPI module initialization */
    spi.open(SPI_LCD);
}

static void lcd_write_command(uint8_t data)
{
    CLEAR_DC_PIN;

    spi.write(SPI_LCD,data);
}

static void lcd_write_data(uint8_t data)
{
    SET_DC_PIN;

    spi.write(SPI_LCD,data);
}

void sendData(uint16_t data)
{
    uint8_t data1 = data>>8;
    uint8_t data2 = data&0xff;

    SET_DC_PIN;

    spi.write(SPI_LCD,data1);
    spi.write(SPI_LCD,data2);
}

void setCol(uint16_t StartCol,uint16_t EndCol)
{
    lcd_write_command(0x2A);                                                      /* Column Command address       */
    sendData(StartCol);
    sendData(EndCol);
}

void setPage(uint16_t StartPage,uint16_t EndPage)
{
    lcd_write_command(0x2B);                                                      /* Column Command address       */
    sendData(StartPage);
    sendData(EndPage);
}

void fillScreen(void)
{
    setCol(0, 239);
    setPage(0, 319);
    lcd_write_command(0x2c);                                                  /* start to write to display ra */

    SET_DC_PIN;

    for(uint16_t i=0; i<38400; i++)
    {
        spi.write(SPI_LCD,0x7f);
        spi.write(SPI_LCD,0x7f);
        spi.write(SPI_LCD,0x7f);
        spi.write(SPI_LCD,0x7f);
    }
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
    /* Initialize Hardware I/O for LCD */
    lcd_hw_init();

    /* strawman transfer */
    spi.write(SPI_LCD,0);        

    /* Reset LCD Pin */
    CLEAR_RST_PIN;
    delay_ms(10);
    SET_RST_PIN;
    delay_ms(500);

    /* Software Reset */
    lcd_write_command(SWRESET);
    delay_ms(200);

    lcd_write_command(PWRCTRLA);        /* Power Control */
    lcd_write_data(0x39); 
    lcd_write_data(0x2C); 
    lcd_write_data(0x00); 
    lcd_write_data(0x34); 
    lcd_write_data(0x02); 

    lcd_write_command(PWRCTRLB);  
    lcd_write_data(0x00); 
    lcd_write_data(0XC1); 
    lcd_write_data(0X30); 

    lcd_write_command(DTCTRLA1);        /* Driver Timing Control */
    lcd_write_data(0x85); 
    lcd_write_data(0x00); 
    lcd_write_data(0x78); 

    lcd_write_command(DTCTRLB);  
    lcd_write_data(0x00); 
    lcd_write_data(0x00); 

    lcd_write_command(POSC);            /* Power On Sequence Control */
    lcd_write_data(0x64); 
    lcd_write_data(0x03); 
    lcd_write_data(0X12); 
    lcd_write_data(0X81); 

    lcd_write_command(PRC);             /* Pump Ratio Control */
    lcd_write_data(0x20); 

    lcd_write_command(ILIPC1);          /* power control */
    lcd_write_data(0x23);   	

    lcd_write_command(ILIPC2);    	
    lcd_write_data(0x10);   	

    lcd_write_command(ILIVC1);          /* VCOM Control */
    lcd_write_data(0x3e);   	
    lcd_write_data(0x28); 

    lcd_write_command(ILIVC2);    
    lcd_write_data(0x86);  	 

    lcd_write_command(MADCTL);    	/* Memory Access Control */
    lcd_write_data(0x48);  	        /* Refresh Order - BGR colour filter */

    lcd_write_command(COLMOD);          /* Pixel Format Set */
    lcd_write_data(0x55);               /* 16 bits/pixel */

    lcd_write_command(ILIFCNM);         /* Frame Rate Control */
    lcd_write_data(0x00);               /* Fosc */
    lcd_write_data(0x18);               /* 24 clocks for line period */

    lcd_write_command(ILIDFC);    	/* Display Function Control */
    lcd_write_data(0x08);               /* Interval Scan */
    lcd_write_data(0x82);               /* Normally Black , Scan cycle interval */
    lcd_write_data(0x27);  

    lcd_write_command(ILIGFD);    	/* 3Gamma Function */
    lcd_write_data(0x00);               /* Disable 3G */

    lcd_write_command(ILIGS);           /* Gamma Set */
    lcd_write_data(0x01);               /* Gamma curve */

    lcd_write_command(ILIPGC);    	/* Positive Gamma Correction */
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

    lcd_write_command(ILINGC);    	/* Negative Gamma Correction */
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

    lcd_write_command(SLEEPOUT);        /* Turn off Sleep Mode */
    delay_ms(120); 

    lcd_write_command(DISPON);          /* Display On */

    /* Memory Write - reset to Start Column/Start Page position */
    lcd_write_command(RAMWRP);          
    fillScreen();
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

