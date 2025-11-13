/*
 * led_ev.c
 *
 *  Created on: Sep 21, 2020
 *      Author: DungTran BK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "vendor/common/app_provison.h"
#include "utilities.h"
#include "led.h"
#include "led_ev.h"

#include "debug.h"
#ifdef  LED_EV_DBG_EN
#define DBG_LED_EV_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_LED_EV_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_LED_EV_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_LED_EV_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_LED_EV_SEND_STR(x)
#define DBG_LED_EV_SEND_INT(x)
#define DBG_LED_EV_SEND_HEX(x)
#define DBG_LED_EV_SEND_BYTE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/
/**
 * @func    LED_TurnOn
 * @brief   None
 * @param
 * @retval  None
 */
void send_led_evt_to_mcu( u8 led_evt, u16 mask )
{
    switch( led_evt )
    {
		case LED_POWER_ON:{
			if(STATE_DEV_UNPROV == get_provision_state()) {
				led_blink_color(0xFFFF,
						LED_COLOR_RED,
						3,
						LAST_STATE_REFRESH_LED,
						300
					);
			}
			else {
				#if NOTIFY_COLOR_PINK_ENABLE
				led_blink_color(LED_CONFIG_MASK,
						LED_COLOR_PINK,
						3,
						LAST_STATE_REFRESH_LED,
						300
					);
				#else
				led_blink_color(0xFFFF,
						LED_COLOR_BLUE,
						3,
						LAST_STATE_REFRESH_LED,
						300
					);
				#endif
			}
			DBG_LED_EV_SEND_STR("\n LED_POWER_ON");
			break;
		}
		case LED_OTA_FAIL:{
			led_blink_color(0xFFFF,
						LED_COLOR_RED,
						6,
						LAST_STATE_REFRESH_LED,
						250
					);
			while(!(reg_uart_status1 & FLD_UART_TX_DONE));
			DBG_LED_EV_SEND_STR("\n LED_OTA_FAIL");
			break;
		}
		case LED_OTA_SUCESS:{
			led_blink_color(0xFFFF,
						LED_COLOR_BLUE,
						6,
						LAST_STATE_REFRESH_LED,
						250
					);
			DBG_LED_EV_SEND_STR("\n LED_OTA_SUCESS");
			break;
		}
		case LED_FAIL_ADD_APPKEY:
		case LED_PROVISION_FAIL:{
			led_blink_color(0xFFFF,
						LED_COLOR_RED,
						3,
						LAST_STATE_REFRESH_LED,
						300
					);
			DBG_LED_EV_SEND_STR("\n LED_PROVISION_FAIL");
			break;
		}
		case LED_SUC_ADD_APPKEY: {
			#if NOTIFY_COLOR_PINK_ENABLE
			led_blink_color(LED_CONFIG_MASK,
					LED_COLOR_PINK,
					3,
					LAST_STATE_REFRESH_LED,
					300
				);
			#else
			led_blink_color(0xFFFF,
					LED_COLOR_BLUE,
					3,
					LAST_STATE_REFRESH_LED,
					300
				);
			#endif
			DBG_LED_EV_SEND_STR("\n LED_SUC_ADD_APPKEY");
			break;
		}
		case LED_PROVISION_SUCCESS:{
			led_blink_color(0xFFFF,
					LED_COLOR_BLUE,
					3,
					LAST_STATE_REFRESH_LED,
					300
				);
			DBG_LED_EV_SEND_STR("\n LED_PROVISION_SUCCESS");
			break;
		}
		case LED_CMD_SET_SUBSCRIPTION:
			led_blink_color(mask,
					LED_COLOR_BLUE,
					2,
					LAST_STATE_REFRESH_LED,
					300
				);
			DBG_LED_EV_SEND_STR("\n LED_CMD_SET_SUBSCRIPTION");
			break;
		case LED_CMD_DEL_SUBSCRIPTION:
			led_blink_color(mask,
					LED_COLOR_RED,
					2,
					LAST_STATE_REFRESH_LED,
					300
				);
			DBG_LED_EV_SEND_STR("\n LED_CMD_DEL_SUBSCRIPTION");
			break;

		case LED_CMD_SET_SCENE:
			led_blink_color(mask,
					LED_COLOR_BLUE,
					2,
					LAST_STATE_REFRESH_LED,
					200
				);
			DBG_LED_EV_SEND_STR("\n LED_CMD_SET_SCENE");
			break;
		case LED_CMD_DEL_SCENE:
			led_blink_color(mask,
					LED_COLOR_RED,
					2,
					LAST_STATE_REFRESH_LED,
					200
				);
			DBG_LED_EV_SEND_STR("\n LED_CMD_DEL_SUBSCRIPTION");
			break;

		case LED_CMD_BINDING_ENABLE:
			led_blink_color(mask,
					LED_COLOR_BLUE,
					2,
					LAST_STATE_REFRESH_LED,
					300
				);
			DBG_LED_EV_SEND_STR("\n LED_CMD_BINDING_ENABLE");
			break;

		case LED_CMD_BINDING_DISABLE:
			led_blink_color(mask,
					LED_COLOR_RED,
					2,
					LAST_STATE_REFRESH_LED,
					300
				);
			DBG_LED_EV_SEND_STR("\n LED_CMD_BINDING_DISABLE");
            break;

		case LED_CMD_BINDING_FAIL:
			led_blink_color(mask,
					LED_COLOR_RED,
					1,
					LAST_STATE_REFRESH_LED,
					300
				);
			DBG_LED_EV_SEND_STR("\n LED_CMD_BINDING_FAIL");
			break;

		case LED_OTA_BLOCK_TRANFER:
			led_blink_color(mask,
					LED_COLOR_PINK,
					1,
					LAST_STATE_REFRESH_LED,
					500
				);
			DBG_LED_EV_SEND_STR("\n LED_OTA_BLOCK_TRANFER");
			break;

		case LED_DIMMING_UP:
			led_blink_color(mask,
					LED_COLOR_BLUE,
					2,
					LAST_STATE_REFRESH_LED,
					100
				);
			DBG_LED_EV_SEND_STR("\n LED_DIMMING_UP");
			break;

		case LED_DIMMING_DOWN_LIMIT:
			led_blink_color(mask,
					LED_COLOR_RED,
					2,
					LAST_STATE_REFRESH_LED,
					100
				);
			DBG_LED_EV_SEND_STR("\n LED_DIMMING_DOWN_LIMIT");
			break;

		default:{
			DBG_LED_EV_SEND_STR("\n UNKNOWN LED EVENT");
			break;
		}
    }
}
// End file
