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
#include "tft.h"

/* Local includes */
#include "lib.h"
#include "cmdparser.h"

/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/

#define CMD_STX (2U)
#define CMD_ETX (3U)

/* Byte Order Definition */
#define HIGH_BYTE (0U)
#define LOW_BYTE  (1U)

/* Definition of buffer index */
#define CMD_INDEX       (0U)
#define SIZE_HIGH_INDEX (1U)
#define SIZE_LOW_INDEX  (2U)
#define DATA_INDEX      (3U)

/* Definition of CMD BLK index */
#define BLK_X0_HIGH_INDEX   (3U)
#define BLK_X0_LOW_INDEX    (4U)
#define BLK_Y0_HIGH_INDEX   (5U)
#define BLK_Y0_LOW_INDEX    (6U)
#define BLK_X1_HIGH_INDEX   (7U)
#define BLK_X1_LOW_INDEX    (8U)
#define BLK_Y1_HIGH_INDEX   (9U)
#define BLK_Y1_LOW_INDEX    (10U)
#define BLK_COLOR_INDEX     (11U)

#define MSG_SIZE (256U)

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/* Internal state for command parser */
typedef enum
{
    STATE_EXPECT_STX,
    STATE_EXPECT_CMD,
    STATE_EXPECT_SIZE,
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
    cmd_t       name;

    /* Minimum cmd data size */
    uint32_t    size;

} cmd_definition_t;

typedef struct
{
    /* Flag to notify complete message found */
    bool message_found;

    /* Command Definition */
    cmd_definition_t cmd;
    
    /* Command Parser State */
    state_t state;
    
    /* Buffer to store last parsed completed message */
    uint8_t buffer[MSG_SIZE];

    /* Index to track buffer */
    uint32_t index;

    /* Buffer size */
    uint32_t size;

} cmd_info_t;

/* Function Pointer for Command Parser State Action */
typedef void (*cmd_state_action_t)(uint8_t byte);

/* Function Pointer for Received Command Action */
typedef void (*cmd_invoke_action_t)(void);

/*-----------------------------------------------------------------------------
 *  Private Data
 *-----------------------------------------------------------------------------*/

/* State Action Function Definition */
static void state_stx(uint8_t byte);
static void state_cmd(uint8_t byte);
static void state_size(uint8_t byte);
static void state_data(uint8_t byte);
static void state_etx(uint8_t byte);

/* Received Command Action */
static void blk_action(void);
static void img_action(void);
static void str_action(void);

/* Command Table to store command list with expected data size */
static const cmd_definition_t cmd_table[MAX_CMD] = 
{
    {CMD_BLK, 4},
    {CMD_IMG, 5},
    {CMD_STR, 7}
};

/* Table storing command state function */
static const cmd_state_action_t cmd_state_table[] = 
{
    state_stx,
    state_cmd,
    state_size,
    state_data,
    state_etx,
};

static const cmd_invoke_action_t cmd_invoke[] = 
{
    blk_action,
    img_action,
    str_action
};

/* Command parser service component */
static cmdparser_services_t     *cmdparser;
static tft_services_t           *tft;

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

/**
 * @brief  Store byte into buffer
 *
 * @param byte:     Data byte
 */
static void store_buffer(uint8_t byte)
{
    if (cmd_info.index < MSG_SIZE)
    {
        cmd_info.buffer[cmd_info.index++] = byte;
    }
}

/**
 * @brief  Reset buffer index to zero
 */
static void reset_buffer(void)
{
    cmd_info.index = 0;
}

static uint32_t get_command_size(void)
{
    return cmd_info.index;
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

        if (byte == (uint8_t)(cmd_table[cmd_index].name))
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
        set_state(STATE_EXPECT_CMD);
    }
}

static void state_cmd(uint8_t byte)
{
    /* Check whether byte is a valid command byte */
    if (is_cmd_byte(byte))
    {
        /* Reset buffer index */
        reset_buffer();

        /* Store byte into buffer */
        store_buffer(byte);
        
        /* Store command name and expected data size */
        cmd_info.cmd.name = (cmd_t)byte;
        cmd_info.cmd.size = cmd_table[byte].size;

        set_state(STATE_EXPECT_DATA);
    }
    else
    {
        /* Return back to STATE_EXPECT_STX to start again */
        set_state(STATE_EXPECT_STX);
    }
}

static void state_size(uint8_t byte)
{
    static uint8_t data_byte = HIGH_BYTE;

    if(data_byte == HIGH_BYTE)
    {
        /* High Byte */

        /* Buffer the high byte order data */
        store_buffer(byte);

        /* Change the next expected byte to LOW_BYTE */
        data_byte = LOW_BYTE;
    }
    else
    {
        /* Low Byte */

        /* Buffer the low byte order data */
        store_buffer(byte);

        /* Calculate 16-bit data size */
        uint32_t high_byte = cmd_info.buffer[SIZE_HIGH_INDEX];
        uint32_t low_byte = cmd_info.buffer[SIZE_LOW_INDEX];

        cmd_info.size = ((high_byte << 8) | low_byte);

        /* Change the next expected byte to LOW_BYTE */
        data_byte = HIGH_BYTE;

        if (cmd_info.size > cmd_info.cmd.size)
        {
            /* set state to STATE_EXPECT_DATA */
            set_state(STATE_EXPECT_DATA);
        }
        else
        {
            /* Receive data size is smaller than minimum size */
            /* Reset back to STATE_EXPECT_STX */
            set_state(STATE_EXPECT_STX);
        }

    }

}

static void state_data(uint8_t byte)
{
    /* Data start from index 3 */
    uint32_t index = cmd_info.index + 3;
    
    if(index < cmd_info.size)
    { 
        /* Buffer byte */
        store_buffer(byte);
    }
    else
    {
        /* Match expected size */

        /* Set to EXPECT_ETX */
        set_state(STATE_EXPECT_ETX);
    }
}

static void state_etx(uint8_t byte)
{ 
    if (byte == CMD_ETX)
    {
        /*  Message found  */
        cmd_info.message_found = true;
    }
    

    /* Reset back to STATE_EXPECT_STX for new message packet */
    set_state(STATE_EXPECT_STX);
}

static void blk_action(void)
{
    /* BLK, SIZE_H, SIZE_L, x0(H) ,x0(L) ,y0(H), y0(L), x1(H), x1(L), y1(H), i
     * y1(L), color */
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
    uint16_t color;

    x0 = convert_to_word(cmd_info.buffer[BLK_X0_HIGH_INDEX], 
                         cmd_info.buffer[BLK_X0_LOW_INDEX]);

    y0 = convert_to_word(cmd_info.buffer[BLK_Y0_HIGH_INDEX], 
                         cmd_info.buffer[BLK_Y0_LOW_INDEX]);

    x1 = convert_to_word(cmd_info.buffer[BLK_X1_HIGH_INDEX], 
                         cmd_info.buffer[BLK_X1_LOW_INDEX]);
    
    y1 = convert_to_word(cmd_info.buffer[BLK_Y1_HIGH_INDEX], 
                         cmd_info.buffer[BLK_Y1_LOW_INDEX]);

    color = (uint16_t)cmd_info.buffer[BLK_COLOR_INDEX];

    tft->fill_area(x0, y0, x1, y1, color);
}

/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/

cmd_t cmdparser_get_command()
{
    return cmd_info.cmd.name;
}

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

bool cmdparser_process(uint8_t byte)
{
    bool is_done = false;

    if (cmdparser_parse(byte))
    {
        /* If full message packet found */

        /* Get command from the packet */
        uint8_t cmd_index = (uint8_t)cmdparser_get_command();

        /* Invoke Action based on command */ 
        cmd_invoke[cmd_index]();

        is_done = true;
    }
    
    return is_done;
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

void cmdparser_init(cmdparser_services_t*   cmdparser_services,
                    tft_services_t*         tft_services)
{
    cmdparser = cmdparser_services;
    tft = tft_services;

    cmdparser->parse = cmdparser_parse;
    cmdparser->process = cmdparser_process;

    /* Command Info Initialisation */
    cmd_info.message_found = false;
    cmd_info.state = STATE_EXPECT_STX;
    cmd_info.index = 0;

    cmd_info.cmd.name = MAX_CMD;
    cmd_info.cmd.size = 0;

    
}
