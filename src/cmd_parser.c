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
#include "setting.h"

/*-----------------------------------------------------------------------------
 *  Configuration
 *-----------------------------------------------------------------------------*/

/* Packet Definition */
#define CMD_STX         (2U)
#define CMD_ETX         (3U)

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
    STATE_EXPECT_DATA,
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
    CMD_SQB,
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
    /* Command Definition */
    cmd_definition_t cmd;
    
    /* Command Parser State */
    state_t state;
    
    /* Buffer to store last parsed completed message */
    uint8_t buffer[MSG_SIZE];

    /* Struture encapsulating  */
    tRingBufObject data_ringbuf_obj;

    /* Command Data Size */
    uint32_t data_size;

    /* Command current data size */
    uint32_t current_data;

} cmd_info_t;

/* Parse state */
typedef enum
{
    STATE_PARAM,
    STATE_DATA
} parse_state_t;

/* Raw Action state */
typedef enum
{
    STATE_SEND_CMD,
    STATE_SEND_DATA
} raw_state_t;

/* Function Pointer for Command Parser State Action */
typedef void (*cmd_state_action_t)(uint8_t byte);

/* Function Pointer for Received Command Action */
typedef bool (*cmd_invoke_action_t)(uint8_t byte);

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
static bool blk_action(uint8_t byte);
static bool img_action(uint8_t byte);
static bool str_action(uint8_t byte);
static bool clr_action(uint8_t byte);
static bool raw_action(uint8_t byte);

/* Command Table to store command list with expected minimum data size */
static const cmd_definition_t cmd_table[MAX_CMD] = 
{
    {CMD_BLK, 10},
    {CMD_IMG, 8},
    {CMD_STR, 8},
    {CMD_CLR, 0},
    {CMD_RAW, 0},
    {CMD_SQB, 0}
};

static const cmd_invoke_action_t cmd_invoke[] =
{
    /* CMD_BLK */   blk_action,
    /* CMD_IMG */   img_action,
    /* CMD_STR */   str_action,
    /* CMD_CLR */   clr_action,
    /* CMD_RAW */   raw_action
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

/* Global variable temporary buffer storage */
static uint8_t data[MSG_SIZE];
static char text[256] = "";

/* Command State */
static cmd_info_t       cmd_info;
static parse_state_t    img_state;
static raw_state_t      raw_state;

#ifdef UART_CMD_0
static uart_instance_t uart_type = UART_0;
#endif

#ifdef UART_CMD_1
static uart_instance_t uart_type = UART_1;
#endif

#ifdef UART_CMD_2
static uart_instance_t uart_type = UART_2;
#endif


/*-----------------------------------------------------------------------------
 *  Helper Functions
 *-----------------------------------------------------------------------------*/
/**
 * @brief   Get the current command parser state 
 * @return  Current Command Parser State
 */
static state_t get_state(void)
{
    return cmd_info.state;
}

/**
 * @brief   Set command parser state to current state
 * @param   state   Desired command parser state
 */
static void set_state(state_t state)
{
    cmd_info.state = state;
}

/**
 * @brief   Check whether the byte is a valid command byte
 * @param   byte     Input byte
 * @return  True if it is a valid command byte, Otherwise, False
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

/**
 * @brief   State Expect STX byte
 * @param   byte    received byte
 */
static void state_stx(uint8_t byte)
{
    ASSERT(get_state() == STATE_EXPECT_STX);

    /* If STX found */
    if (byte == CMD_STX)
    {
        set_state(STATE_EXPECT_CMD);
    }
}

/**
 * @brief   State Expect Command byte
 * @param   byte    received byte
 */
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

/**
 * @brief   State Expect STX byte
 * @param   byte    received byte
 */
static void state_size(uint8_t byte)
{
    ASSERT(get_state() == STATE_EXPECT_SIZE);

    uint32_t data_size = 0;
    uint8_t temp[3];

    /* Buffer the first 3 byte (MSB first) */
    if (RingBufUsed(&cmd_info.data_ringbuf_obj) < 3)
    {
        RingBufWrite(&cmd_info.data_ringbuf_obj, &byte, 1);
    }
    /* The forth byte */
    else
    {
        RingBufRead(&cmd_info.data_ringbuf_obj, &temp[0], 3);
        ASSERT(RingBufEmpty(&cmd_info.data_ringbuf_obj));

        data_size |= (((uint32_t)temp[0]) << 24);
        data_size |= (((uint32_t)temp[1]) << 16);
        data_size |= (((uint32_t)temp[2]) << 8);
        data_size |= (((uint32_t)byte) & 0xFF);

        cmd_info.data_size = data_size;

        if (data_size > 0)
        {
            if (data_size >= cmd_info.cmd.size)
            {
                set_state(STATE_EXPECT_DATA);

                /* Clear current read data size */
                cmd_info.current_data = 0;
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
            /* Execute action */
            if (cmd_invoke[(uint8_t)(cmd_info.cmd.name)](byte))
            {
                set_state(STATE_EXPECT_ETX);
            }
        }
    }
}

/**
 * @brief   State Expect Data byte
 * @param   byte    received byte
 */
static void state_data(uint8_t byte)
{
    ASSERT(get_state() == STATE_EXPECT_DATA);

    cmd_info.current_data++;

    /* Execute action */
    if (cmd_invoke[(uint8_t)(cmd_info.cmd.name)](byte))
    {
        set_state(STATE_EXPECT_ETX);
    }
}

/**
 * @brief   State Expect ETX byte
 * @param   byte    received byte
 */
static void state_etx(uint8_t byte)
{ 
    /* Reset back to STATE_EXPECT_STX for new message packet */
    set_state(STATE_EXPECT_STX);
}

/**
 * @brief   Block Action (Fill Rectangle Command)
 * @param   byte    received byte
 * @return  True if the received byte is the last data byte
 */
static bool blk_action(uint8_t byte)
{
    ASSERT(cmd_info.cmd.name == CMD_BLK);

    /* x0(H) ,x0(L) ,y0(H), y0(L), x1(H), x1(L), y1(H), y1(L),
     * color(H), color(L) */
    bool last_data = false;
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
    uint16_t color;

    /* Buffer the param byte */
    RingBufWrite(&cmd_info.data_ringbuf_obj, &byte, 1);

    /* Check whether complete packet is received */
    if (RingBufUsed(&cmd_info.data_ringbuf_obj) == cmd_info.data_size)
    {
        RingBufRead(&cmd_info.data_ringbuf_obj, &data[0], cmd_info.data_size);
        ASSERT(RingBufEmpty(&cmd_info.data_ringbuf_obj));

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
    
        tft_fill_area(x0, y0, x1, y1, color);

        last_data = true;

    }

    return last_data;
}

/**
 * @brief   Image Action (Draw Image Command)
 * @param   byte    received byte
 * @return  True if the received byte is the last data byte
 */
static bool img_action(uint8_t byte)
{
    /* x(H), x(L), y(H), y(L), h(H), h(L), w(H), w(L), 16bit-pixel */

    bool last_data = false;
    uint16_t height = 0;
    uint16_t width = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t temp[8];

    /*
     * Sending Image Pixel
     * Place it as the first condition for better optimization
     */
    if (img_state == STATE_DATA)
    {
        /* Transfer pixel information */
        tft_send_data_only(byte);
    }
    /* Getting Image Parameter */
    else
    {
        /* Buffer the first 7 byte (MSB first) */
        if (RingBufUsed(&cmd_info.data_ringbuf_obj) < 7)
        {
            RingBufWrite(&cmd_info.data_ringbuf_obj, &byte, 1);
        }
        else
        /* The 8th byte */
        {
            RingBufRead(&cmd_info.data_ringbuf_obj, &temp[0], 7);
            ASSERT(RingBufEmpty(&cmd_info.data_ringbuf_obj));

            /* Get starting x and y position */
            x |= (((uint16_t)temp[0]) << 8);
            x |= (((uint16_t)temp[1]) & 0xFF);
            y |= (((uint16_t)temp[2]) << 8);
            y |= (((uint16_t)temp[3]) & 0xFF);

            /* Get Height and Width */
            height |= (((uint16_t)temp[4]) << 8);
            height |= (((uint16_t)temp[5]) & 0xFF);
            width |= (((uint16_t)temp[6]) << 8);
            width |= (((uint16_t)byte) & 0xFF);

            /* Change to Image Pixel State */
            img_state = STATE_DATA;

            /* Set the area to be filled */
            uint16_t x1 = x + width - 1;
            uint16_t y1 = y + height - 1;

            /* Start image transaction by setting area boundary */
            tft_start_image_transfer(x, y, x1, y1);

        }
    }

    /* Check if it's last byte */
    if (cmd_info.current_data == cmd_info.data_size)
    {
        img_state = STATE_PARAM;
        tft_done_transfer();

        last_data = true;
    }

    return last_data;
}

/**
 * @brief   Raw Action (Send Raw TFT Command)
 * @param   byte    received byte
 * @return  True if the received byte is the last data byte
 */
static bool raw_action(uint8_t byte)
{
    bool last_data = false;

    /* RAW, data... */
    if (raw_state == STATE_SEND_CMD)
    {
        raw_state = STATE_SEND_DATA;

        /* Transfer raw command */
        tft_send_command(byte);
    }
    /* Data Byte */
    else
    {
        /* Transfer raw data*/
        tft_send_data(byte);
    }

    /* Check if it's last byte */
    if (cmd_info.current_data == cmd_info.data_size)
    {
        last_data = true;
        raw_state = STATE_SEND_CMD;
    }

    return last_data;

}

/**
 * @brief   String Action (Draw string Command)
 * @param   byte    received byte
 * @return  True if the received byte is the last data byte
 */
static bool str_action(uint8_t byte)
{
    ASSERT(cmd_info.cmd.name == CMD_STR);

    /* x(H) ,x(L) ,y(H), y(L), font_size, color(H), color(L), text.... */

    bool last_data = false;
    uint16_t x;
    uint16_t y;
    uint16_t font_size;
    uint16_t color;
    uint8_t text_size = cmd_info.data_size - 7;

    /* Buffer the param byte */
    RingBufWrite(&cmd_info.data_ringbuf_obj, &byte, 1);

    /* Check whether complete packet is received */
    if (RingBufUsed(&cmd_info.data_ringbuf_obj) == cmd_info.data_size)
    {
        RingBufRead(&cmd_info.data_ringbuf_obj, &data[0], cmd_info.data_size);
        ASSERT(RingBufEmpty(&cmd_info.data_ringbuf_obj));

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
        memcpy(&text[0], &data[STR_TEXT], text_size);

        /* Set Null termination at the last character */
        text[text_size] = 0;

        tft_draw_string_only(&text[0], x, y, font_size, color);

        last_data = true;
    }

    return last_data;
}

/**
 * @brief   Clear Action (Clear screen Command)
 * @param   byte    received byte
 * @return  True if the received byte is the last data byte
 */
static bool clr_action(uint8_t byte)
{
    ASSERT(cmd_info.cmd.name == CMD_CLR);
    ASSERT(byte == 0);
    
    tft_clear_screen();

    return true;
}

/**
 * @brief   Process Command Parser by invoking state table based on current
 *          parser state
 * @param   byte    received byte
 */
static void cmd_parser_process(uint8_t byte)
{
    uint8_t state = (uint8_t)get_state();

    /* Execute Command Parser */
    cmd_state_table[state](byte);
}

/*-----------------------------------------------------------------------------
 *  Event Callback Functons
 *-----------------------------------------------------------------------------*/

/**
 * @brief   Callback action when it received client data (PC)
 */
static void pc_data_available_cb(void)
{
    uint8_t read_byte;
    
    uart_read(uart_type, &read_byte, 1);

    cmd_parser_process(read_byte);
}

/*-----------------------------------------------------------------------------
 *  Services
 *-----------------------------------------------------------------------------*/

/**
 * @brief   Start command parser service by opening UART port
 */
void cmd_parser_start(void)
{
    /* Start UART Service */
    uart_open(uart_type, pc_data_available_cb);

    /* Register TFT callback */
    //tft_register_done_callback(tft_done_cb);
}

/**
 * @brief   Stop command parser service by closing UART port
 */
void cmd_parser_stop(void)
{
    /* Close UART Service */
    uart_close(uart_type);
}

/*-----------------------------------------------------------------------------
 *  Initialisation
 *-----------------------------------------------------------------------------*/
/**
 * @brief   Command Parser Initialization
 */
void cmd_parser_init(void)
{
    /* Command Info Initialisation */
    cmd_info.state = STATE_EXPECT_STX;
    cmd_info.data_size = 0;
    cmd_info.current_data = 0;

    cmd_info.cmd.name = MAX_CMD;
    cmd_info.cmd.size = 0;

    /* Image state */
    img_state = STATE_PARAM;

    /* Raw state */
    raw_state = STATE_SEND_CMD;

    RingBufInit(&cmd_info.data_ringbuf_obj,
                &cmd_info.buffer[0],
                sizeof(cmd_info.buffer));

}
