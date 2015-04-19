/*
 * =====================================================================================
 *
 *       Filename:  template.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/15/2014 01:54:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef TFT_H
#define TFT_H

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/
/* Third party libraries include */

/* Local includes */
#include "lib.h"
#include "spi.h"

/*-----------------------------------------------------------------------------
 *  Constants
 *-----------------------------------------------------------------------------*/

/* Colors */
#define RED             0xf800
#define GREEN           0x07e0
#define BLUE            0x001f
#define BLACK           0x0000
#define YELLOW          0xffe0
#define WHITE           0xffff
#define CYAN            0x07ff  
#define BRIGHT_RED      0xf810  
#define GRAY1           0x8410  
#define GRAY2           0x4208

/* TFT Orientation (V - Vertical, H - Horizontal, I - Inverted) */
#define ORIENT_V        0x48
#define ORIENT_V_I      0x88
#define ORIENT_H        0xe8
#define ORIENT_H_I      0x28

/*-----------------------------------------------------------------------------
 *  Types
 *-----------------------------------------------------------------------------*/

#if NON_BLOCKING
typedef enum
{
    TFT_READY = 0,
    TFT_BUSY
} tft_state_t;
#endif

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

#if NON_BLOCKING
/* TFT Done Callback */
typedef void (*tft_done_cb_t)(void);
#endif

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

void tft_send_command(uint8_t cmd);
void tft_send_data(uint8_t data);
void tft_send_raw(uint8_t    cmd,
                  uint8_t*   data,
                  uint32_t   size);
void tft_set_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void tft_start_image_transfer(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void tft_done_transfer(void);
void tft_set_orientation(uint8_t orientation);
void tft_fill_area(uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1,
                   uint16_t color);
void tft_clear_screen(void);
void tft_reset(void);
void tft_start(void);
void tft_set_pixel(uint16_t x, uint16_t y, uint16_t color);
void tft_draw_horizontal_line(uint16_t x, uint16_t y,
                              uint16_t length,
                              uint16_t color);
void tft_draw_vertical_line(uint16_t x, uint16_t y,
                            uint16_t length,
                            uint16_t color);
void tft_draw_line(uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1,
                   uint16_t color);
void tft_send_data_only(uint8_t byte);
void tft_fill_rectangle(uint16_t x, uint16_t y,
                        uint16_t length, uint16_t width,
                        uint16_t color);
void tft_fill_circle(uint16_t xc, uint16_t yc,
                     int16_t r,
                     uint16_t color);
void tft_draw_rectangle(uint16_t x, uint16_t y,
                        uint16_t length, uint16_t width,
                        uint16_t color);
void tft_draw_triangle(uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2,
                       uint16_t color);
void tft_draw_circle(uint16_t xc, uint16_t yc,
                     uint16_t r,
                     uint16_t color);
void tft_draw_char(uint8_t ascii, uint16_t x, uint16_t y,
                   uint16_t size, uint16_t fgcolor, uint16_t bgcolor);
void tft_draw_string(char *string, uint16_t x, uint16_t y,
                     uint16_t size, uint16_t fgcolor, uint16_t bgcolor);
uint8_t tft_draw_number(int long_num, uint16_t x, uint16_t y,
                        uint16_t size, uint16_t fgcolor, uint16_t bgcolor);
void tft_draw_char_only(uint8_t ascii, uint16_t x, uint16_t y,
                        uint16_t size, uint16_t color);
void tft_draw_string_only(char *string, uint16_t x, uint16_t y,
                          uint16_t size, uint16_t color);
void tft_test(void);
void tft_running_animation(void);

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

void tft_init(void);

#endif
