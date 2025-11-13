/*
 * led_scene.c
 *
 *  Created on: Aug 21, 2024
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj/tl_common.h"
#include "vendor/common/system_time.h"
#include "utilities.h"
#include "fifo.h"
#include "led_scene.h"

#include "debug.h"
#ifdef  LED_SC_DBG_EN
	#define DBG_LED_SC_SEND_STR(x)   Dbg_sendString((s8*)x)
	#define DBG_LED_SC_SEND_INT(x)   Dbg_sendInt(x)
	#define DBG_LED_SC_SEND_HEX(x)   Dbg_sendHex(x)
	#define DBG_LED_SC_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
#else
	#define DBG_LED_SC_SEND_STR(x)
	#define DBG_LED_SC_SEND_INT(x)
	#define DBG_LED_SC_SEND_HEX(x)
	#define DBG_LED_SC_SEND_BYTE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
const u16 led_scene_arr[NUMBER_LED_SCENE] = LED_SC_ARRAY;

static LedBlinkInit_str toggleLedArr[NUMBER_LED_SCENE];
static LedCommand_str cmdIsRunning = COMMAND_LED_DEFAULT;
static Fifo_t fifoLedCommands;
static LedCommand_str bufferLedCommands[BUF_LED_CMD_SIZE];

/******************************************************************************/
/*                        PRIVATE FUNCTIONS DECLERATION                       */
/******************************************************************************/
#define SC_LED_STATE(x)   gpio_read_output(led_scene_arr[x])

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

/**
 * @func   led_off
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_scene_off(u8 idx)
{
	gpio_write(led_scene_arr[idx], LED_L);
}

/**
 * @func   led_off_all
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_scene_off_all(void)
{
	foreach(i, NUMBER_LED_SCENE) {
		led_scene_off(i);
	}
}

/**
 * @func   led_off
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_scene_on(u8 idx)
{
	gpio_write(led_scene_arr[idx], LED_H);
}

/**
 * @func   led_off_all
 * @brief
 * @param  BYTE: Led Number
 * @retval None
 */
void led_scene_on_all(void)
{
	foreach(i, NUMBER_LED_SCENE) {
		led_scene_on(i);
	}
}
/**
 * @func   led_scene_push_led_command_to_fifo
 * @brief  Push led command to led fifo buffer
 * @param  LED_CmdTypeDef
 * @retval None
 */
u8 led_scene_push_led_command_to_fifo(LedCommand_str* ledCmd)
{
	if(FifoPush(&fifoLedCommands, ledCmd))
		return true;
	return false;
}

/**
 * @func   led_scene_push_blink_led_cmd_with_interval_to_fifo
 * @brief  None
 * @param
 * @retval None
 */
void led_scene_push_blink_led_cmd_with_interval_to_fifo(u16 mask, u8 blinkTime, u16 interval)
{
	LedCommand_str ledCmd = COMMAND_LED_DEFAULT;
	ledCmd.ledMask = mask;
	ledCmd.blinkTime = blinkTime << 1;
	ledCmd.blinkInterval = interval;
	(void)led_scene_push_led_command_to_fifo(&ledCmd);
}

/**
 * @func   led_scene_push_fast_blink_led_cmd_to_fifo
 * @brief  None
 * @param
 * @retval None
 */
void led_scene_push_fast_blink_led_cmd_to_fifo(u16 mask, u8 blinkTime)
{
	LedCommand_str ledCmd = COMMAND_LED_DEFAULT;
	ledCmd.ledMask = mask;
	ledCmd.blinkTime = blinkTime << 1;
	ledCmd.blinkInterval = 200;
	(void)led_scene_push_led_command_to_fifo(&ledCmd);
}
/**
 * @func   led_scene_push_normal_blink_led_cmd_to_fifo
 * @brief  None
 * @param
 * @retval None
 */
void led_scene_push_normal_blink_led_cmd_to_fifo(u16 mask, u8 blinkTime)
{
	LedCommand_str ledCmd = COMMAND_LED_DEFAULT;
	ledCmd.ledMask = mask;
	ledCmd.blinkTime = blinkTime << 1;
	(void)led_scene_push_led_command_to_fifo(&ledCmd);
}
/**
 * @func   led_init
 * @brief  Initialization for led module
 * @param  None
 * @retval None
 */
void led_scene_init(void)
{
	for(u8 i = 0; i < NUMBER_LED_SCENE; i++)
	{
		gpio_set_func(led_scene_arr[i], AS_GPIO);
		gpio_set_output_en(led_scene_arr[i], 1);
		toggleLedArr[i].toggleLedInterval = DEFAULT_BLINK_INTERVAL;
		toggleLedArr[i].toggleLedFlag = BLINK_IDLE;
	}
	led_scene_off_all();
	FifoInit(&fifoLedCommands, &bufferLedCommands, sizeof(LedCommand_str), BUF_LED_CMD_SIZE);
}

/**
 * @func   led_scene_refresh
 * @brief  Handle function to refresh led
 * @param  BYTE: ledMask
 * @retval None
 */
void led_scene_refresh(uint16_t ledMask)
{
	led_scene_off_all();
}
/**
 * @func   led_toggle
 * @brief  Inverting the led state
 * @param  BYTE: Led Number
 * @retval None
 */
static void led_scene_toggle(uint8_t idx)
{
	if(idx < NUMBER_LED_SCENE) {
		gpio_toggle(led_scene_arr[idx]);
	}
}
/**
 * @func   led_toggle_handle
 * @brief  Handle Blink Led
 * @param  None
 * @retval None
 */
void led_scene_toggle_handle(void)
{
	u8 i;
	static u32 ledBlinkScanTimer = 0;
	if(clock_time_exceed_ms(ledBlinkScanTimer, TIMER_10MS) > 0){
		for(i=0; i<= NUMBER_LED_SCENE; i++){
			if(toggleLedArr[i].toggleLedFlag == BLINK_ACTIVE) {
				if(toggleLedArr[i].toggleLedTimes > 0) {
					if(clock_time_exceed_ms(toggleLedArr[i].toggleledLastTime,
							toggleLedArr[i].toggleLedInterval) > 0) {
						toggleLedArr[i].toggleledLastTime = clock_time_ms();
						toggleLedArr[i].toggleLedTimes--;
						if(toggleLedArr[i].toggleLedTimes == 0) {
							toggleLedArr[i].toggleLedFlag = BLINK_IDLE;
							if(FifoIsEmpty(&fifoLedCommands) == true) {
								led_scene_refresh(0xFFFF);
							}
						}
						else {
							led_scene_toggle(i);
						}
					}
				}
				else {
					toggleLedArr[i].toggleLedFlag = BLINK_IDLE;
				}
			}
		}
		ledBlinkScanTimer = clock_time_ms();
	}
}

/**
 * @func   led_scene_blink
 * @brief  Change all led blink parameters
 * @param  BYTE: ledMask, blinkTimes, lastState, ledColor
 * @retval None
 */
static void led_scene_blink(uint16_t ledMask, uint8_t blinkTimes, uint16_t blinkInterval)
{
	for(u8 i = 0; i < NUMBER_LED_SCENE; i++) {
		if(((ledMask>>i)&0x01) == 1){
			led_scene_on(i);
			toggleLedArr[i].toggleledLastTime  = clock_time_ms();
			toggleLedArr[i].toggleLedFlag      = BLINK_ACTIVE;
			toggleLedArr[i].toggleLedTimes     = blinkTimes;
			toggleLedArr[i].toggleLedInterval  = blinkInterval;
		}
	}
}

/**
 * @func   led_scene_handle_event_function
 * @brief  Handle led Command if blink flag = BLINK_IDLE (No Command is running)
 * @param  None
 * @retval None
 */
void led_scene_handle_event_function(void)
{
	u8 i;
	LedBlinkFlag_enum blinkFlag = BLINK_IDLE;
	for(i = 0; i < NUMBER_LED_SCENE; i++){
		if(toggleLedArr[i].toggleLedFlag == 1){
			blinkFlag = BLINK_ACTIVE;
			break;
		}
	}
	if(blinkFlag == BLINK_IDLE){
		// Check FIFO Led
		if (FifoIsEmpty(&fifoLedCommands) == false){
			if(FifoPop(&fifoLedCommands, &cmdIsRunning)){
				if(cmdIsRunning.ledMode == LED_MODE_REFRESH){
					led_scene_refresh(0xFFFF);
				}else if(cmdIsRunning.ledMode == LED_MODE_BLINK){
					led_scene_blink(
							cmdIsRunning.ledMask,
							cmdIsRunning.blinkTime,
							cmdIsRunning.blinkInterval
						);
				}else if(cmdIsRunning.ledMode == LED_MODE_ON){
					if(cmdIsRunning.ledMask != 0){
						for(i=0; i < NUMBER_LED_SCENE; i++){
							if(((cmdIsRunning.ledMask >> i)&0x01) == 1){
								led_scene_on(i);
							}
						}
					}
				}else if(cmdIsRunning.ledMode == LED_MODE_OFF){
					if(cmdIsRunning.ledMask != 0){
						for(i=0; i < NUMBER_LED_SCENE; i++){
							if(((cmdIsRunning.ledMask >> i)&0x01) == 1){
								led_off(i);
							}
						}
					}
				}
			}
		}
	}
	led_scene_toggle_handle();
}
// End
