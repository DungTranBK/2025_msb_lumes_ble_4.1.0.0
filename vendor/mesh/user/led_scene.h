/*
 * led_scene.h
 *
 *  Created on: Aug 21, 2024
 *      Author: DungTranBK
 */

#ifndef LED_SCENE_H_
#define LED_SCENE_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "config_board.h"
#include "tl_common.h"
#include "led.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define LED_SC_ARRAY { PIN_SC_LED_WHITE_0, PIN_SC_LED_WHITE_1 }

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void led_scene_off(u8 idx);
void led_scene_off_all(void);
void led_scene_on(u8 idx);
void led_scene_on_all(void);
u8 led_scene_push_led_command_to_fifo(LedCommand_str* ledCmd);
void led_scene_push_fast_blink_led_cmd_to_fifo(u16 mask, u8 blinkTime);
void led_scene_push_normal_blink_led_cmd_to_fifo(u16 mask, u8 blinkTime);
void led_scene_init(void);
void led_scene_refresh(uint16_t ledMask);
void led_scene_toggle_handle(void);
void led_scene_handle_event_function(void);
void led_scene_push_blink_led_cmd_with_interval_to_fifo(u16 mask, u8 blinkTime, u16 interval);

#endif /* LED_SCENE_H_ */
