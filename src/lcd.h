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

#ifndef LCD_H
#define LCD_H

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
/* Basic Colors */
#define RED             0xf800
#define GREEN           0x07e0
#define BLUE            0x001f
#define BLACK           0x0000
#define YELLOW          0xffe0
#define WHITE           0xffff

/* Other Colors */
#define CYAN            0x07ff  
#define BRIGHT_RED      0xf810  
#define GRAY1           0x8410  
#define GRAY2           0x4208  

/*-----------------------------------------------------------------------------
 *  Types
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/
typedef struct
{
    void (*open)(void);
    void (*clear_screen)(void);
    void (*fill_area)(uint16_t x0, uint16_t y0, 
                      uint16_t x1, uint16_t y1, 
                      uint16_t color);
    void (*fill_rectangle)(uint16_t x, uint16_t y, 
                           uint16_t length, uint16_t width, 
                           uint16_t color);
    void (*fill_circle)(uint16_t xc, uint16_t yc, 
                        uint16_t r,
                        uint16_t color);
    void (*draw_line)(uint16_t x0, uint16_t y0, 
                      uint16_t x1, uint16_t y1,
                      uint16_t color);
    void (*draw_rectangle)(uint16_t x0, uint16_t y0, 
                           uint16_t x1, uint16_t y1,
                           uint16_t color);
    void (*draw_circle)(uint16_t xc, uint16_t yc, 
                        uint16_t r,
                        uint16_t color);
    void (*test)(void);
} lcd_services_t;

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/
void lcd_init(lcd_services_t *lcd_services,
              spi_services_t *spi_services);

#endif
