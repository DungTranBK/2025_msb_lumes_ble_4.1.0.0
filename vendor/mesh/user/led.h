/*
 * Copyright (c) 2019
 * Lumi, JSC.
 * All Rights Reserved
 *
 *
 * Description:led.h
 *
 * Author: DungTran
 *
 * Last Changed By:  $Author: DungTran $
 * Revision:         $Revision: 1.0 $
 * Last Changed:     $Date: 12/17/19 $
 */
/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#ifndef LED_H_
#define LED_H_

#include "config_board.h"
#include "tl_common.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

typedef void (*typeLed_HandleRefreshLedCallbackFunc)(uint16_t);

#if (NUMBER_LED_RL == 1)
	#define LED_0RED        PIN_LED_RED_1
	#define LED_0BLUE       PIN_LED_BLUE_1

    #define LED_ARRAY       { {LED_0RED, LED_0BLUE} }
#endif

#if (NUMBER_LED_RL == 2)

	#define LED_0RED        PIN_LED_RED_0
	#define LED_0BLUE       PIN_LED_BLUE_0

	#define LED_1RED        PIN_LED_RED_2
	#define LED_1BLUE       PIN_LED_BLUE_2

    #define LED_ARRAY {  \
            {LED_0RED, LED_0BLUE}, {LED_1RED, LED_1BLUE}  \
        }
#elif (NUMBER_LED_RL == 3)

	#define LED_0RED        PIN_LED_RED_0
	#define LED_0BLUE       PIN_LED_BLUE_0

	#define LED_1RED        PIN_LED_RED_1
	#define LED_1BLUE       PIN_LED_BLUE_1

	#define LED_2RED        PIN_LED_RED_2
	#define LED_2BLUE       PIN_LED_BLUE_2

    #define LED_ARRAY {  \
            {LED_0RED, LED_0BLUE}, {LED_1RED, LED_1BLUE}, \
            {LED_2RED, LED_2BLUE}  \
        }
#endif

enum LedColor_enum{
	LED_COLOR_RED    = 0x00,
	LED_COLOR_GREEN  = 0x01,
	LED_COLOR_BLUE   = 0x02,
	LED_COLOR_PINK   = 0x03,
	LED_COLOR_WHITE  = 0x04,
	LED_COLOR_NONE   = 0x05
};

enum LedColorIdx{
	LED_RED_IDX    = 0x00,
	LED_BLUE_IDX   = 0x01,
	END_LED_IDX    = 0x02
};

typedef uint8_t LedColor_enum;

enum LedLastState_enum{
	LAST_STATE_REFRESH_LED   = 0,
	LAST_STATE_ON_RED        = 1,
	LAST_STATE_ON_BLUE       = 2,
	LAST_STATE_ON_PINK       = 3,
	LAST_STATE_COLOR_NONE    = 4,
	LAST_STATE_REFRESH_LED_DELAY = 5
};
typedef uint8_t LedLastState_enum;

#define COMMAND_LED_DEFAULT  \
{                            \
    LED_MODE_BLINK,          \
    0xFFFF,                  \
    LED_COLOR_RED,           \
    4,                       \
    LAST_STATE_REFRESH_LED,  \
    300,                     \
	0                        \
}

enum BlinkLedStatus_enum{
	NOT_BLINK = 0,
	BLINKING  = 1,
};
typedef uint8_t BlinkLedStatus_enum;

enum LedBlinkFlag_enum{
	BLINK_IDLE   = 0,
	BLINK_ACTIVE = 1,
};
typedef uint8_t LedBlinkFlag_enum;

enum LedMode_enum{
	LED_MODE_OFF = 0,
	LED_MODE_ON = 1,
	LED_MODE_BLINK = 2,
	LED_MODE_BLINK_FOREVER = 3,
	LED_MODE_REFRESH = 4,
	LED_MODE_NONE = 0xFF
};
typedef uint8_t LedMode_enum;


typedef struct {
	LedMode_enum      ledMode;
	uint16_t          ledMask;
	LedColor_enum     ledColor;
	uint8_t           blinkTime;
	LedLastState_enum lastState;
	uint16_t          blinkInterval;
	uint16_t          delayTimeRefresh;
}LedCommand_str;

typedef struct {
	LedColor_enum     toggleLedState;
	uint32_t          toggleledLastTime;
	LedBlinkFlag_enum toggleLedFlag;
	uint8_t           toggleLedTimes;
	LedLastState_enum toggleLedLastState;
	uint32_t          toggleLedInterval;
}LedBlinkInit_str;

typedef struct {
	u32  delay_t;
	u32  delay_start_t;
	u16  led_mask;
}refresh_led_delay_t;

typedef struct
{
	u16 red;
	u16 blue;
}led_display_relay_st_t;

enum {
	LED_L = 0,
	LED_H = 1
};

#define BUF_LED_CMD_SIZE        10

#define DEFAULT_BLINK_INTERVAL	300
#define MIN_BLINK_INTERVAL	    100
#define MAX_BLINK_INTERVAL      500

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void led_set_color(uint8_t ledNbr, LedColor_enum color);
void led_init(void);
void led_refresh_callback_init(typeLed_HandleRefreshLedCallbackFunc refreshLedCallbackInit);
void led_refresh(uint16_t ledMask);
void led_handle_event_function(void);
void led_push_normal_blink_led_cmd_to_fifo(uint8_t blinkTime, LedColor_enum color);
uint8_t led_get_blink_led_flag(void);
uint8_t led_push_led_command_to_fifo(LedCommand_str* ledCmd);
void led_off_all(void);
void led_off(uint8_t ledNbr);
void led_setup_refresh_led_delay(uint16_t delay_t, u16 led_mask);
void led_push_fast_blink_led_cmd_to_fifo(uint8_t blinkTime, LedColor_enum color);

void led_blink_color(u16 ledMask, LedColor_enum  color, u8 blinkTimes, LedLastState_enum lastState,u16 blinkInterval);

#endif

// End File
