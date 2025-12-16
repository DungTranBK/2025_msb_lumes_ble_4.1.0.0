/*
 * switch.c
 *
 *  Created on: Mar 3, 2021
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../../proj_lib/sig_mesh/app_mesh.h"
#include "../../common/system_time.h"
#include "vendor/mesh/user/fact/fact.h"
#include "led.h"
#include "utilities.h"
#include "sw_config.h"
#include "sw_auto.h"
#include "execution_scene.h"
#include "relay.h"
#include "energy.h"
#include "switch.h"

#include "debug.h"
#ifdef SW_DBG_EN
#define DBG_SW_SEND_STR(x)     Dbg_sendString((s8*)x)
#define DBG_SW_SEND_INT(x)     Dbg_sendInt(x)
#define DBG_SW_SEND_HEX(x)     Dbg_sendHex(x)
#define DBG_SW_SEND_HEX_ONE(x) Dbg_sendHexOneByte(x);
#else
#define DBG_SW_SEND_STR(x)
#define DBG_SW_SEND_INT(x)
#define DBG_SW_SEND_HEX(x)
#define DBG_SW_SEND_HEX_ONE(x)
#endif

#if DEV_TYPE_SEL == G_TYPE_SWITCH_BUTTON
/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/

static bool display_in_fact_mode_flag = false;
static u32 dp_start_time_ms = 0;


/**
 * @func   Switch_displayLedInFactMode
 * @brief
 * @param  None
 * @retval None
 */
static void Switch_displayLedInFactMode(void)
{
	if(display_in_fact_mode_flag == true)
	{
		if(clock_time_exceed_ms(dp_start_time_ms, TIMER_2S))
		{
			LedCommand_str led_cmd = COMMAND_LED_DEFAULT;
			led_cmd.ledMask = BACKUP_MASK_RL;
			led_cmd.ledMode  = LED_MODE_ON;
			led_cmd.ledColor = LED_COLOR_PINK;
			(void)led_push_led_command_to_fifo(&led_cmd);
			display_in_fact_mode_flag = false;
			DBG_SW_SEND_STR("\n Switch_displayLedInFactMode");
		}
	}
}

/**
 * @func
 * @brief
 * @param  None
 * @retval None
 */
void Switch_handleExitFactMode(void)
{
	relay_deinit_after_fact();
	start_reboot();
}

/**
 * @func   Switch_handleChangeToFactMode
 * @brief
 * @param  None
 * @retval None
 */
void Switch_handleFactConfirmOrActivate(u8 state)
{
    if(state == FACT_CONFIRM) {
        led_push_normal_blink_led_cmd_to_fifo(2, LED_COLOR_BLUE);
    }
    if(state == FACT_ACTIVATE) {
    	foreach(i, NUMBER_RL) {
    		relay_set_target_state(i, G_OFF, SRC_DEVICE, false);
    	}
    	display_in_fact_mode_flag = true;
    	dp_start_time_ms = clock_time_ms();
    }
}

/**
 * @func   switch_loop_100ms
 * @brief
 * @param  None
 * @retval None
 */
void sw_loop_100ms_interval(void)
{
	auto_loop_task();
	sw_config_auto_send_task();
	execution_loop_task();
	Switch_displayLedInFactMode();
	energy_proc();
}
#endif
