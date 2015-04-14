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
#include "fonts.h"
#include "ringbuf.h"

/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/
#define NON_BLOCKING        (0)

/* TFT (ILI9341) size */
#define TFT_HEIGHT          (320)
#define TFT_WIDTH           (240)

#define MIN_X               (0)
#define MIN_Y               (0)
#define MAX_X               (319)
#define MAX_Y               (239)

/* Command info FIFO queue size */
#define CMD_QUEUE_SIZE      (64)

/* Command info TFT data size */
#define CMD_DATA_SIZE       (4096)

/* pin mapping for RST tft */ 
#define RST_PIN_BASE        GPIO_PORTE_BASE
#define RST_PIN             GPIO_PIN_3
/* Set RST pin to high */
#define SET_RST_PIN         SET_BITS(RST_PIN_BASE, RST_PIN)
/* Set RST pin to low */
#define CLEAR_RST_PIN       CLEAR_BITS(RST_PIN_BASE, RST_PIN)

/* pin mapping for CS tft */
#define CS_PIN_BASE        GPIO_PORTA_BASE
#define CS_PIN             GPIO_PIN_3
/* Set CS pin to high */
#define SET_CS_PIN         SET_BITS(CS_PIN_BASE, CS_PIN)
/* Set CS pin to low */
#define CLEAR_CS_PIN       CLEAR_BITS(CS_PIN_BASE, CS_PIN)

/* pin mapping for D/C tft */
#define DC_PIN_BASE         GPIO_PORTE_BASE
#define DC_PIN              GPIO_PIN_2
/* Set D/C pin to high */
#define SET_DC_PIN          SET_BITS(DC_PIN_BASE, DC_PIN)
/* Set D/C pin to low */
#define CLEAR_DC_PIN        CLEAR_BITS(DC_PIN_BASE, DC_PIN)

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

typedef enum
{
    STATE_CMD = 0,//!< STATE_CMD
    STATE_DATA,   //!< STATE_DATA
} tft_send_state_t;

typedef void (*tft_action_t)(void);

#if NON_BLOCKING
typedef struct
{
    uint8_t cmd;
    uint32_t size;
} tft_cmd_info_t;

typedef struct
{
    uint8_t rx;
    uint8_t wx;

    tft_cmd_info_t      buffer[CMD_QUEUE_SIZE];
} cmd_queue_t;

/* Info per TFT device */
typedef struct
{
    tRingBufObject data_ringbuf_obj;

    /* Internal State of TFT */
    tft_state_t         state;
    tft_send_state_t    send_state;

    uint8_t             data[CMD_DATA_SIZE];

    tft_cmd_info_t      last_cmd;

    /* Callback when TFT is DONE */
    tft_done_cb_t       done_cb;

} tft_info_t;
#endif

typedef struct
{
    uint16_t max_x;
    uint16_t max_y;
    uint8_t  orientation;
} tft_info_t;

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

static spi_services_t   *spi;
static tft_info_t       tft_info;
#if NON_BLOCKING
static cmd_queue_t      cmd_queue;
#endif

/* Function Prototype */
static void spi_tx_cb(void);

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/
#if NON_BLOCKING
static bool cmd_queue_empty()
{
    uint8_t rx = cmd_queue.rx;
    uint8_t wx = cmd_queue.wx;

    return ((wx == rx) ? true : false);
}

static bool cmd_queue_full()
{
    uint8_t rx = cmd_queue.rx;
    uint8_t wx = cmd_queue.wx;

    return ((((wx + 1) % CMD_QUEUE_SIZE) == rx) ? true : false);
}

static bool cmd_queue_put(tft_cmd_info_t *cmd_info)
{
    ASSERT(cmd_info != NULL);

    bool is_full = cmd_queue_full();

    if (is_full == false)
    {
        uint8_t wx = cmd_queue.wx;

        cmd_queue.buffer[wx].cmd = cmd_info->cmd;
        cmd_queue.buffer[wx].size = cmd_info->size;

        /* Update write index */
        cmd_queue.wx = ((wx + 1) % CMD_QUEUE_SIZE);
    }

    return !is_full;
}

static bool cmd_queue_read(tft_cmd_info_t *cmd_info)
{
    ASSERT(cmd_info != NULL);

    bool is_empty = cmd_queue_empty();

    if (is_empty == false)
    {
        uint8_t rx = cmd_queue.rx;

        cmd_info->cmd = cmd_queue.buffer[rx].cmd;
        cmd_info->size = cmd_queue.buffer[rx].size;

        /* Update read index */
        cmd_queue.rx = ((rx + 1) % CMD_QUEUE_SIZE);
    }

    return !is_empty;
}
#endif

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

    /* Enable PortA for CS PIN */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    /* Set CS pin as output */
    GPIOPinTypeGPIOOutput(CS_PIN_BASE, CS_PIN);

    SET_CS_PIN;

    /* SPI module initialization */
    spi->open(SPI_TFT, spi_tx_cb);
}

/**
 * @brief  TFT write command
 *
 * @param data: Command (8-bit) 
 */
static void tft_send_command(uint8_t cmd)
{
    CLEAR_DC_PIN;

    CLEAR_CS_PIN;

    spi->write(SPI_TFT, cmd);

    SET_CS_PIN;
}

/**
 * @brief  TFT write data
 *
 * @param data: data (8-bit)
 */
static void tft_send_data(uint8_t data)
{
    SET_DC_PIN;

    CLEAR_CS_PIN;

    spi->write(SPI_TFT, data);

    SET_CS_PIN;
}

#if NON_BLOCKING
static void send_command_struct(tft_cmd_info_t* cmd_info)
{
    tft_info.last_cmd.cmd = cmd_info->cmd;
    tft_info.last_cmd.size = cmd_info->size;

    /* Change to STATE_DATA if contain data */
    if (cmd_info->size > 0)
    {
        tft_info.send_state = STATE_DATA;
    }
    /* Command only */
    else
    {
        tft_info.send_state = STATE_CMD;
    }

    spi->write_non_blocking(SPI_TFT, &cmd_info->cmd, 1);
}
#endif

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

    CLEAR_CS_PIN;

    spi->write(SPI_TFT, high_byte);
    spi->write(SPI_TFT, low_byte);

    SET_CS_PIN;
}

/**
 * @brief  TFT set column
 *
 * @param start_column: Starting position of the column
 * @param end_column: End position of the column
 */
static void set_column(uint16_t start_column,uint16_t end_column)
{
    tft_send_command(CASETP);              /* Column Address Set */
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
    tft_send_command(PASETP);              /* Page Address Set */
    send_word(StartPage);
    send_word(EndPage);
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
    tft_send_command(RAMWRP);              /* Memory Write */
}
/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

#if NON_BLOCKING
static void tft_state_cmd(void)
{
    tft_cmd_info_t cmd_info;

    ASSERT(tft_info.send_state == STATE_CMD);
    ASSERT(tft_info.state == TFT_BUSY);

    CLEAR_DC_PIN;

    bool result = cmd_queue_read(&cmd_info);

    /* Contain queue */
    if (result)
    {
        send_command_struct(&cmd_info);
    }
    else
    {
        /* No more data */
        tft_info.state = TFT_READY;

        /* Callback to when TFT is READY */
        tft_info.done_cb();
    }
}

static void tft_state_data(void)
{
    uint32_t size = tft_info.last_cmd.size;
    uint32_t read_size;
    uint8_t data[64];

    ASSERT(tft_info.send_state == STATE_DATA);
    ASSERT(tft_info.state == TFT_BUSY);

    SET_DC_PIN;

    while (size)
    {
        if (size <= 64)
        {
            read_size = size;
            RingBufRead(&tft_info.data_ringbuf_obj, &data[0], read_size);
        }
        /* Expected size more than 64 bytes */
        else
        {
            read_size = 64;
            RingBufRead(&tft_info.data_ringbuf_obj, &data[0], read_size);
        }

        spi->write_non_blocking(SPI_TFT, &data[0], size);
        size -= read_size;
    }

    /* TODO: Handle next state for data */
    tft_info.send_state = STATE_CMD;
}
#endif

/* Callback from SPI when transmission completed */
static void spi_tx_cb(void)
{
#if NON_BLOCKING
    if (tft_info.send_state == STATE_CMD)
    {
        tft_state_cmd();
    }
    else
    {
        tft_state_data();
    }
#endif
}


/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/
#if NON_BLOCKING
static void tft_register_done_callback(tft_done_cb_t tft_done_cb)
{
    ASSERT(tft_done_cb != NULL);

    tft_info.done_cb = tft_done_cb;
}
#endif

static void tft_send_raw(uint8_t    cmd,
                         uint8_t*   data,
                         uint32_t   size)
{
#if NON_BLOCKING
    /* Buffer the raw command */
    tft_cmd_info_t cmd_info;

    cmd_info.cmd = cmd;
    cmd_info.size = size;

    /* Queue the command data */
    bool result;
    result = cmd_queue_put(&cmd_info);
    ASSERT(result != false);

    /* Buffer the data byte if contain data */
    if (size > 0)
    {
        RingBufWrite(&tft_info.data_ringbuf_obj, data, size);
    }

    /* Check if TFT is ready */
    if (tft_info.state == TFT_READY)
    {
        tft_info.state = TFT_BUSY;

        /* Send data if TFT is ready */
        CLEAR_DC_PIN;

        result = cmd_queue_read(&cmd_info);
        ASSERT(result != false);

        send_command_struct(&cmd_info);
    }
#else
    uint32_t i;

    tft_send_command(cmd);
    for (i = 0; i < size; i++)
    {
        tft_send_data(data[i]);
    }
#endif
}

static void tft_set_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    set_column(x0, x1);
    set_page(y0, y1);
    tft_send_command(RAMWRP);              /* Memory Write */
}

static void tft_start_image_transfer(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if ((tft_info.orientation == ORIENT_H) || (tft_info.orientation == ORIENT_H_I))
    {
        tft_set_area(x0, y0, x1, y1);
    }
    else
    {
        tft_set_area(x0, y0, y1, x1);
    }
    CLEAR_CS_PIN;
    SET_DC_PIN;
}

static void tft_done_transfer(void)
{
    /* Set CS pin to high to indicate transfer is completed */
    SET_CS_PIN;
}

static void tft_set_orientation(uint8_t orientation)
{
    tft_send_command(MADCTL);       /* Memory Access Control */
    tft_send_data(orientation);     /* Refresh Order - BGR colour filter */

    tft_info.orientation = orientation;
    if ((orientation == ORIENT_H) || (orientation == ORIENT_H_I))
    {
        tft_info.max_x = MAX_X;
        tft_info.max_y = MAX_Y;
    }
    else
    {
        tft_info.max_x = MAX_Y;
        tft_info.max_y = MAX_X;
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
    x0 = constrain(x0, MIN_X, tft_info.max_x);
    x1 = constrain(x1, MIN_X, tft_info.max_x);
    y0 = constrain(y0, MIN_Y, tft_info.max_y);
    y1 = constrain(y1, MIN_Y, tft_info.max_y);

    /* get total area (pixels) */
    xy = (x1 - x0 + 1);
    xy = xy * (y1 - y0 + 1);

    /* Set Coordinate */
    tft_set_area(x0, y0, x1, y1);

    SET_DC_PIN;
    CLEAR_CS_PIN;

    /* Start Filling area with color */
    uint8_t high_color = color >> 8;
    uint8_t low_color = color & 0xff;
    for(i=0; i < xy; i++)
    {
        spi->write(SPI_TFT, high_color);
        spi->write(SPI_TFT, low_color);
    }
    SET_CS_PIN;
}

/**
* @brief  Clear TFT screen to all black
*/
static void tft_clear_screen(void)
{
    tft_fill_area(MIN_X, MIN_Y, tft_info.max_x, tft_info.max_y, BLACK);
}

static void tft_reset(void)
{
    /* Reset TFT Pin */
    CLEAR_RST_PIN;
    delay_ms(10);
    SET_RST_PIN;
    delay_ms(500);
}

/**
 * @brief  TFT initialization (Hardware)
 */
static void tft_start(void)
{
    /* Initialize Hardware I/O for TFT */
    hw_init();

    /* strawman transfer */
    spi->write(SPI_TFT,0);        

    /* Reset TFT Pin */
    tft_reset();

    /* Software Reset */
    tft_send_command(SWRESET);
    delay_ms(200);

    tft_send_command(PWRCTRLA);        /* Power Control */
    tft_send_data(0x39);
    tft_send_data(0x2C);
    tft_send_data(0x00);
    tft_send_data(0x34);
    tft_send_data(0x02);

    tft_send_command(PWRCTRLB);
    tft_send_data(0x00);
    tft_send_data(0XC1);
    tft_send_data(0X30);

    tft_send_command(DTCTRLA1);        /* Driver Timing Control */
    tft_send_data(0x85);
    tft_send_data(0x00);
    tft_send_data(0x78);

    tft_send_command(DTCTRLB);
    tft_send_data(0x00);
    tft_send_data(0x00);

    tft_send_command(POSC);            /* Power On Sequence Control */
    tft_send_data(0x64);
    tft_send_data(0x03);
    tft_send_data(0X12);
    tft_send_data(0X81);

    tft_send_command(PRC);             /* Pump Ratio Control */
    tft_send_data(0x20);

    tft_send_command(ILIPC1);          /* power control */
    tft_send_data(0x23);

    tft_send_command(ILIPC2);
    tft_send_data(0x10);

    tft_send_command(ILIVC1);          /* VCOM Control */
    tft_send_data(0x3e);
    tft_send_data(0x28);

    tft_send_command(ILIVC2);
    tft_send_data(0x86);

    tft_set_orientation(ORIENT_V);

    tft_send_command(COLMOD);          /* Pixel Format Set */
    tft_send_data(0x55);               /* 16 bits/pixel */

    tft_send_command(ILIFCNM);         /* Frame Rate Control */
    tft_send_data(0x00);               /* Fosc */
    tft_send_data(0x10);               /* 16 clocks for line period */

    tft_send_command(ILIDFC);       /* Display Function Control */
    tft_send_data(0x08);               /* Interval Scan */
    tft_send_data(0x82);               /* Normally Black , Scan cycle interval */
    tft_send_data(0x27);

    tft_send_command(ILIGFD);       /* 3Gamma Function */
    tft_send_data(0x00);               /* Disable 3G */

    tft_send_command(ILIGS);           /* Gamma Set */
    tft_send_data(0x01);               /* Gamma curve */

    tft_send_command(ILIPGC);       /* Positive Gamma Correction */
    tft_send_data(0x0F);
    tft_send_data(0x31);
    tft_send_data(0x2B);
    tft_send_data(0x0C);
    tft_send_data(0x0E);
    tft_send_data(0x08);
    tft_send_data(0x4E);
    tft_send_data(0xF1);
    tft_send_data(0x37);
    tft_send_data(0x07);
    tft_send_data(0x10);
    tft_send_data(0x03);
    tft_send_data(0x0E);
    tft_send_data(0x09);
    tft_send_data(0x00);

    tft_send_command(ILINGC);       /* Negative Gamma Correction */
    tft_send_data(0x00);
    tft_send_data(0x0E);
    tft_send_data(0x14);
    tft_send_data(0x03);
    tft_send_data(0x11);
    tft_send_data(0x07);
    tft_send_data(0x31);
    tft_send_data(0xC1);
    tft_send_data(0x48);
    tft_send_data(0x08);
    tft_send_data(0x0F);
    tft_send_data(0x0C);
    tft_send_data(0x31);
    tft_send_data(0x36);
    tft_send_data(0x0F);

    tft_send_command(SLEEPOUT);        /* Turn off Sleep Mode */
    delay_ms(120); 

    tft_send_command(DISPON);          /* Display On */

    /* Memory Write - reset to Start Column/Start Page position */
    tft_send_command(RAMWRP);
    
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
    tft_send_command(RAMWRP);              /* Memory Write */

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
    tft_send_command(RAMWRP);              /* Memory Write */

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

static void tft_send_data_only(uint8_t byte)
{
    spi->write(SPI_TFT, byte);
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
static void tft_draw_rectangle(uint16_t x, uint16_t y, 
                               uint16_t length, uint16_t width,
                               uint16_t color)
{
    tft_draw_horizontal_line(x, y, length, color);
    tft_draw_horizontal_line(x, y + width, length, color);
    tft_draw_vertical_line(x, y, width, color);
    tft_draw_vertical_line(x + length, y, width, color);
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
 * @brief Draw ASCII charactar at (x,y) with foreground and background colour 
 *
 * @param ascii:    ASCII Character. eg: 'A'
 * @param x:        Starting x position
 * @param y:        Starting y position
 * @param size:     Size of the font
 * @param fgcolor:  Foreground colour
 * @param bgcolor:  Background colour
 */
static void tft_draw_char(uint8_t ascii, uint16_t x, uint16_t y, 
                          uint16_t size, uint16_t fgcolor, uint16_t bgcolor)
{
    uint8_t i, f;

    if((ascii >= 32) && (ascii <= 127))
    {
        ;
    }
    else
    {
        ascii = '?'-32;
    }
    for (i =0; i < TFT_FONT_X; i++ )
    {
        uint8_t temp = font_map[ascii-0x20][i];
        for(f=0; f < 8; f++)
        {
            if((temp >> f) & 0x01)
            {
                tft_fill_rectangle(x + i * size, y + f * size,
                                   size, size, fgcolor);
            }
            else
            {
                tft_fill_rectangle(x + i * size, y + f * size,
                                   size, size, bgcolor);
            }
        }
    }
}

/**
* @brief  Draw string at (x,y) with fill foreground and background color
*
* @param string:    String input. Eg: "abc"
* @param x:         x coordinate
* @param y:         y coordinate
* @param size:      font size
* @param fgcolor:   foreground color
* @param bgcolor:   background color
*/
static void tft_draw_string(char *string, uint16_t x, uint16_t y,
                     uint16_t size, uint16_t fgcolor, uint16_t bgcolor)
{
    while(*string)
    {
        tft_draw_char(*string, x, y, size, fgcolor, bgcolor);
        *string++;

        if(x < tft_info.max_x)
        {
            x += TFT_FONT_SPACE * size;                                     /* Move cursor right            */
        }
    }
}


/**
 * @brief Print number character at (x,y) with foreground & background color 
 *
 * @param long_num: Number to be printed
 * @param x:        x-coordinate
 * @param y:        y-coordinate
 * @param size:     Size of the number text
 * @param fgcolor:  Foreground color
 * @param bgcolor:  Background color
 *
 * @return:         The number of character printed for the number input
 */
static uint8_t tft_draw_number(int long_num, uint16_t x, uint16_t y, 
                               uint16_t size, uint16_t fgcolor, uint16_t bgcolor)
{
    uint8_t char_buffer[10] = "";
    uint8_t i = 0;
    uint8_t f = 0;

    if (long_num < 0)
    {
        f = 1;
        tft_draw_char('-',x, y, size, fgcolor, bgcolor);
        long_num = -long_num;
        if(x < tft_info.max_x)
        {
            x += TFT_FONT_SPACE * size;        
        }
    }

    else if (long_num == 0)
    {
        f = 1;
        tft_draw_char('0', x, y, size, fgcolor, bgcolor);
        return f;
        if(x < tft_info.max_x)
        {
            x += TFT_FONT_SPACE * size;       
        }
    }

    while (long_num > 0)
    {
        char_buffer[i++] = (uint8_t)long_num % 10;
        long_num /= 10;
    }

    f = f+i;
    for(; i > 0; i--)
    {
        tft_draw_char('0'+ char_buffer[i - 1], x, y, size, fgcolor, bgcolor);
        if(x < tft_info.max_x)
        {
            /* Move the cursor to right */
            x += TFT_FONT_SPACE*size; 
        }
    }
    return f;
}


/**
 * @brief  Draw ASCII Character without background colour
 *
 * @param ascii:    ASCII Character ('G')
 * @param x:        x coordinate 
 * @param y:        y coordinate
 * @param size:     Font size
 * @param color:    Font colour
 */
static void tft_draw_char_only(uint8_t ascii, uint16_t x, uint16_t y, 
                               uint16_t size, uint16_t color) 
{
    uint8_t col = 0;
    uint8_t row = 0;
    uint8_t bit = 0x01;
    uint8_t oc = ascii - 0x20;

    while (row < 8) 
    {
        while (col < 8) 
        {
                if (font_map[oc][col] & bit)
                {
                    /* tft_set_pixel(x + col, y + row, color); */
                    tft_fill_rectangle(x + col * size, y + row * size,
                                   size, size, color);
                }
                col++;
        }
        col = 0;
        bit <<= 1;
        row++;
    }
}

/**
* @brief  Draw string at (x,y) without background colour
*
* @param string:    String input. Eg: "abc"
* @param x:         x coordinate
* @param y:         y coordinate
* @param size:      font size
* @param color:     color
*/
static void tft_draw_string_only(char *string, uint16_t x, uint16_t y,
                                 uint16_t size, uint16_t color)
{
    while(*string)
    {
        tft_draw_char_only(*string, x, y, size, color);
        *string++;

        if(x < tft_info.max_x)
        {
            x += TFT_FONT_SPACE * size;
        }
    }
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
    tft_draw_char('j', 0, 120, 4, WHITE, RED);
    tft_draw_char('W', 30, 120, 6, WHITE, RED);
    tft_draw_string("abc", 0, 170, 3, BLACK, GREEN);
    tft_draw_number(-12345, 0, 200, 2, BLACK, GREEN);
    tft_draw_char_only('G', 200, 200, 3, WHITE);
    tft_draw_string_only("Tesla", 100, 170, 3, WHITE);
}

static void tft_running_animation(void)
{
    static uint16_t i = 0;
    static uint16_t j = 0;
    
    i = i % 240;

    tft_fill_rectangle(i++, 0, 50, 50,BLACK);
    tft_fill_rectangle(i,0,50,50,BLUE);

    tft_fill_circle(j++, 100, 10, BLACK);
    tft_fill_circle(j, 100, 10, WHITE);
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
    tft_services->start = tft_start;
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
    tft_services->running_animation = tft_running_animation;
    tft_services->draw_char = tft_draw_char;
    tft_services->draw_string = tft_draw_string;
    tft_services->draw_number = tft_draw_number;
    tft_services->draw_char_only = tft_draw_char_only;
    tft_services->draw_string_only = tft_draw_string_only;
    tft_services->set_pixel = tft_set_pixel;
    tft_services->set_area = tft_set_area;
    tft_services->start_image_transfer = tft_start_image_transfer;
    tft_services->send_data_only = tft_send_data_only;
    tft_services->done_transfer = tft_done_transfer;
    tft_services->set_orientation = tft_set_orientation;
    tft_services->send_raw = tft_send_raw;
    tft_services->send_command = tft_send_command;
    tft_services->send_data = tft_send_data;
    tft_services->reset = tft_reset;
#if NON_BLOCKING
    tft_services->register_done_callback = tft_register_done_callback;
#endif

    /* SPI Component Services */
    spi = spi_services;

#if NON_BLOCKING
    tft_info.state = TFT_READY;
    tft_info.send_state = STATE_CMD;

    memset(&cmd_queue.buffer[0], 0, sizeof(&cmd_queue.buffer));
    RingBufInit(&tft_info.data_ringbuf_obj,
                &tft_info.data[0],
                sizeof(tft_info.data));
    cmd_queue.rx = 0;
    cmd_queue.wx = 0;
#endif
}

