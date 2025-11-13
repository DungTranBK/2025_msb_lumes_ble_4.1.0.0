/*
 * led_ev.h
 *
 *  Created on: Sep 21, 2020
 *      Author: DungTran BK
 */

#ifndef LED_EV_H_
#define LED_EV_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "../../../proj/tl_common.h"
/******************************************************************************/
/*                       EXPORT TYPE AND DEFINITION                           */
/******************************************************************************/

typedef enum {
	LED_POWER_ON,
	LED_OTA_FAIL,
	LED_OTA_SUCESS,
	LED_PROVISION_FAIL,
	LED_PROVISION_SUCCESS,
	LED_SUC_ADD_APPKEY,
	LED_FAIL_ADD_APPKEY,
	LED_CMD_SET_SUBSCRIPTION,
	LED_CMD_DEL_SUBSCRIPTION,
	LED_CMD_SET_SCENE,
	LED_CMD_DEL_SCENE,
	LED_CMD_BINDING_ENABLE,
	LED_CMD_BINDING_DISABLE,
	LED_CMD_BINDING_FAIL,
	LED_OTA_BLOCK_TRANFER,
	LED_DIMMING_UP,
	LED_DIMMING_DOWN_LIMIT,

	LED_CONFIG_LOCK_SCHEDULE_ENABLE,
	LED_CONFIG_LOCK_SCHEDULE_FAIL,
	LED_CONFIG_LOCK_SCHEDULE_DISABLE,

	LED_CONFIG_UNLOCK,

	END_LED_EVT
}led_evt_t;

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/

void send_led_evt_to_mcu( u8 led_evt, u16 mask );

#endif /* LED_EV_H_ */
