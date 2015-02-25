/*
 * =====================================================================================
 *
 *       Filename:  cmdparser.c
 *
 *    Description:  Implementation file for command Parser for UART message received 
 *
 *        Version:  1.0
 *        Created:  02/03/2015 07:32:15 PM
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
#include "lib.h"
#include "cmdparser.h"

/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/

#define CMD_STX (2U)
#define CMD_ETX (3U)

#define MSG_SIZE (256U)

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/* Internal state for command parser */
typedef enum
{
    STATE_EXPECT_STX,
    STATE_EXPECT_CMD,
    STATE_EXPECT_DATA,
    STATE_EXPECT_ETX
} state_t;

/* Command Type Definition */ 
typedef enum
{
    CMD_BLK = 0x00,
    CMD_IMG,
    CMD_STR,
    MAX_CMD
} cmd_t;

/* Command Info Type Definition */
typedef struct
{
    /* Command Type */
    cmd_t       cmd;

    /* Expected cmd data size */
    uint32_t    size;

} cmd_definition_t;

typedef struct
{
    /* Flag to notify complete message found */
    bool message_found;
    
    /* Command Parser State */
    state_t state;
    
    /* Buffer to store last parsed completed message */
    uint8_t buffer[MSG_SIZE];

    /* Message buffer size */
    uint32_t buffer_size;

} cmd_info_t;

/* Function Pointer for Command Parser State Action */
typedef void (*cmd_func_ptr)(uint8_t byte);

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

/* State Action Function Definition */
static void state_stx(uint8_t byte);
static void state_cmd(uint8_t byte);
static void state_data(uint8_t byte);
static void state_etx(uint8_t byte);

/* Command Table to store command list with expected data size */
static const cmd_definition_t cmd_table[MAX_CMD] = 
{
    {CMD_BLK, 10},
    {CMD_IMG, 10},
    {CMD_STR, 10}
};

/* Table storing command state function */
static const cmd_func_ptr cmd_state_table[] = 
{
    state_stx,
    state_cmd,
    state_data,
    state_etx,
};

/* Command State */
static cmd_info_t cmd_info;

/*-----------------------------------------------------------------------------
 *  Private Function
 *-----------------------------------------------------------------------------*/


/**
 * @brief   Get the current command parser state 
 *
 * @return: Current Command Parser State 
 */
static state_t get_state(void)
{
    return cmd_info.state;
}


/**
 * @brief  Set command parser state to current state
 *
 * @param state:  Desired command parser state
 */
static void set_state(state_t state)
{
    cmd_info.state = state;
}

static void store_buffer(uint8_t byte)
{


}

/**
 * @brief  Check whether the byte is a valid command byte
 *
 * @param byte:     Input byte
 *
 * @return:         True if it is a valid command byte, Otherwise, False
 */
static bool is_cmd_byte(uint8_t byte)
{
    bool is_valid_cmd = false;
    uint8_t cmd_index = 0;

    /* 
     * Iterate throught the command table definition to compare 
     * with the input byte 
     */
    while (cmd_index < (uint8_t)(MAX_CMD))
    {

        if (byte == (uint8_t)(cmd_table[cmd_index].cmd))
        {
            /* Matched Command found */
            is_valid_cmd = true;
            break;
        }

        /* Increase index to search the next command list from the table */
        cmd_index++;
    }

    return is_valid_cmd;
}

static void state_stx(uint8_t byte)
{
    /* If STX found */
    if (byte == CMD_STX)
    {
        /* Store byte into buffer */



        set_state(STATE_EXPECT_STX);
    }
}

static void state_cmd(uint8_t byte)
{
    /* Check whether byte is a valid command byte */
    if (is_cmd_byte(byte))
    {
        set_state(STATE_EXPECT_DATA);
    }
    else
    {
        /* Return back to STATE_EXPECT_STX to start again */
        set_state(STATE_EXPECT_STX);
    }
}

static void state_data(uint8_t byte)
{
    /* If STX found */
    if (byte == CMD_STX)
    {
        set_state(STATE_EXPECT_STX);
    }
}

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

bool cmdparser_parse(uint8_t byte)
{
    bool is_found;
    uint8_t state = (uint8_t)get_state();

    /* Execute Command Parser */
    cmd_state_table[state](byte);

    /* Check whether complete message found */
    is_found = cmd_info.message_found;

    /* Clear message found flag */
    cmd_info.message_found = false;

    return is_found; 
}

/*-----------------------------------------------------------------------------
 *  Event call-backs
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/

void cmdparser_init(void)
{
    cmd_info.message_found = false;
    cmd_info.state = STATE_EXPECT_STX;
}
