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
/* LCD (ILI9341) size */
#define LCD_HEIGHT      240
#define LCD_WIDTH       320

#define MIN_X           0
#define MIN_Y           0
#define MAX_X           239
#define MAX_Y           319

/* pin mapping for D/C lcd */ 
#define DC_PIN_BASE         GPIO_PORTE_BASE
#define DC_PIN              GPIO_PIN_2

/* Set D/C pin to high */
#define SET_DC_PIN          SET_BITS(DC_PIN_BASE, DC_PIN)

/* Set D/C pin to low */
#define CLEAR_DC_PIN        CLEAR_BITS(DC_PIN_BASE, DC_PIN)

/* pin mapping for RST lcd */ 
#define RST_PIN_BASE        GPIO_PORTE_BASE
#define RST_PIN             GPIO_PIN_3

/* Set RST pin to high */
#define SET_RST_PIN         SET_BITS(RST_PIN_BASE, RST_PIN)

/* Set RST pin to low */
#define CLEAR_RST_PIN       CLEAR_BITS(RST_PIN_BASE, RST_PIN)


/*-----------------------------------------------------------------------------
 *  ILI9341 command list
 *-----------------------------------------------------------------------------*/

/* ILI9340 command */
#define SWRESET		0x01
#define	BSTRON		0x03
#define RDDIDIF		0x04
#define RDDST		0x09
#define SLEEPIN         0x10
#define	SLEEPOUT	0x11
#define	NORON		0x13
#define	INVOFF		0x20
#define INVON      	0x21
#define	SETCON		0x25
#define DISPOFF         0x28
#define DISPON          0x29
#define CASETP          0x2A
#define PASETP          0x2B
#define RAMWRP          0x2C
#define RGBSET	        0x2D
#define	MADCTL		0x36
#define SEP		0x37
#define	COLMOD		0x3A
#define DISCTR          0xB9
#define DOR		0xBA
#define	EC		0xC0
#define RDID1		0xDA
#define RDID2		0xDB
#define RDID3		0xDC

#define SETOSC		0xB0
#define SETPWCTR4	0xB4
#define SETPWCTR5	0xB5
#define SETEXTCMD	0xC1
#define SETGAMMAP	0xC2
#define SETGAMMAN	0xC3

// ILI9340 specific
#define ILIGS		0x26
#define ILIMAC		0x36
#define ILIFCNM		0xB1
#define ILIFCIM		0xB2
#define ILIFCPM		0xB3
#define ILIDFC		0xB6
#define ILIPC1		0xC0
#define ILIPC2		0xC1
#define ILIVC1		0xC5
#define ILIVC2		0xC7
#define PWRCTRLA	0xCB
#define PWRCTRLB	0xCF
#define RDID4		0xD3
#define GER4SPI		0xD9
#define ILIPGC		0xE0
#define ILINGC		0xE1
#define DTCTRLA1	0xE8
#define DTCTRLA2	0xE9
#define DTCTRLB		0xEA
#define POSC		0xED
#define ILIGFD		0xF2
#define PRC		0xF7

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

static spi_services_t   spi;

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

/**
 * @brief  Initialize LCD hardware setting 
 *         SPI initialization
 *         D/C & RST GPIO hardware initialization
 */
static void hw_init(void)
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

/**
 * @brief  LCD write command
 *
 * @param data: Command (8-bit) 
 */
static void send_command(uint8_t cmd)
{
    CLEAR_DC_PIN;

    spi.write(SPI_LCD, cmd);
}

/**
 * @brief  LCD write data
 *
 * @param data: data (8-bit)
 */
static void send_data(uint8_t data)
{
    SET_DC_PIN;

    spi.write(SPI_LCD, data);
}

/**
 * @brief  LCD write in word
 *
 * @param word: data (16-bit)
 */
static void send_word(uint16_t word)
{
    uint8_t high_byte = word>>8;
    uint8_t low_byte = word&0xff;

    SET_DC_PIN;

    spi.write(SPI_LCD, high_byte);
    spi.write(SPI_LCD, low_byte);
}

/**
 * @brief  LCD set column
 *
 * @param start_column: Starting position of the column
 * @param end_column: End position of the column
 */
static void set_column(uint16_t start_column,uint16_t end_column)
{
    send_command(CASETP);              /* Column Address Set */
    send_word(start_column);
    send_word(end_column);
}

/**
 * @brief  LCD set page
 *
 * @param StartPage: Starting position of the page
 * @param EndPage: End position of the page
 */
static void set_page(uint16_t StartPage,uint16_t EndPage)
{
    send_command(PASETP);              /* Page Address Set */
    send_word(StartPage);
    send_word(EndPage);
}

static void set_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    set_column(x0, x1);
    set_page(y0, y1);
    send_command(RAMWRP);              /* Memory Write */
}
/**
 * @brief  set starting position of x and y
 *
 * @param x: x coordinate
 * @param y: y coordinate
 */
static void set_xy(uint16_t x, uint16_t y)
{
    set_column(x, x);
    set_page(y, y);
    send_command(RAMWRP);              /* Memory Write */
}
/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/**
* @brief  Clear LCD screen to all black
*/
static void lcd_clear_screen(void)
{
    set_area(0, 0, (LCD_HEIGHT - 1), (LCD_WIDTH - 1));

    SET_DC_PIN;
    
    uint32_t total_pixel = (LCD_WIDTH * LCD_HEIGHT) / 2;

    for (uint16_t i=0; i<total_pixel; i++)
    {
        spi.write(SPI_LCD,0);
        spi.write(SPI_LCD,0);
        spi.write(SPI_LCD,0);
        spi.write(SPI_LCD,0);
    }
}


/**
 * @brief  LCD initialization (Hardware)
 */
static void lcd_open(void)
{
    /* Initialize Hardware I/O for LCD */
    hw_init();

    /* strawman transfer */
    spi.write(SPI_LCD,0);        

    /* Reset LCD Pin */
    CLEAR_RST_PIN;
    delay_ms(10);
    SET_RST_PIN;
    delay_ms(500);

    /* Software Reset */
    send_command(SWRESET);
    delay_ms(200);

    send_command(PWRCTRLA);        /* Power Control */
    send_data(0x39); 
    send_data(0x2C); 
    send_data(0x00); 
    send_data(0x34); 
    send_data(0x02); 

    send_command(PWRCTRLB);  
    send_data(0x00); 
    send_data(0XC1); 
    send_data(0X30); 

    send_command(DTCTRLA1);        /* Driver Timing Control */
    send_data(0x85); 
    send_data(0x00); 
    send_data(0x78); 

    send_command(DTCTRLB);  
    send_data(0x00); 
    send_data(0x00); 

    send_command(POSC);            /* Power On Sequence Control */
    send_data(0x64); 
    send_data(0x03); 
    send_data(0X12); 
    send_data(0X81); 

    send_command(PRC);             /* Pump Ratio Control */
    send_data(0x20); 

    send_command(ILIPC1);          /* power control */
    send_data(0x23);   	

    send_command(ILIPC2);    	
    send_data(0x10);   	

    send_command(ILIVC1);          /* VCOM Control */
    send_data(0x3e);   	
    send_data(0x28); 

    send_command(ILIVC2);    
    send_data(0x86);  	 

    send_command(MADCTL);    	/* Memory Access Control */
    send_data(0x48);  	        /* Refresh Order - BGR colour filter */

    send_command(COLMOD);          /* Pixel Format Set */
    send_data(0x55);               /* 16 bits/pixel */

    send_command(ILIFCNM);         /* Frame Rate Control */
    send_data(0x00);               /* Fosc */
    send_data(0x18);               /* 24 clocks for line period */

    send_command(ILIDFC);    	/* Display Function Control */
    send_data(0x08);               /* Interval Scan */
    send_data(0x82);               /* Normally Black , Scan cycle interval */
    send_data(0x27);  

    send_command(ILIGFD);    	/* 3Gamma Function */
    send_data(0x00);               /* Disable 3G */

    send_command(ILIGS);           /* Gamma Set */
    send_data(0x01);               /* Gamma curve */

    send_command(ILIPGC);    	/* Positive Gamma Correction */
    send_data(0x0F); 
    send_data(0x31); 
    send_data(0x2B); 
    send_data(0x0C); 
    send_data(0x0E); 
    send_data(0x08); 
    send_data(0x4E); 
    send_data(0xF1); 
    send_data(0x37); 
    send_data(0x07); 
    send_data(0x10); 
    send_data(0x03); 
    send_data(0x0E); 
    send_data(0x09); 
    send_data(0x00); 

    send_command(ILINGC);    	/* Negative Gamma Correction */
    send_data(0x00); 
    send_data(0x0E); 
    send_data(0x14); 
    send_data(0x03); 
    send_data(0x11); 
    send_data(0x07); 
    send_data(0x31); 
    send_data(0xC1); 
    send_data(0x48); 
    send_data(0x08); 
    send_data(0x0F); 
    send_data(0x0C); 
    send_data(0x31); 
    send_data(0x36); 
    send_data(0x0F); 

    send_command(SLEEPOUT);        /* Turn off Sleep Mode */
    delay_ms(120); 

    send_command(DISPON);          /* Display On */

    /* Memory Write - reset to Start Column/Start Page position */
    send_command(RAMWRP);          
    
    lcd_clear_screen();
}


/**
 * @brief  LCD set pixel to specific color
 *
 * @param x: X coordinate
 * @param y: Y coordinate
 * @param color: refer to COLOR macro
 */
static void lcd_set_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    set_xy(x,y);
    send_word(color);
}

/**
 * @brief Fill area (x0, y0) to (x1, y1) with colour 
 *
 * @param x0: Top left x coordinate
 * @param y0: Top left y coordinate
 * @param x1: Bottom right x coordinate
 * @param y1: Bottom right y coordinate
 * @param color: Refer color macro
 */
static void lcd_fill_area(uint16_t x0, uint16_t y0, 
                          uint16_t x1, uint16_t y1, 
                          uint16_t color)
{
    uint32_t xy=0;
    uint32_t i=0;

    /* Using XOR operator to swap both value */
    if(x0 > x1)
    {
        x0 = x0^x1;
        x1 = x0^x1;
        x0 = x0^x1;
    }

    if(y0 > y1)
    {
        y0 = y0^y1;
        y1 = y0^y1;
        y0 = y0^y1;
    }

    /* Constrain number to be within a range */
    x0 = constrain(x0, MIN_X,MAX_X);
    x1 = constrain(x1, MIN_X,MAX_X);
    y0 = constrain(y0, MIN_Y,MAX_Y);
    y1 = constrain(y1, MIN_Y,MAX_Y);

    /* get total area (pixels) */
    xy = (x1 - x0 + 1);
    xy = xy * (y1 - y0 + 1);

    /* Set Coordinate */
    set_area(x0, y0, x1, y1);
                        
    SET_DC_PIN;

    /* Start Filling area with color */
    uint8_t high_color = color >> 8;
    uint8_t low_color = color & 0xff;
    for(i=0; i < xy; i++)
    {
        spi.write(SPI_LCD, high_color);
        spi.write(SPI_LCD, low_color);
    }
}


/**
 * @brief  Draw rectangle with top left starting position (x,y) with length
 *         and width filled with color
 *
 * @param x: Top left x coordinate
 * @param y: Top left y coordinate
 * @param length: Length of the rectangle
 * @param width:  Width of the rectangle
 * @param color:  Refer color macro
 */
static void lcd_fill_rectangle(uint16_t x, uint16_t y, 
                               uint16_t length, uint16_t width, 
                               uint16_t color)
{
    lcd_fill_area(x, y, (x + length), (y + length), color);
}

/**
 * @brief  Draw Line from (x0, y0) to (x1, y1) with color
 *
 * @param x0: Starting point (x)
 * @param y0: Starting point (x)
 * @param x1: End Point (x)
 * @param y1: End Point (y)
 * @param color
 */
static void lcd_draw_line(uint16_t x0, uint16_t y0, 
                          uint16_t x1, uint16_t y1,
                          uint16_t color)
{
    int16_t x = x1-x0;
    int16_t y = y1-y0;
    int16_t dx = abs(x);
    int16_t sx = x0<x1 ? 1 : -1;
    int16_t dy = -abs(y);
    int16_t sy = y0<y1 ? 1 : -1;
    int16_t err = dx+dy;
    int16_t e2;

    uint8_t high_color = color >> 8;
    uint8_t low_color = color & 0xff;

    if (y0 == y1)           /* Horizontal */
    {
        set_area(x0, y0, x1, y1);
        while(dx-- > 0)
        {
            send_data(high_color);
            send_data(low_color);
        }
    }
    else if (x0 == x1)      /* Vertical */
    {
        dy = abs(dy);
        set_area(x0, y0, x1, y1);
        while(dy-- > 0)
        {
            send_data(high_color);
            send_data(low_color);
        }
    }
    else                   /* Angled */
    {
       while(1)
       {
           lcd_set_pixel(x0, y0, color);
           e2 = 2 * err;
           if (e2 >= dy)
           {
               if(x0 == x1)
                   break;
               err += dy;
               x0 += sx;
           }
           if (e2 <= dx)
           {
               if (y0 == y1)
                   break;
               err += dx;
               y0 += sy;
           }
       } 
    }
}

/**
 * @brief  Draw Rectangle Boundary without fill
 *
 * @param x: Top left x coordinate
 * @param y: Top left y coordinate
 * @param length: Length of the rectangle
 * @param width:  Width of the rectangle
 * @param color:  Refer color macro
 */
static void lcd_draw_rectangle(uint16_t x0, uint16_t y0, 
                               uint16_t x1, uint16_t y1,
                               uint16_t color)
{
    lcd_draw_line(x0, y0, x1, y0, color);
    lcd_draw_line(x0, y1, x1, y1, color);
    lcd_draw_line(x0, y0, x0, y1, color);
    lcd_draw_line(x1, y0, x1, y1, color);
}
/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

/**
 * @brief  LCD servise Initialization
 *
 * @param lcd_services: LCD component service
 * @param spi_services: SPI component service
 */
void lcd_init(lcd_services_t *lcd_services,
              spi_services_t *spi_services)
{
    lcd_services->clear_screen = lcd_clear_screen;
    lcd_services->open = lcd_open;
    lcd_services->fill_area = lcd_fill_area;
    lcd_services->fill_rectangle = lcd_fill_rectangle;
    lcd_services->draw_line = lcd_draw_line;
    lcd_services->draw_rectangle = lcd_draw_rectangle;

    /* SPI Component Services */
    spi = *spi_services;
}

