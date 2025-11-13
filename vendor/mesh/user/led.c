/*
 * Copyright (c) 2019
 * Lumi, JSC.
 * All Rights Reserved
 *
 *
 * Description:
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
#include "proj/tl_common.h"
#include "vendor/common/system_time.h"
#include "utilities.h"
#include "fifo.h"
#include "led.h"

#include "debug.h"
#ifdef  LED_DBG_EN
	#define DBG_LED_SEND_STR(x)   Dbg_sendString((s8*)x)
	#define DBG_LED_SEND_INT(x)   Dbg_sendInt(x)
	#define DBG_LED_SEND_HEX(x)   Dbg_sendHex(x)
	#define DBG_LED_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
#else
	#define DBG_LED_SEND_STR(x)
	#define DBG_LED_SEND_INT(x)
	#define DBG_LED_SEND_HEX(x)
	#define DBG_LED_SEND_BYTE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
typeLed_HandleRefreshLedCallbackFunc pvLed_HandleRefreshLed = NULL;

const u16 ledAddr[NUMBER_LED_RL][2] = LED_ARRAY;

static LedBlinkInit_str toggleLedArr[NUMBER_LED_RL];
static LedCommand_str cmdIsRunning = COMMAND_LED_DEFAULT;
static Fifo_t fifoLedCommands;
static LedCommand_str bufferLedCommands[BUF_LED_CMD_SIZE];

static refresh_led_delay_t refresh_led_delay_st = { 0, 0, 0xFFFF };

/******************************************************************************/
/*                        PRIVATE FUNCTIONS DECLERATION                       */
/******************************************************************************/
static void led_set_color_red(uint8_t ledNbr);
static void led_set_color_blue(uint8_t ledNbr);
static void led_set_color_pink(uint8_t ledNbr);
static void led_handle_refresh_delay(void);

#define CFG_LED_RED_STATE(x)    gpio_read_output(ledAddr[x][LED_RED_IDX])
#define CFG_LED_BLUE_STATE(x)   gpio_read_output(ledAddr[x][LED_BLUE_IDX])

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

/**
 * @func   led_blink_led_red
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_blink_led_red(u8 idx)
{
	gpio_write(ledAddr[idx][LED_BLUE_IDX], 0);
	gpio_toggle(ledAddr[idx][LED_RED_IDX]);
}

/**
 * @func   led_blink_led_blue
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_blink_led_blue(u8 idx)
{
	gpio_write(ledAddr[idx][LED_RED_IDX], 0);
	gpio_toggle(ledAddr[idx][LED_BLUE_IDX]);
}

/**
 * @func   LED_blinkPink
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_blink_led_pink(u8 idx)
{
	if(CFG_LED_RED_STATE(idx) == CFG_LED_BLUE_STATE(idx))
	{
		gpio_toggle(ledAddr[idx][LED_RED_IDX]);
		gpio_toggle(ledAddr[idx][LED_BLUE_IDX]);
	}
	else
	{
		gpio_toggle(ledAddr[idx][LED_RED_IDX]);
	}
}


/**
 * @func   led_set_color
 * @brief
 * @param  BYTE: Led Number
 *         BYTE: Led Color
 * @retval None
 */
void led_set_color(uint8_t ledNbr, LedColor_enum color)
{
	if(ledNbr < NUMBER_LED_RL)
	{
		switch(color){
		case LED_COLOR_RED:
			led_set_color_red(ledNbr);
		    break;

		case LED_COLOR_BLUE:
			led_set_color_blue(ledNbr);
			break;

		case LED_COLOR_PINK:
			led_set_color_pink(ledNbr);
			break;

		case LED_COLOR_NONE:
			led_off(ledNbr);
			break;
		}
	}
}
/**
 * @func   led_set_color_last_state
 * @brief
 * @param  BYTE: Led Number
 *         BYTE: Led Color
 * @retval None
 */
static void led_set_color_last_state(uint8_t ledNbr, LedColor_enum color)
{
	if(ledNbr < NUMBER_LED_RL)
	{
		switch(color)
		{
		case LAST_STATE_ON_RED:
			led_set_color_red(ledNbr);
		    break;

		case LAST_STATE_ON_BLUE:
			led_set_color_blue(ledNbr);
			break;

		case LAST_STATE_ON_PINK:
			led_set_color_pink(ledNbr);
			break;

		case LAST_STATE_COLOR_NONE:
			led_off(ledNbr);
			break;
		}
	}
}

/**
 * @func   led_set_color_red
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
static void led_set_color_red(u8 idx)
{
	if(idx < NUMBER_LED_RL)
	{
		foreach(i, 2)
		{
			if(i == LED_RED_IDX)
			{
				gpio_write(ledAddr[idx][i], LED_H);
			}
			else
			{
				gpio_write(ledAddr[idx][i], LED_L);
			}
		}
	}
}

/**
 * @func   led_set_color_blue
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
static void led_set_color_blue(u8 idx)
{
	if(idx < NUMBER_LED_RL)
	{
		foreach(i, 2)
		{
			if(i == LED_BLUE_IDX)
			{
				gpio_write(ledAddr[idx][i], LED_H);
			}
			else
			{
				gpio_write(ledAddr[idx][i], LED_L);
			}
		}
	}
}

/**
 * @func   LED_setColorGreen
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
static void led_set_color_pink(u8 idx)
{
	if(idx < NUMBER_LED_RL)
	{
		gpio_write(ledAddr[idx][LED_RED_IDX], LED_H);
		gpio_write(ledAddr[idx][LED_BLUE_IDX], LED_H);
	}
}

/**
 * @func   led_off
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_off(u8 idx)
{
	gpio_write(ledAddr[idx][LED_RED_IDX], LED_L);
	gpio_write(ledAddr[idx][LED_BLUE_IDX], LED_L);
}
/**
 * @func   led_off_all
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_off_all(void)
{
	foreach(i, NUMBER_LED_RL) {
		led_off(i);
	}
}

/**
 * @func   led_push_led_command_to_fifo
 * @brief  Push led command to led fifo buffer
 * @param  LED_CmdTypeDef
 * @retval None
 */
uint8_t led_push_led_command_to_fifo(LedCommand_str* ledCmd)
{
	if(FifoPush(&fifoLedCommands, ledCmd))
		return true;
	return false;
}

/**
 * @func   led_push_fast_blink_led_cmd_to_fifo
 * @brief  None
 * @param
 * @retval None
 */
void led_push_fast_blink_led_cmd_to_fifo(uint8_t blinkTime, LedColor_enum color)
{
	LedCommand_str ledCmd = COMMAND_LED_DEFAULT;
	ledCmd.ledColor = color;
	ledCmd.blinkTime = blinkTime;
	ledCmd.blinkInterval = 180;
	(void)led_push_led_command_to_fifo(&ledCmd);
}
/**
 * @func   led_push_normal_blink_led_cmd_to_fifo
 * @brief  None
 * @param
 * @retval None
 */
void led_push_normal_blink_led_cmd_to_fifo(uint8_t blinkTime, LedColor_enum color)
{
	LedCommand_str ledCmd = COMMAND_LED_DEFAULT;
	ledCmd.ledColor = color;
	ledCmd.blinkTime = blinkTime << 1;
	(void)led_push_led_command_to_fifo(&ledCmd);
}
/**
 * @func   led_init
 * @brief  Initialization for led module
 * @param  None
 * @retval None
 */
void led_init(void)
{
	for(uint8_t i = 0; i < NUMBER_LED_RL; i++)
	{
	    for(u8 j = 0; j < 2; j++)
	    {
	    	gpio_set_func(ledAddr[i][j], AS_GPIO);
	        gpio_set_output_en(ledAddr[i][j], 1);
	    }
		toggleLedArr[i].toggleLedInterval = DEFAULT_BLINK_INTERVAL;
		toggleLedArr[i].toggleLedFlag = BLINK_IDLE;
	}
	led_off_all();
	FifoInit(&fifoLedCommands, &bufferLedCommands, sizeof(LedCommand_str), BUF_LED_CMD_SIZE);
}
/**
 * @func   led_refresh_callback_init
 * @brief  Initialization handle function to refresh led
 * @param  Function pointer
 * @retval None
 */
void led_refresh_callback_init(typeLed_HandleRefreshLedCallbackFunc refreshLedCallbackInit)
{
	if(refreshLedCallbackInit != NULL)
	{
		pvLed_HandleRefreshLed = refreshLedCallbackInit;
	}
}

/**
 * @func   led_refresh
 * @brief  Handle function to refresh led
 * @param  BYTE: ledMask
 * @retval None
 */
void led_refresh(uint16_t ledMask)
{
	if(pvLed_HandleRefreshLed != NULL)
	{
		pvLed_HandleRefreshLed(ledMask);
	}
}
/**
 * @func   led_toggle
 * @brief  Inverting the led state
 * @param  BYTE: Led Number
 * @retval None
 */
void led_toggle(uint8_t ledNbr)
{
	if(ledNbr <= NUMBER_LED_RL)
	{
		switch(toggleLedArr[ledNbr].toggleLedState)
		{
			case LED_COLOR_RED:
			{
				led_blink_led_red(ledNbr);
				break;
			}
			case LED_COLOR_BLUE:
			{
				led_blink_led_blue(ledNbr);
				break;
			}
			case LED_COLOR_PINK:
			{
				led_blink_led_pink(ledNbr);
				break;
			}
			default: // Unknown Led Color
				break;
		}
	}
}
/**
 * @func   led_toggle_handle
 * @brief  Handle Blink Led
 * @param  None
 * @retval None
 */
void led_toggle_handle(void)
{
	uint8_t i;
	uint8_t enableRefreshLed = false;

	for(i=0; i<= NUMBER_LED_RL; i++){
		if(toggleLedArr[i].toggleLedFlag == BLINK_ACTIVE){
			if(toggleLedArr[i].toggleLedTimes > 0){
				if(clock_time_exceed_ms(toggleLedArr[i].toggleledLastTime,toggleLedArr[i].toggleLedInterval) > 0){
					toggleLedArr[i].toggleledLastTime = clock_time_ms();
					toggleLedArr[i].toggleLedTimes--;
					if(toggleLedArr[i].toggleLedTimes == 0){
						toggleLedArr[i].toggleLedFlag = BLINK_IDLE;
						switch(toggleLedArr[i].toggleLedLastState)
						{
							case LAST_STATE_ON_RED:
							case LAST_STATE_ON_BLUE:
							case LAST_STATE_COLOR_NONE:
								led_set_color_last_state(i, toggleLedArr[i].toggleLedLastState);
								break;
							case LAST_STATE_REFRESH_LED:
							default:
								enableRefreshLed = true;
								break;
						}

					}else{
						led_toggle(i);
					}
				}
			}else{
				toggleLedArr[i].toggleLedFlag = BLINK_IDLE;
			}
		}
	}
	if(enableRefreshLed == true){
		led_refresh(0xFFFF);
		DBG_LED_SEND_STR("\n LED - Refresh Led");
	}
}

/**
 * @func   led_Blink
 * @brief  Change all led blink parameters
 * @param  BYTE: ledMask, blinkTimes, lastState, ledColor
 * @retval None
 */
void led_blink(uint16_t ledMask, uint8_t blinkTimes,LedLastState_enum lastState,       \
		             LedColor_enum ledColor, uint16_t blinkInterval)
{
	for(uint8_t i = 0; i < NUMBER_LED_RL; i++){
		if(((ledMask>>i)&0x01) == 1){
			led_off(i);
			toggleLedArr[i].toggleLedState     = ledColor;
			toggleLedArr[i].toggleledLastTime  = clock_time_ms();
			toggleLedArr[i].toggleLedFlag      = BLINK_ACTIVE;
			toggleLedArr[i].toggleLedTimes     = blinkTimes;
			toggleLedArr[i].toggleLedLastState = lastState;
			toggleLedArr[i].toggleLedInterval  = blinkInterval;
		}
		else
		{
			if(ledColor == LED_COLOR_PINK   \
					&& lastState == LAST_STATE_REFRESH_LED) {
				led_off(i);
			}
		}
	}
}

/**
 * @func   led_get_blink_led_flag
 * @brief
 * @param  None
 * @retval None
 */
uint8_t led_get_blink_led_flag(void)
{
	LedBlinkFlag_enum blinkFlag = BLINK_IDLE;
	for(u8 i = 0; i < NUMBER_LED_RL; i++){
		if(toggleLedArr[i].toggleLedFlag == BLINK_ACTIVE){
			blinkFlag = BLINK_ACTIVE;
			break;
		}
	}
	return blinkFlag;
}

/**
 * @func   led_handle_event_function
 * @brief  Handle led Command if blink flag = BLINK_IDLE (No Command is running)
 * @param  None
 * @retval None
 */
void led_handle_event_function(void)
{
	static u32 ledHandleScanTimer = 0;
	if(clock_time_exceed_ms(ledHandleScanTimer, TIMER_10MS))
	{
		u8 i;
		LedBlinkFlag_enum blinkFlag = BLINK_IDLE;
		for(i = 0; i < NUMBER_LED_RL; i++){
			if(toggleLedArr[i].toggleLedFlag == 1){
				blinkFlag = BLINK_ACTIVE;
				break;
			}
		}
		if(blinkFlag == BLINK_IDLE){
			// Refresh delay
			led_handle_refresh_delay();
			// Check FIFO Led
			if (FifoIsEmpty(&fifoLedCommands) == false){
				if(FifoPop(&fifoLedCommands, &cmdIsRunning)){
					if(cmdIsRunning.ledMode == LED_MODE_REFRESH){
						if(pvLed_HandleRefreshLed != NULL){
							pvLed_HandleRefreshLed(cmdIsRunning.ledMask);
						}
					}else if(cmdIsRunning.ledMode == LED_MODE_BLINK){
						led_blink(cmdIsRunning.ledMask, cmdIsRunning.blinkTime, \
								cmdIsRunning.lastState, cmdIsRunning.ledColor, cmdIsRunning.blinkInterval);
					}else if(cmdIsRunning.ledMode == LED_MODE_ON){
						switch(cmdIsRunning.ledColor){
							case LED_COLOR_RED:
							case LED_COLOR_BLUE:
							case LED_COLOR_PINK:
								if(cmdIsRunning.ledMask != 0){
									for(i=0; i < NUMBER_LED_RL; i++){
										if(((cmdIsRunning.ledMask >> i)&0x01) == 1){
											led_set_color(i, cmdIsRunning.ledColor);
										}
									}
								}
								break;
							default:
								//Unknown led color, so do not change the led color
								break;
							}
					}else if(cmdIsRunning.ledMode == LED_MODE_OFF){
						if(cmdIsRunning.ledMask != 0){
							for(i=0; i < NUMBER_LED_RL; i++){
								if(((cmdIsRunning.ledMask >> i)&0x01) == 1){
									led_off(i);
								}
							}
						}
					}
				}
			}
		}
		led_toggle_handle();
		// Reset timer
		ledHandleScanTimer = clock_time_ms();
	}
}

/**
 * @func   led_handle_refresh_delay
 * @brief
 * @param  None
 * @retval None
 */
static void led_handle_refresh_delay(void)
{
	if(refresh_led_delay_st.delay_t != 0){
	    if(clock_time_get_elapsed_time(refresh_led_delay_st.delay_start_t) > refresh_led_delay_st.delay_t){
	    	led_refresh(refresh_led_delay_st.led_mask);
	    	refresh_led_delay_st.delay_t = 0;
	    }
	}
}

/**
 * @func   LED_setupRefreshLedDelay
 * @brief
 * @param  None
 * @retval None
 */
void LED_setupRefreshLedDelay(uint16_t delay_t, u16 led_mask)
{
	refresh_led_delay_st.delay_t = delay_t;
	refresh_led_delay_st.led_mask = led_mask;
	refresh_led_delay_st.delay_start_t = clock_time_ms();
}

/**
 * @func   led_blink_color
 * @brief
 * @param  None
 * @retval None
 */
void led_blink_color(u16 ledMask,
			   LedColor_enum  color,
		       u8 blinkTimes,
			   LedLastState_enum lastState,
			   u16 blinkInterval)
{
	LedCommand_str led_cmd = COMMAND_LED_DEFAULT;
	led_cmd.ledMask = ledMask;
	led_cmd.ledColor = color;
	led_cmd.blinkTime = blinkTimes << 1;
	led_cmd.lastState = lastState;
	led_cmd.blinkInterval = blinkInterval;
	led_push_led_command_to_fifo(&led_cmd);
}

// End File
