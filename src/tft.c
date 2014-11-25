/*
 * =====================================================================================
 *
 *       Filename:  tft.c
 *
 *    Description:  TFT 240 x 320 module implementation file
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
#include "tft.h"

/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/
/* TFT (ILI9341) size */
#define TFT_HEIGHT      240
#define TFT_WIDTH       320

#define MIN_X           0
#define MIN_Y           0
#define MAX_X           239
#define MAX_Y           319

/* pin mapping for D/C tft */ 
#define DC_PIN_BASE         GPIO_PORTE_BASE
#define DC_PIN              GPIO_PIN_2

/* Set D/C pin to high */
#define SET_DC_PIN          SET_BITS(DC_PIN_BASE, DC_PIN)

/* Set D/C pin to low */
#define CLEAR_DC_PIN        CLEAR_BITS(DC_PIN_BASE, DC_PIN)

/* pin mapping for RST tft */ 
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
#define SWRESET     0x01
#define BSTRON      0x03
#define RDDIDIF     0x04
#define RDDST       0x09
#define SLEEPIN     0x10
#define SLEEPOUT    0x11
#define NORON       0x13
#define INVOFF      0x20
#define INVON       0x21
#define SETCON      0x25
#define DISPOFF     0x28
#define DISPON      0x29
#define CASETP      0x2A
#define PASETP      0x2B
#define RAMWRP      0x2C
#define RGBSET      0x2D
#define MADCTL      0x36
#define SEP         0x37
#define COLMOD      0x3A
#define DISCTR      0xB9
#define DOR         0xBA
#define EC          0xC0
#define RDID1       0xDA
#define RDID2       0xDB
#define RDID3       0xDC

#define SETOSC      0xB0
#define SETPWCTR4   0xB4
#define SETPWCTR5   0xB5
#define SETEXTCMD   0xC1
#define SETGAMMAP   0xC2
#define SETGAMMAN   0xC3

// ILI9340 specific
#define ILIGS       0x26
#define ILIMAC      0x36
#define ILIFCNM     0xB1
#define ILIFCIM     0xB2
#define ILIFCPM     0xB3
#define ILIDFC      0xB6
#define ILIPC1      0xC0
#define ILIPC2      0xC1
#define ILIVC1      0xC5
#define ILIVC2      0xC7
#define PWRCTRLA    0xCB
#define PWRCTRLB    0xCF
#define RDID4       0xD3
#define GER4SPI     0xD9
#define ILIPGC      0xE0
#define ILINGC      0xE1
#define DTCTRLA1    0xE8
#define DTCTRLA2    0xE9
#define DTCTRLB     0xEA
#define POSC        0xED
#define ILIGFD      0xF2
#define PRC         0xF7

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
 * @brief  Initialize TFT hardware setting 
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
    spi.open(SPI_TFT);
}

/**
 * @brief  TFT write command
 *
 * @param data: Command (8-bit) 
 */
static void send_command(uint8_t cmd)
{
    CLEAR_DC_PIN;

    spi.write(SPI_TFT, cmd);
}

/**
 * @brief  TFT write data
 *
 * @param data: data (8-bit)
 */
static void send_data(uint8_t data)
{
    SET_DC_PIN;

    spi.write(SPI_TFT, data);
}

/**
 * @brief  TFT write in word
 *
 * @param word: data (16-bit)
 */
static void send_word(uint16_t word)
{
    uint8_t high_byte = word>>8;
    uint8_t low_byte = word&0xff;

    SET_DC_PIN;

    spi.write(SPI_TFT, high_byte);
    spi.write(SPI_TFT, low_byte);
}

/**
 * @brief  TFT set column
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
 * @brief  TFT set page
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
* @brief  Clear TFT screen to all black
*/
static void tft_clear_screen(void)
{
    set_area(0, 0, (TFT_HEIGHT - 1), (TFT_WIDTH - 1));

    SET_DC_PIN;
    
    uint32_t total_pixel = (TFT_WIDTH * TFT_HEIGHT) / 2;

    for (uint16_t i=0; i<total_pixel; i++)
    {
        spi.write(SPI_TFT,0);
        spi.write(SPI_TFT,0);
        spi.write(SPI_TFT,0);
        spi.write(SPI_TFT,0);
    }
}


/**
 * @brief  TFT initialization (Hardware)
 */
static void tft_open(void)
{
    /* Initialize Hardware I/O for TFT */
    hw_init();

    /* strawman transfer */
    spi.write(SPI_TFT,0);        

    /* Reset TFT Pin */
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

    send_command(MADCTL);       /* Memory Access Control */
    send_data(0x48);            /* Refresh Order - BGR colour filter */

    send_command(COLMOD);          /* Pixel Format Set */
    send_data(0x55);               /* 16 bits/pixel */

    send_command(ILIFCNM);         /* Frame Rate Control */
    send_data(0x00);               /* Fosc */
    send_data(0x18);               /* 24 clocks for line period */

    send_command(ILIDFC);       /* Display Function Control */
    send_data(0x08);               /* Interval Scan */
    send_data(0x82);               /* Normally Black , Scan cycle interval */
    send_data(0x27);  

    send_command(ILIGFD);       /* 3Gamma Function */
    send_data(0x00);               /* Disable 3G */

    send_command(ILIGS);           /* Gamma Set */
    send_data(0x01);               /* Gamma curve */

    send_command(ILIPGC);       /* Positive Gamma Correction */
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

    send_command(ILINGC);       /* Negative Gamma Correction */
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
    
    tft_clear_screen();
}


/**
 * @brief  TFT set pixel to specific color
 *
 * @param x: X coordinate
 * @param y: Y coordinate
 * @param color: refer to COLOR macro
 */
static void tft_set_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    set_xy(x,y);
    send_word(color);
}

/**
 * @brief  Added draw a coloured horizontal line API starting at (x,y) with length
 *
 * @param x: starting x coordinate
 * @param y: starting y coordinate
 * @param length: length of the line
 * @param color: Refer color macro
 */
static void tft_draw_horizontal_line(uint16_t x, uint16_t y, 
                                     uint16_t length,
                                     uint16_t color)
{
    set_column(x, (x + length));
    set_page(y, y);
    send_command(RAMWRP);              /* Memory Write */

    uint16_t i;
    for(i = 0; i < length; i++)
        send_word(color);
}

/**
 * @brief  Added draw a coloured vertical line API starting at (x,y) with length
 *
 * @param x: starting x coordinate
 * @param y: starting y coordinate
 * @param length: length of the line
 * @param color: Refer color macro
 */
static void tft_draw_vertical_line(uint16_t x, uint16_t y, 
                                   uint16_t length,
                                   uint16_t color)
{
    set_column(x, x);
    set_page(y, (y + length));
    send_command(RAMWRP);              /* Memory Write */

    uint16_t i;
    for(i = 0; i < length; i++)
        send_word(color);
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
static void tft_draw_line(uint16_t x0, uint16_t y0, 
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

    while(1)
    {
        tft_set_pixel(x0, y0, color);
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

/**
 * @brief Fill area (x0, y0) to (x1, y1) with colour 
 *
 * @param x0: Top left x coordinate
 * @param y0: Top left y coordinate
 * @param x1: Bottom right x coordinate
 * @param y1: Bottom right y coordinate
 * @param color: Refer color macro
 */
static void tft_fill_area(uint16_t x0, uint16_t y0, 
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
        spi.write(SPI_TFT, high_color);
        spi.write(SPI_TFT, low_color);
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
static void tft_fill_rectangle(uint16_t x, uint16_t y, 
                               uint16_t length, uint16_t width, 
                               uint16_t color)
{
    tft_fill_area(x, y, (x + length), (y + length), color);
}

static void tft_fill_circle(uint16_t xc, uint16_t yc, 
                            int16_t r,
                            uint16_t color)
{
    int16_t x = -r;
    int16_t y = 0;
    int16_t err = 2-2*r;
    int16_t e2;

    do 
    {
        tft_draw_vertical_line(xc-x, yc-y, 2*y, color);
        tft_draw_vertical_line(xc+x, yc-y, 2*y, color);

        e2 = err;
        if (e2 <= y) 
        {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x) 
                e2 = 0;
        }
        if (e2 > x) 
            err += ++x * 2 + 1;
    } while (x <= 0);

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
static void tft_draw_rectangle(uint16_t x0, uint16_t y0, 
                               uint16_t length, uint16_t width,
                               uint16_t color)
{
    tft_draw_horizontal_line(x0, y0, length, color);
    tft_draw_horizontal_line(x0, y0 + width, length, color);
    tft_draw_vertical_line(x0, y0, width, color);
    tft_draw_vertical_line(x0 + length, y0, width, color);
}


/**
* @brief  Draw Triangle based on Coordinate (x0, y0), (x1, y1) & (x2, y2)
*         without fill
*
* @param x0: first point (x-coordinate)
* @param y0: first point (y-coordinate)
* @param x1: second point (x-coordinate)
* @param y1: second point (y-coordinate)
* @param x2: third point (x-coordinate)
* @param y2: third point (y-coordinate)
* @param color
*/
void tft_draw_triangle(uint16_t x0, uint16_t y0, 
                       uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2,
                       uint16_t color)
{
    tft_draw_line(x0, y0, x1, y1,color);
    tft_draw_line(x0, y0, x2, y2,color);
    tft_draw_line(x1, y1, x2, y2,color);
}

/**
 * @brief  Draw circle using center point (xc, yc) with radius,r 
 *
 * @param xc: Center point (x-coordinate)
 * @param yc: Center point (y-coordinate)
 * @param r: Radius
 * @param color
 */
static void tft_draw_circle(uint16_t xc, uint16_t yc, 
                            uint16_t r,
                            uint16_t color)
{
    int16_t x = -r;
    int16_t y = 0;
    int16_t err = 2 - 2 * r;
    int16_t e2;
    
    do
    {
        tft_set_pixel(xc-x, yc+y, color);
        tft_set_pixel(xc+x, yc+y, color);
        tft_set_pixel(xc+x, yc-y, color);
        tft_set_pixel(xc-x, yc-y, color);
        e2 = err;
        if (e2 <= y)
        {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x)
                e2 = 0;
        }
        if (e2 > x)
            err += ++x * 2 + 1;
    } while (x <= 0);
}

/**
 * @brief  TFT sanity test by drawing several image 
 */
static void tft_test(void)
{
    tft_fill_area(0,0, 100, 100, BLUE);
    tft_fill_area(20,20, 80, 80, RED);
    tft_fill_rectangle(100, 100, 50, 50, GREEN);
    tft_draw_horizontal_line(0, 75, 240, WHITE);
    tft_draw_vertical_line(75, 0, 320, WHITE);
    tft_draw_line(0, 50, 240, 50, YELLOW);
    tft_draw_line(50, 0, 50, 320, CYAN);
    tft_draw_line(0, 0, 100, 100, GRAY1);
    tft_draw_rectangle(150,150,240,240,YELLOW);
    tft_draw_triangle(90,155,120,290,140,155, BLUE);
    tft_draw_circle(200, 50, 10, RED);
    tft_fill_circle(150, 50, 10, WHITE);

}
/*-----------------------------------------------------------------------------
 *  Initialization
 *-----------------------------------------------------------------------------*/

/**
 * @brief  TFT service Initialization
 *
 * @param tft_services: TFT component service
 * @param spi_services: SPI component service
 */
void tft_init(tft_services_t *tft_services,
              spi_services_t *spi_services)
{
    tft_services->clear_screen = tft_clear_screen;
    tft_services->open = tft_open;
    tft_services->fill_area = tft_fill_area;
    tft_services->fill_rectangle = tft_fill_rectangle;
    tft_services->fill_circle = tft_fill_circle;
    tft_services->draw_horizontal_line = tft_draw_horizontal_line;
    tft_services->draw_vertical_line = tft_draw_vertical_line;
    tft_services->draw_line = tft_draw_line;
    tft_services->draw_rectangle = tft_draw_rectangle;
    tft_services->draw_triangle = tft_draw_triangle;
    tft_services->draw_circle = tft_draw_circle;
    tft_services->test = tft_test;

    /* SPI Component Services */
    spi = *spi_services;
}


