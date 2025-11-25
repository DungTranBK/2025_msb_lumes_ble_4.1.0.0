/*
 * button.c
 *
 *  Created on: Sep 21, 2020
 *      Author: DungTran BK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "vendor/common/system_time.h"
#include "vendor/mesh/user/fact/fact.h"
#include "led.h"
#include "led_scene.h"
#include "utilities.h"
#include "factory_reset.h"
#include "execution_scene.h"
#include "key_report.h"
#include "sw_config.h"
#include "dimming.h"
#include "relay.h"
#include "lock_schedule.h"
#include "button.h"

#include "debug.h"
#ifdef  BUTTON_DBG_EN
#define DBG_BUTTON_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_BUTTON_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_BUTTON_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_BUTTON_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_BUTTON_SEND_STR(x)
#define DBG_BUTTON_SEND_INT(x)
#define DBG_BUTTON_SEND_HEX(x)
#define DBG_BUTTON_SEND_BYTE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static btn_evt_t opt_btn_before_st[NUMBER_BUTTON] = {
		{ NO_PRESS, 0, false },
	#if NUMBER_BUTTON > 1
		{ NO_PRESS, 0, false },
	#endif
	#if NUMBER_BUTTON > 2
		{ NO_PRESS, 0, false },
	#endif
	#if NUMBER_BUTTON > 3
		{ NO_PRESS, 0, false },
	#endif
	#if NUMBER_BUTTON > 4
		{ NO_PRESS, 0, false },
	#endif
	#if NUMBER_BUTTON > 5
		{ NO_PRESS, 0, false },
	#endif
};

typedef struct {
	u32  disable_st_time_ms;
	bool is_enable;
}btn_many_par_t;

static btn_many_par_t btn_many_par[NUMBER_BUTTON] =  {
		{ 0, true },
    #if NUMBER_BUTTON > 1
		{ 0, true },
	#endif
	#if NUMBER_BUTTON > 2
		{ 0, true },
	#endif
	#if NUMBER_BUTTON > 3
		{ 0, true },
	#endif
	#if NUMBER_BUTTON > 4
		{ 0, true },
	#endif
	#if NUMBER_BUTTON > 5
		{ 0, true },
	#endif
};

static para_btn_reset_t   para_btn_reset = {
				.is_active = false,
				.step = STEP_IDLE,
				.change_step_last_t = MAX_U32,
				.is_active_st_time = 0,
			};


static para_btn_reset_t   para_btn_default = {
				.is_active = false,
				.step = STEP_IDLE,
				.change_step_last_t = MAX_U32,
				.is_active_st_time = 0,
			};

#define MASTER_BTN_INDEX        BUTTON_SWITCH_0_IDX


const u8 const_button_config_idx_arr[2]  = { BUTTON_SCENE_0_IDX, BUTTON_SWITCH_0_IDX };

const u8 const_button_default_idx_arr[2] = { BUTTON_SCENE_1_IDX, BUTTON_SWITCH_0_IDX };

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/

/**
 * @func   ev_handle_default_timeout_handle
 * @brief
 * @param
 * @retval None
 */
static void ev_handle_default_timeout_handle(void)
{
	para_btn_default.step = STEP_IDLE;
	para_btn_default.is_active = false;
	led_refresh(BACKUP_MASK_RL);
}

/**
 * @func   button_check_clean_default_param
 * @brief
 * @param
 * @retval None
 */
void button_check_clean_default_param(void)
{
	if(para_btn_default.is_active == true) {
		if((clock_time_get_elapsed_time(para_btn_default.is_active_st_time) > CONFIRM_RESET_TIMEOUT_MS)) {
			ev_handle_default_timeout_handle();
			DBG_BUTTON_SEND_STR("\nev_handle_check_clean_default_param: 0");
		}
	}
	if(para_btn_default.step == STEP_HOLD_5S) {
		if(clock_time_exceed_ms(para_btn_default.change_step_last_t, TIMER_2S)) {
			ev_handle_default_timeout_handle();
			DBG_BUTTON_SEND_STR("\nev_handle_check_clean_reset_param: 1");
		}
	}
}

/**
 * @func   ev_handle_reset_timeout_handle
 * @brief
 * @param
 * @retval None
 */
static void ev_handle_reset_timeout_handle(void)
{
	para_btn_reset.step = STEP_IDLE;
	para_btn_reset.is_active = false;
	led_refresh(BACKUP_MASK_RL);
}

/**
 * @func   button_check_clean_reset_param
 * @brief
 * @param
 * @retval None
 */
void button_check_clean_reset_param(void)
{
	if(para_btn_reset.is_active == true) {
		if((clock_time_get_elapsed_time(para_btn_reset.is_active_st_time) > CONFIRM_RESET_TIMEOUT_MS)) {
			ev_handle_reset_timeout_handle();
			DBG_BUTTON_SEND_STR("\nev_handle_check_clean_reset_param: 0");
		}
	}
	if(para_btn_reset.step == STEP_HOLD_5S) {
		if(clock_time_exceed_ms(para_btn_reset.change_step_last_t, TIMER_2S)) {
			ev_handle_reset_timeout_handle();
			DBG_BUTTON_SEND_STR("\nev_handle_check_clean_reset_param: 1");
		}
	}
}

/**
 * @func    button_handle_scene_btn_state
 * @brief   None
 * @param
 * @retval  None
 */
void button_handle_scene_btn_state(u8 idx, u8 evt)
{
	if(evt == NO_PRESS) {
		if(btn_many_par[idx].is_enable == false) {
			if(clock_time_exceed_ms(  \
					btn_many_par[idx].disable_st_time_ms, TIMER_1S)) {
				btn_many_par[idx].is_enable = true;
			}
		}
		return;
	}
	u8 new_evt;
	bool dimming_flag =  execution_dimming_is_exist(idx, &new_evt);
	bool is_lock = button_is_lock(idx);

	switch(evt)
	{
		case START_PRESS:
		{
			if(is_lock == false)
			{
				if(dimming_flag == FALSE)
				{
					led_scene_push_blink_led_cmd_with_interval_to_fifo(1 << idx, 1, TIMER_150MS);
				}
			}
			break;
		}
		case HOLD_50MS:
		{
			/*
			if(is_lock == false)
			{
				if(dimming_flag == TRUE) {
					opt_btn_before_st[idx].hold_500ms_flag = TRUE;
					execution_dimming_control_by_key_number(idx, new_evt, B_HOLD_500MS);
					key_report_send(idx, new_evt, false);
				}
			}
			*/
			break;
		}
		case HOLD_500MS:
		{
			if(dimming_flag == TRUE) {
				opt_btn_before_st[idx].hold_500ms_flag = TRUE;
				execution_dimming_control_by_key_number(idx, new_evt, B_HOLD_500MS);
				key_report_send(idx, new_evt, false);
				btn_many_par[idx].is_enable = false;
				btn_many_par[idx].disable_st_time_ms = clock_time_ms();
				DBG_BUTTON_SEND_STR("\n HOLD_500MS: ");
				DBG_BUTTON_SEND_INT(idx);
			}
			break;
		}
		case PRESS_ONE_TIME:
		{
			if(is_lock == false) {
				if(dimming_flag == false) {
					key_report_send(idx, BTN_KEY_PRESS_1_TIME, true);
				}
				else {
					if(btn_many_par[idx].is_enable == false) {
						btn_many_par[idx].is_enable = true;
						DBG_BUTTON_SEND_STR("\n DISABLE MANY < ");
						DBG_BUTTON_SEND_INT(idx);
						break;
					}
					key_report_send(idx, BTN_KEY_PRESS_1_TIME, false);
				}
			}
			break;
		}
		case PRESS_TWO_TIME:
		{
			if(is_lock == false)
			{
				if(dimming_flag == FALSE)
				{
					key_report_send(idx, BTN_KEY_PRESS_2_TIMES, true);
				}
				else
				{
					if(btn_many_par[idx].is_enable == false) {
						btn_many_par[idx].is_enable = true;
						break;
					}
					key_report_send(idx, BTN_KEY_PRESS_2_TIMES, false);
				}
			}
			break;
		}

		case HOLD_2S:
		{
			if(is_lock == false)
			{
				if(dimming_flag == FALSE)
				{
					DBG_BUTTON_SEND_STR("\n SCENE HOLD_2S: ");
					DBG_BUTTON_SEND_INT(idx);
					led_scene_push_blink_led_cmd_with_interval_to_fifo(1 << idx, 2, BLINK_HOLD_2S_INTERVAL);
				}
			}
			break;
		}

		case HOLD_10S:
		{

			break;
		}
		case RELEASE:
		{
			if(dimming_flag == TRUE) {
				if(opt_btn_before_st[idx].evt == START_PRESS
						|| opt_btn_before_st[idx].evt == HOLD_50MS) {
					led_scene_push_blink_led_cmd_with_interval_to_fifo(1 << idx, 1, TIMER_70MS);
				}
			}
			DBG_BUTTON_SEND_STR("\n SCENE RELEASE: ");
			if(is_lock == false)
			{
				if(dimming_flag == true) {
					if(opt_btn_before_st[idx].hold_500ms_flag == true) {
						execution_dimming_control_by_key_number(idx, new_evt, B_RELEASE);
						opt_btn_before_st[idx].hold_500ms_flag = false;
					}
					else
					{
						if(opt_btn_before_st[idx].evt == START_PRESS) {
							execution_dimming_control_by_key_number(idx, new_evt, B_START_PRESS);
						}
					}
				}
				else
				{
					if(opt_btn_before_st[idx].evt == HOLD_2S)
					{
						if(!clock_time_exceed_ms(opt_btn_before_st[idx].update_st_t, TIMER_2S))
						{
							key_report_send(idx, BTN_KEY_HOLD_2_SECONDS, true);
						}
					}
				}
			}
			break;
		}
	}
}

/**
 * @func    button_handle_relay_btn_state
 * @brief   None
 * @param
 * @retval  None
 */
void button_handle_relay_btn_state(u8 idx, u8 evt)
{
	if(evt == NO_PRESS) {
		return;
	}
	u8 inter_idx;
	if(get_relay_idx_follow_model_index(idx, &inter_idx) == false)
	{
		return;
	}
	if(button_is_lock(idx) == true)
	{
		return;
	}
	if(sw_config_st.switch_mode[inter_idx] == LIGHTING_SWITCH_TYPE) {
		if(button_is_lock(idx) == false) {
			dimming_handle_btn_st(idx, evt);
		}
		DBG_BUTTON_SEND_STR("\n DIMMING Handle");
		return;
	}
	switch(evt)
	{
		case START_PRESS:
			DBG_BUTTON_SEND_STR("\n RL START_PRESS: ");
			if(button_is_lock(idx) == false)
			{
				if(sw_config_st.switch_mode[inter_idx] == TOGGLE_SWITCH_TYPE)
				{
					relay_toggle_state(inter_idx, SRC_DEVICE);
				}
				else if(sw_config_st.switch_mode[inter_idx] == MOMENTORY_SWITCH_TYPE)
				{
					relay_set_target_state(inter_idx, G_ON, SRC_DEVICE, true);
				}
			}
#ifdef  BUTTON_DBG_EN
			else
			{
				DBG_BUTTON_SEND_STR("\n Button is lock, please unlock first");
			}
#endif
			break;

		case PRESS_TEN_TIME:
		{
			if(inter_idx == (BUTTON_SWITCH_0_IDX - ELE_RELAY_OFFSET))
			{
				sw_config_enable_auto_send();
				led_blink_color(1 << inter_idx,
							LED_COLOR_BLUE,
							2,
							LAST_STATE_REFRESH_LED,
							TIMER_200MS
						);
			}
			else if(inter_idx == (BUTTON_SWITCH_1_IDX - ELE_RELAY_OFFSET))
			{
				execution_set_up_auto_send();
				led_blink_color(1 << inter_idx,
							LED_COLOR_BLUE,
							2,
							LAST_STATE_REFRESH_LED,
							TIMER_200MS
						);
			}
			else
			{
				DBG_BUTTON_SEND_STR("\n UNUSED Event");
			}
			DBG_BUTTON_SEND_STR("\n PRESS_TEN_TIME: ");
			DBG_BUTTON_SEND_INT(inter_idx);
			break;
		}
		case RELEASE:
			DBG_BUTTON_SEND_STR("\n RL RELEASE: ");
			if(sw_config_st.switch_mode[inter_idx] == MOMENTORY_SWITCH_TYPE)
			{
				relay_set_target_state(inter_idx, G_OFF, SRC_DEVICE, true);
			}
			break;
	}
}

/**
 * @func    is_default_button
 * @brief   None
 * @param
 * @retval  None
 */
static bool is_default_button(u8 idx)
{
	foreach(i, 2) {
		if(idx == const_button_default_idx_arr[i]) {
			return true;
		}
	}
	return false;
}

/**
 * @func    update_default_by_btn_step
 * @brief   None
 * @param
 * @retval  None
 */
static void update_default_by_btn_step(u8 step)
{
	if(step < STEP_INVALID) {
		para_btn_default.step = step;
		para_btn_default.change_step_last_t = clock_time_ms();
	}
}

/**
 * @func    is_config_button
 * @brief   None
 * @param
 * @retval  None
 */
static bool is_config_button(u8 idx)
{
	foreach(i, 2) {
		if(idx == const_button_config_idx_arr[i]) {
			return true;
		}
	}
	return false;
}


/**
 * @func    update_reset_by_btn_step
 * @brief   None
 * @param
 * @retval  None
 */
static void update_reset_by_btn_step(u8 step)
{
	if(step < STEP_INVALID) {
		para_btn_reset.step = step;
		para_btn_reset.change_step_last_t = clock_time_ms();
	}
}

/**
 * @func    button_handle_special_event
 * @brief   None
 * @param
 * @retval  None
 */
void button_handle_special_event(u8 idx, u8 evt)
{
	if(is_config_button(idx) == true)
	{
		u8 idx_cmp = (const_button_config_idx_arr[0] == idx)?  \
				const_button_config_idx_arr[1]:const_button_config_idx_arr[0];
		switch(evt)
		{
			case HOLD_5S:
			{
				if(opt_btn_before_st[idx_cmp].evt == HOLD_5S) {
					led_off_all();
					led_scene_off_all();
					led_set_color(BUTTON_SWITCH_0_IDX - ELE_RELAY_OFFSET, LED_COLOR_PINK);
					update_reset_by_btn_step(STEP_HOLD_5S);
				}
				break;
			}

			case PRESS_ONE_TIME:
			{
				if(idx == MASTER_BTN_INDEX) {
					if(para_btn_reset.is_active == true  \
							&& para_btn_reset.step == STEP_HOLD_5S_RELEASE) {
						if(!clock_time_exceed_ms( \
								para_btn_reset.is_active_st_time, CONFIRM_RESET_TIMEOUT_MS)) {
							setup_factory_reset_with_delay(true, true);
							DBG_BUTTON_SEND_STR("\n OUT_NETWORK");
						}
						else {
							button_check_clean_reset_param();
							DBG_BUTTON_SEND_STR("\n ev_handle_reset_by_touch_timeout");
						}
					}
				}
				break;
			}
			case RELEASE:
			{
				if(para_btn_reset.step == STEP_HOLD_5S) {
					if(!clock_time_exceed_ms(para_btn_reset.change_step_last_t, TIMER_2S)) {
						if(opt_btn_before_st[idx_cmp].evt == RELEASE) {
							update_reset_by_btn_step(STEP_HOLD_5S_RELEASE);
							para_btn_reset.is_active = true;
							para_btn_reset.is_active_st_time = clock_time_ms();
							led_off_all();
							DBG_BUTTON_SEND_STR("\n --- 1");
						}
					}
					else {
						ev_handle_reset_timeout_handle();
						DBG_BUTTON_SEND_STR("\n --- 2");
					}
				}
				else {
					DBG_BUTTON_SEND_STR("\n --- 3");
				}
				break;
			}
		}
	}
	// Default
	if(is_default_button(idx) == true) {
		u8 idx_cmp = (const_button_default_idx_arr[0] == idx)?  \
				const_button_default_idx_arr[1]:const_button_default_idx_arr[0];
		switch(evt)
		{
			case HOLD_5S:
			{
				if(opt_btn_before_st[idx_cmp].evt == HOLD_5S) {
					led_off_all();
					led_scene_on(BUTTON_SCENE_1_IDX);
					update_default_by_btn_step(STEP_HOLD_5S);
				}
				break;
			}

			case PRESS_FIVE_TIME:
			{
				if(idx == BUTTON_SCENE_1_IDX) {
					if(para_btn_default.is_active == true  \
							&& para_btn_default.step == STEP_HOLD_5S_RELEASE) {
						if(!clock_time_exceed_ms( \
								para_btn_default.is_active_st_time, CONFIRM_RESET_TIMEOUT_MS)) {
							DBG_BUTTON_SEND_STR("\n Reset to default: ");
							// TODO
							setup_factory_reset_with_delay(true, false);
							factory_force_reset_enery_and_relay_on_time();
							led_scene_push_blink_led_cmd_with_interval_to_fifo(1 << BUTTON_SCENE_1_IDX, 5, TIMER_150MS);
						}
						else {
							button_check_clean_default_param();
							DBG_BUTTON_SEND_STR("\n Press one time is timeout");
						}
					}
				}
				break;
			}
			case RELEASE:
			{
				if(para_btn_default.step == STEP_HOLD_5S) {
					if(!clock_time_exceed_ms(para_btn_default.change_step_last_t, TIMER_2S)) {
						if(opt_btn_before_st[idx_cmp].evt == RELEASE) {
							update_default_by_btn_step(STEP_HOLD_5S_RELEASE);
							para_btn_default.is_active = true;
							para_btn_default.is_active_st_time = clock_time_ms();
							led_scene_off_all();
							DBG_BUTTON_SEND_STR("\n --- 1");
						}
					}
					else {
						ev_handle_default_timeout_handle();
						DBG_BUTTON_SEND_STR("\n --- 2");
					}
				}
				break;
			}
		}
	}
}

/**
 * @func    button_handle_option_btn_state
 * @brief   None
 * @param
 * @retval  None
 */
void button_handle_option_btn_state(u8 idx, u8 evt)
{
	ElementSupportType_Enum type = get_ele_support_type(idx);

	if(fact_is_active() == 0)
	{
		if(type == ELE_SUPPORT_SCENE) {
			button_handle_scene_btn_state(idx, evt);
		}
		else if(type == ELE_SUPPORT_RELAY) {
			if(para_btn_reset.is_active != true) {
				button_handle_relay_btn_state(idx, evt);
			}
		}
		else {
			DBG_BUTTON_SEND_STR("\n BUTTON index invalid");
			return;
		}
		button_handle_special_event(idx, evt);
	}
	// Update Parameter
	if(idx < NUMBER_BUTTON) {
		opt_btn_before_st[idx].evt = evt;
		opt_btn_before_st[idx].update_st_t = clock_time_ms();
	}

}
// End file
