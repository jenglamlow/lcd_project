/*
 * =====================================================================================
 *
 *       Filename:  cmd_parser.c
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
#include "cmd_parser.h"
#include "ringbuf.h"

/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/

/* Packet Definition */
#define CMD_STX         (2U)
#define CMD_ETX         (3U)

/* Definition of Buffer Index */
enum
{
    CMD_INDEX = 0U,
    SIZE_32B_INDEX,
    SIZE_24B_INDEX,
    SIZE_16B_INDEX,
    SIZE_8B_INDEX,
    DATA_INDEX
};


/* Definition of CMD BLK index */
enum
{
    BLK_X0_HIGH = 0U,
    BLK_X0_LOW,
    BLK_Y0_HIGH,
    BLK_Y0_LOW,
    BLK_X1_HIGH,
    BLK_X1_LOW,
    BLK_Y1_HIGH,
    BLK_Y1_LOW,
    BLK_COLOR_HIGH,
    BLK_COLOR_LOW
};


/* Definition of CMD STR index */
enum
{
    STR_X_HIGH = 0U,
    STR_X_LOW,
    STR_Y_HIGH,
    STR_Y_LOW,
    STR_FONT_SIZE,
    STR_COLOR_HIGH,
    STR_COLOR_LOW,
    STR_TEXT
};

#define MSG_SIZE        (512U)

/*-----------------------------------------------------------------------------
 *  Private Types
 *-----------------------------------------------------------------------------*/

/* Internal state for command parser */
typedef enum
{
    STATE_EXPECT_STX,
    STATE_EXPECT_CMD,
    STATE_EXPECT_SIZE,
    STATE_EXPECT_DATA_STORE,
    STATE_EXPECT_DATA_PASS,
    STATE_EXPECT_ETX
} state_t;

/* Command Type Definition */ 
typedef enum
{
    CMD_BLK = 0x00,
    CMD_IMG,
    CMD_STR,
    CMD_CLR,
    CMD_RAW,
    MAX_CMD
} cmd_t;

/* Command Info Type Definition */
typedef struct
{
    /* Command Type */
    cmd_t       name;

    /* Expected minimum cmd data size */
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

    /* Struture encapsulating  */
    tRingBufObject data_ringbuf_obj;

    uint32_t data_size;

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
static void state_data_store(uint8_t byte);
static void state_data_pass(uint8_t byte);
static void state_etx(uint8_t byte);

/* Received Command Action */
static void blk_action(void);
static void img_action(void);
static void str_action(void);
static void clr_action(void);
static void raw_action(void);

/* Command Table to store command list with expected minimum data size */
static const cmd_definition_t cmd_table[MAX_CMD] = 
{
    {CMD_BLK, 10},
    {CMD_IMG, 4},
    {CMD_STR, 8},
    {CMD_RAW, 0},
    {CMD_CLR, 0}
};

/* Table storing command state function */
static const cmd_state_action_t cmd_state_table[] = 
{
    state_stx,
    state_cmd,
    state_size,
    state_data_store,
    state_data_pass,
    state_etx,
};

static const cmd_invoke_action_t cmd_invoke[] = 
{
    blk_action,
    img_action,
    str_action,
    clr_action,
    raw_action
};

/* Command parser service component */
static cmd_parser_services_t    *cmd_parser;
static tft_services_t           *tft;
static uart_services_t          *uart;

/* Command State */
static cmd_info_t cmd_info;

/*-----------------------------------------------------------------------------
 *  Helper Functions
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

static cmd_t get_command(void)
{
    return cmd_info.cmd.name;
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
    ASSERT(get_state() == STATE_EXPECT_STX);

    /* If STX found */
    if (byte == CMD_STX)
    {
        set_state(STATE_EXPECT_CMD);
    }
}

static void state_cmd(uint8_t byte)
{
    ASSERT(get_state() == STATE_EXPECT_CMD);

    /* Check whether byte is a valid command byte */
    if (is_cmd_byte(byte))
    {
        /* Store command name and expected data size */
        cmd_info.cmd.name = (cmd_t)byte;
        cmd_info.cmd.size = cmd_table[byte].size;

        /* Flush ring buffer if contain any residual data */
        RingBufFlush(&cmd_info.data_ringbuf_obj);

        /* Change to Data State */
        set_state(STATE_EXPECT_SIZE);
    }
    else
    {
        /* Return back to STATE_EXPECT_STX to start again */
        set_state(STATE_EXPECT_STX);
    }
}

static void state_size(uint8_t byte)
{
    ASSERT(get_state() == STATE_EXPECT_SIZE);

    uint32_t data_size = 0;
    uint8_t temp[3];

    /* Buffer the first 3 byte (MSB first) */
    if (RingBufUsed(&cmd_info.data_ringbuf_obj) < 4)
    {
        RingBufWrite(&cmd_info.data_ringbuf_obj, &byte, 1);
    }
    /* The forth byte */
    {
        RingBufRead(&cmd_info.data_ringbuf_obj, &temp[0], 3);

        data_size |= (((uint32_t)temp[0]) << 24);
        data_size |= (((uint32_t)temp[1]) << 16);
        data_size |= (((uint32_t)temp[2]) << 8);
        data_size |= (((uint32_t)byte) & 0xFF);

        cmd_info.data_size = data_size;

        ASSERT(RingBufEmpty(&cmd_info.data_ringbuf_obj));

        if (data_size > 0)
        {
            if (data_size >= cmd_info.cmd.size)
            {
                /* If command is IMG */
                if (cmd_info.cmd.name == CMD_IMG)
                {
                    set_state(STATE_EXPECT_DATA_PASS);
                }
                else
                {
                    /* Store into buffer if data size is smaller than MAX size */
                    if (data_size < MSG_SIZE)
                    {
                        set_state(STATE_EXPECT_DATA_STORE);
                    }
                    /* If exceed, pass the data instead of storing */
                    else
                    {
                        set_state(STATE_EXPECT_DATA_PASS);
                    }
                }
            }
            else
            {
                /* Receive data size is smaller than minimum size */
                /* Reset back to STATE_EXPECT_STX */
                set_state(STATE_EXPECT_STX);
            }
        }
        /* Jump to STATE_EXPECT_ETX when no data */
        else
        {
            set_state(STATE_EXPECT_ETX);
        }
    }
}

static void state_data_store(uint8_t byte)
{
    ASSERT(get_state() == STATE_EXPECT_DATA_STORE);
    
    /* Store buffer if less than maximum buffer size */
    RingBufWrite(&cmd_info.data_ringbuf_obj, &byte, 1);

    /* Expected data size reached */
    if (RingBufUsed(&cmd_info.data_ringbuf_obj) == cmd_info.data_size)
    {
        set_state(STATE_EXPECT_ETX);
    }
}

static void state_data_pass(uint8_t byte)
{
    ASSERT(get_state() == STATE_EXPECT_DATA_PASS);

    if (cmd_info.cmd.name == CMD_IMG)
    {

    }
}


static void state_etx(uint8_t byte)
{ 
    if (byte == CMD_ETX)
    {
        /* Full packet found */


        /* Execute action */
    }
    

    /* Reset back to STATE_EXPECT_STX for new message packet */
    set_state(STATE_EXPECT_STX);
}

static void blk_action(void)
{
    ASSERT(cmd_info.cmd.name == CMD_BLK);

    /* x0(H) ,x0(L) ,y0(H), y0(L), x1(H), x1(L), y1(H), y1(L),
     * color(H), color(L) */
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
    uint16_t color;
    uint8_t data[MSG_SIZE];

    RingBufRead(&cmd_info.data_ringbuf_obj, &data[0], cmd_info.data_size);

    x0 = convert_to_word(data[BLK_X0_HIGH],
                         data[BLK_X0_LOW]);

    y0 = convert_to_word(data[BLK_Y0_HIGH],
                         data[BLK_Y0_LOW]);

    x1 = convert_to_word(data[BLK_X1_HIGH],
                         data[BLK_X1_LOW]);
    
    y1 = convert_to_word(data[BLK_Y1_HIGH],
                         data[BLK_Y1_LOW]);

    color = convert_to_word(data[BLK_COLOR_HIGH],
                            data[BLK_COLOR_LOW]);

    tft->fill_area(x0, y0, x1, y1, color);
}

static void img_action(void)
{

}

static void raw_action(void)
{

}

static void str_action(void)
{
    ASSERT(cmd_info.cmd.name == CMD_STR);

    /* x(H) ,x(L) ,y(H), y(L), font_size, color,
     * text.... */
    char text[256] = "";
    uint8_t data[MSG_SIZE];

    uint16_t x;
    uint16_t y;
    uint16_t font_size;
    uint16_t color;

    RingBufRead(&cmd_info.data_ringbuf_obj, &data[0], cmd_info.data_size);

    x = convert_to_word(data[STR_X_HIGH],
                        data[STR_X_LOW]);

    y = convert_to_word(data[STR_Y_HIGH],
                        data[STR_Y_LOW]);

    font_size = (uint16_t)data[STR_FONT_SIZE];

    color = convert_to_word(data[STR_COLOR_HIGH],
                            data[STR_COLOR_LOW]);

    /* 
     * Size = total data byte size - 7 
     * 6 = x(H) ,x(L) ,y(H), y(L), font_size, color(H), color(L)
     */
    memcpy(&text[0], &data[STR_TEXT], (cmd_info.data_size - 7));

    tft->draw_string_only(&text[0], x, y, font_size, color);
}

static void clr_action(void)
{
    ASSERT(cmd_info.cmd.name == CMD_CLR);
    
    tft->clear_screen();
}


static cmd_t cmd_parser_get_command()
{
    return cmd_info.cmd.name;
}



static bool cmd_parser_parse(uint8_t byte)
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

static void cmd_parser_process(uint8_t byte)
{
#if 0
    if (cmd_parser_parse(byte))
    {
        /* If full message packet found */

        /* Get command from the packet */
        uint8_t cmd_index = (uint8_t)cmd_parser_get_command();

        /* Invoke Action based on command */ 
        cmd_invoke[cmd_index]();

        is_done = true;
    }
#endif
    uint8_t state = (uint8_t)get_state();

    /* Execute Command Parser */
    cmd_state_table[state](byte);

}

/*-----------------------------------------------------------------------------
 *  Event Callback Functons
 *-----------------------------------------------------------------------------*/

static void pc_data_available_cb(void)
{
    uint8_t read_byte;
    
    uart->read(UART_CMD, &read_byte, 1);

    cmd_parser_process(read_byte);
}

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

static void cmd_parser_start(void)
{
    /* Start UART Service */
    uart->open(UART_CMD, pc_data_available_cb);

    /* Register TFT callback */
    //tft->register_done_callback(tft_done_cb);
}

static void cmd_parser_stop(void)
{
    /* Close UART Service */
    uart->close(UART_CMD);
}

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/
/**
 * Command parser service initialisation
 * @param cmd_parser_services   Command parser service
 * @param uart_services         UART service
 * @param tft_services          TFT service
 */
void cmd_parser_init(cmd_parser_services_t* cmd_parser_services,
                     uart_services_t*       uart_services,
                     tft_services_t*        tft_services)
{
    tft = tft_services;
    uart = uart_services;

    cmd_parser_services->parse = cmd_parser_parse;
    cmd_parser_services->process = cmd_parser_process;
    cmd_parser_services->start = cmd_parser_start;
    cmd_parser_services->stop = cmd_parser_stop;

    /* Command Info Initialisation */
    cmd_info.message_found = false;
    cmd_info.state = STATE_EXPECT_STX;
    cmd_info.data_size = 0;

    cmd_info.cmd.name = MAX_CMD;
    cmd_info.cmd.size = 0;

    RingBufInit(&cmd_info.data_ringbuf_obj,
                &cmd_info.buffer[0],
                sizeof(cmd_info.buffer));
}
