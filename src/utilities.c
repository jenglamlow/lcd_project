/*
 * =====================================================================================
 *
 *       Filename:  utils.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/15/2014 01:51:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Low Jeng Lam
 *   Organization:  
 *
 * =====================================================================================
 */

/*-----------------------------------------------------------------------------
 *  Include
 *-----------------------------------------------------------------------------*/
/* Local includes */
#include "utilities.h"
#include "inc/hw_nvic.h"


/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/

/* Hardcoded to 80Mhz clock frequency */
#define F_CPU 80000000

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/
static void helper(void)
{
}


/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

uint16_t convert_to_word(uint8_t high_byte, 
                         uint8_t low_byte)
{
    return (uint16_t)(((uint16_t)(high_byte) << 16) | low_byte);
}

/**
 * @brief  Delay in microseconds (Blocking)
 *
 * @param us    Microseconds
 */
void delay_us(uint32_t us)
{
    volatile uint32_t elapsed_time;
    uint32_t start_time = HWREG(NVIC_ST_CURRENT);
    do{
            elapsed_time = start_time-(HWREG(NVIC_ST_CURRENT) & 0x00FFFFFF);
    }
    while(elapsed_time <= us * (F_CPU/1000000));
}


/**
 * @brief  Delay in Milliseconds (Blocking)
 *
 * @param ms    Milliseconds
 */
void delay_ms(uint32_t ms)
{
    /* uint32_t i; */
    /* for(i=0; i<ms; i++){ */
    /*         delay_us(1000); */
    /* } */
    ms = (ROM_SysCtlClockGet()/3000)*ms;
    ROM_SysCtlDelay(ms);
}
