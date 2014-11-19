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

    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_2,
                 0x00);
}

static void lcd_set_rst_pin(void)
{
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_2,
                 GPIO_PIN_2);
}

static void lcd_clear_rst_pin(void)
{
    GPIOPinWrite(GPIO_PORTE_BASE,
                 GPIO_PIN_2,
                 0x00);
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

uint8_t colorLowByte = 0;
uint8_t colorHighByte = 0;
uint8_t bgColorLowByte = 0;
uint8_t bgColorHighByte = 0;


void setArea(unsigned int xStart, unsigned int yStart, unsigned int xEnd, unsigned int yEnd) {

    unsigned char yStartMSB = yStart >> 8;
    unsigned char yEndMSB = yEnd >> 8;
    unsigned char xStartMSB = xStart >> 8;
    unsigned char xEndMSB = xEnd >> 8;

    lcd_write_command(0x2A);

    lcd_write_data(xStartMSB);
    lcd_write_data(xStart);

    lcd_write_data(xEndMSB);
    lcd_write_data(xEnd);

    lcd_write_command(0x2B);

    lcd_write_data(yStartMSB);
    lcd_write_data(yStart);

    lcd_write_data(yEndMSB);
    lcd_write_data(yEnd);

    lcd_write_command(0x2C);
    // data to follow
}

void setColor(uint16_t color) {
    colorLowByte = color;
    colorHighByte = color >> 8;
}

void setBackgroundColor(uint16_t color) {
    bgColorLowByte = color;
    bgColorHighByte = color >> 8;
}


void clearScreen(uint8_t blackWhite) {
    setArea(0, 0, 240 - 1, 320 - 1);
    setBackgroundColor(blackWhite ? 0x0000 : 0xFFFF);
    uint16_t w = 240;
    uint16_t h = 320;
    while (h != 0) {
        while (w != 0) {
            lcd_write_data(bgColorHighByte);
            lcd_write_data(bgColorLowByte);
            w--;
        }
        w = 320;
        h--;
    }
}

void window (unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    lcd_write_command(0x2A);
    lcd_write_data(x >> 8);
    lcd_write_data(x);
    lcd_write_data((x+w-1) >> 8);
    lcd_write_data(x+w-1);
    
    lcd_write_command(0x2B);
    lcd_write_data(y >> 8);
    lcd_write_data(y);
    lcd_write_data((y+h-1) >> 8);
    lcd_write_data(y+h-1);
}

void WindowMax (void)
{
    window (0, 0, 320,  240);
}

const unsigned char font_5x7[][5] = {       // basic font
     {0x00, 0x00, 0x00, 0x00, 0x00} // 20
    ,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
    ,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
    ,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
    ,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
    ,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
    ,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
    ,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
    ,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
    ,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
    ,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
    ,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
    ,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
    ,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
    ,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
    ,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
    ,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
    ,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
    ,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
    ,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
    ,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
    ,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
    ,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
    ,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
    ,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
    ,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
    ,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
    ,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
    ,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
    ,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
    ,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
    ,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
    ,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
    ,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
    ,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
    ,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
    ,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
    ,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
    ,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
    ,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
    ,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
    ,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
    ,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
    ,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
    ,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
    ,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
    ,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
    ,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
    ,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
    ,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
    ,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
    ,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
    ,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
    ,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
    ,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
    ,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
    ,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
    ,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
    ,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
    ,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
    ,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c ¥
    ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
    ,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
    ,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
    ,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
    ,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
    ,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
    ,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
    ,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
    ,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
    ,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
    ,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
    ,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
    ,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
    ,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j
    ,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
    ,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
    ,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
    ,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
    ,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
    ,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
    ,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
    ,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
    ,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
    ,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
    ,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
    ,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
    ,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
    ,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
    ,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
    ,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
    ,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
    ,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
    ,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
    ,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e ~
    ,{0x00, 0x06, 0x09, 0x09, 0x06} // 7f Deg Symbol
};

void drawPixel(uint16_t x, uint16_t y) {
    setArea(x, y, x, y);
    lcd_write_data(colorHighByte);
    lcd_write_data(colorLowByte);
}

void drawCharSm(uint16_t x, uint16_t y, char c) {
    uint8_t col = 0;
    uint8_t row = 0;
    uint8_t bit = 0x01;
    uint8_t oc = c - 0x20;
    while (row < 8) {
        while (col < 5) {
            if (font_5x7[oc][col] & bit)
                drawPixel(x + col, y + row);
            col++;
        }
        col = 0;
        bit <<= 1;
        row++;
    }
}

void drawString(uint16_t x, uint16_t y, char type, char *string) {
    uint16_t xs = x;

    while (*string) {
        drawCharSm(xs, y, *string++);
        xs += 6;
    }
}

uint8_t _orientation = 0;
void setOrientation(uint8_t orientation) {

    lcd_write_command(0x36);

    switch (orientation) {
    case 1:
        lcd_write_data(0xE8);
        _orientation = 1;
        break;
    case 2:
        lcd_write_data(0x88);
        _orientation = 2;
        break;
    case 3:
        lcd_write_data(0x28);
        _orientation = 3;
        break;
    default:
        lcd_write_data(0x48);
        _orientation = 0;
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

static void delay_ms1(int del)     //generates delay in milliseconds
{
del = (SysCtlClockGet()/3.0)*del/1000.0;
SysCtlDelay(del);
}
static void lcd_open(void)
{
    lcd_hw_init();

#if 0
    lcd_write_command(0xCB);
    lcd_write_data(0x39);
    lcd_write_data(0x2C);
    lcd_write_data(0x00);
    lcd_write_data(0x34);
    lcd_write_data(0x02);

    lcd_write_command(0xCF);
    lcd_write_data(0x00);
    lcd_write_data(0XC1);
    lcd_write_data(0X30);

    lcd_write_command(0xE8);
    lcd_write_data(0x85);
    lcd_write_data(0x00);
    lcd_write_data(0x78);

    lcd_write_command(0xEA);
    lcd_write_data(0x00);
    lcd_write_data(0x00);

    lcd_write_command(0xED);
    lcd_write_data(0x64);
    lcd_write_data(0x03);
    lcd_write_data(0X12);
    lcd_write_data(0X81);

    lcd_write_command(0xF7);
    lcd_write_data(0x20);

    lcd_write_command(0xC0);
    lcd_write_data(0x23);
    lcd_write_command(0xC1);
    lcd_write_data(0x10);
    lcd_write_command(0xC5);
    lcd_write_data(0x3e);
    lcd_write_data(0x28);
    lcd_write_command(0xC7);
    lcd_write_data(0x86);

    setOrientation(0);

    lcd_write_command(0x3A);
    lcd_write_data(0x55);

    lcd_write_command(0xB1);
    lcd_write_data(0x00);
    lcd_write_data(0x18);

    lcd_write_command(0xB6);
    lcd_write_data(0x08);
    lcd_write_data(0x82);
    lcd_write_data(0x27);

    lcd_write_command(0xF2);
    lcd_write_data(0x00);
    lcd_write_command(0x26);
    lcd_write_data(0x01);

    lcd_write_command(0xE0);
    const unsigned char gamma1[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
            0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
    unsigned char c = 0;
    while (c < 16) {
        lcd_write_data(gamma1[c]);
        c++;
    }

    lcd_write_command(0xE1);
    const unsigned char gamma2[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
            0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
    c = 0;
    while (c < 16) {
        lcd_write_data(gamma2[c]);
        c++;
    }

    lcd_write_command(0x11);
    SysCtlDelay(100000);
    lcd_write_command(0x29);
    lcd_write_command(0x2C);
#endif
#if 1
    lcd_write_command  (0x01);

    delay_ms1(10);

    //************* Start Initial Sequence **********//

    lcd_write_command(0x28);                     // display off  
 
    /* Start Initial Sequence ----------------------------------------------------*/
     lcd_write_command(0xCF);                     
     lcd_write_data(0x00);
     lcd_write_data(0x83);
     lcd_write_data(0x30);
     
     lcd_write_command(0xED);                     
     lcd_write_data(0x64);
     lcd_write_data(0x03);
     lcd_write_data(0x12);
     lcd_write_data(0x81);
     
     lcd_write_command(0xE8);                     
     lcd_write_data(0x85);
     lcd_write_data(0x01);
     lcd_write_data(0x79);
     
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
     
     lcd_write_command(0xC0);                     // POWER_CONTROL_1
     lcd_write_data(0x26);
 
     lcd_write_command(0xC1);                     // POWER_CONTROL_2
     lcd_write_data(0x11);
     
     lcd_write_command(0xC5);                     // VCOM_CONTROL_1
     lcd_write_data(0x35);
     lcd_write_data(0x3E);
     
     lcd_write_command(0xC7);                     // VCOM_CONTROL_2
     lcd_write_data(0xBE);
     
     lcd_write_command(0x36);                     // MEMORY_ACCESS_CONTROL
     lcd_write_data(0x48);
     
     lcd_write_command(0x3A);                     // COLMOD_PIXEL_FORMAT_SET
     lcd_write_data(0x55);                 // 16 bit pixel 
     
     lcd_write_command(0xB1);                     // Frame Rate
     lcd_write_data(0x00);
     lcd_write_data(0x1B);               
     
     lcd_write_command(0xF2);                     // Gamma Function Disable
     lcd_write_data(0x08);
     
     lcd_write_command(0x26);                     
     lcd_write_data(0x01);                 // gamma set for curve 01/2/04/08
     
     lcd_write_command(0xE0);                     // positive gamma correction
     lcd_write_data(0x1F); 
     lcd_write_data(0x1A); 
     lcd_write_data(0x18); 
     lcd_write_data(0x0A); 
     lcd_write_data(0x0F); 
     lcd_write_data(0x06); 
     lcd_write_data(0x45); 
     lcd_write_data(0x87); 
     lcd_write_data(0x32); 
     lcd_write_data(0x0A); 
     lcd_write_data(0x07); 
     lcd_write_data(0x02); 
     lcd_write_data(0x07);
     lcd_write_data(0x05); 
     lcd_write_data(0x00);
     
     lcd_write_command(0xE1);                     // negativ gamma correction
     lcd_write_data(0x00); 
     lcd_write_data(0x25); 
     lcd_write_data(0x27); 
     lcd_write_data(0x05); 
     lcd_write_data(0x10); 
     lcd_write_data(0x09); 
     lcd_write_data(0x3A); 
     lcd_write_data(0x78); 
     lcd_write_data(0x4D); 
     lcd_write_data(0x05); 
     lcd_write_data(0x18); 
     lcd_write_data(0x0D); 
     lcd_write_data(0x38);
     lcd_write_data(0x3A); 
     lcd_write_data(0x1F);

    WindowMax();

     lcd_write_command(0xB7);                       // entry mode
     lcd_write_data(0x07);
     
     lcd_write_command(0xB6);                       // display function control
     lcd_write_data(0x0A);
     lcd_write_data(0x82);
     lcd_write_data(0x27);
     lcd_write_data(0x00);
     
     lcd_write_command(0x11);                     // sleep out
     
    delay_ms1(10);
     
     lcd_write_command(0x29);                     // display on
     
    delay_ms1(10);

     clearScreen(1);
#endif
    setColor(0xF800);
    drawString(5, 5, 0, "Texas Instruments");

    delay_ms1(10);
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

