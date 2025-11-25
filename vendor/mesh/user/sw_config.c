/*
 * sw_config.c
 *
 *  Created on: Jan 25, 2021
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/system_time.h"
#include "utilities.h"
#include "lock_schedule.h"
#include "led.h"
#include "flash_user.h"
#include "com.h"
#include "energy.h"
#include "sw_config.h"

#if DEV_TYPE_SEL == G_TYPE_SWITCH_BUTTON

#include "debug.h"
#ifdef  SW_CONFIG_DBG_EN
#define DBG_SW_CONFIG_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_SW_CONFIG_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_SW_CONFIG_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_SW_CONFIG_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
#else
#define DBG_SW_CONFIG_SEND_STR(x)
#define DBG_SW_CONFIG_SEND_INT(x)
#define DBG_SW_CONFIG_SEND_HEX(x)
#define DBG_SW_CONFIG_SEND_BYTE(x)
#endif

typeConfig_handle_btn_change_mode pvConfig_handle_btn_change_mode = NULL;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const u8 msg_set_cmd_arr[]  =
		{
			VD_CONFIG_SW_SET_MAP_UNMAP_INPUT,
			VD_CONFIG_SWITCH_MODE_OPT,
			VD_CONFIG_ON_POWER_UP_STATE,
			#if MAP_INPUT_OUTPUT_EN
			VD_MAP_INPUT_OUTPUT_OPT,
			#endif
			VD_LINK_UNLINK_V2_OPT,
			VD_EN_ON_OFF_GROUP_DEFAULT,
		};

static const u8 msg_get_cmd_arr[]  =
		{
			VD_CONFIG_SW_SET_MAP_UNMAP_INPUT,
			VD_CONFIG_SWITCH_MODE_OPT,
			VD_CONFIG_ON_POWER_UP_STATE,
            #if MAP_INPUT_OUTPUT_EN
			VD_MAP_INPUT_OUTPUT_OPT,
            #endif
			VD_LINK_UNLINK_V2_OPT,
			VD_EN_ON_OFF_GROUP_DEFAULT,
		};

const u8 opcode_msg_auto_send[] =
		{
			VD_CONFIG_SWITCH_MODE_OPT,
			VD_CONFIG_ON_POWER_UP_STATE,
            #if MAP_INPUT_OUTPUT_EN
			VD_MAP_INPUT_OUTPUT_OPT,
            #endif
			VD_LINK_UNLINK_V2_OPT,
			VD_EN_ON_OFF_GROUP_DEFAULT,
			VD_TS_LOCK_SCHEDULE,    // External file
			VD_ACTIVE_ENERGY_INFORMATION,
			VD_TOTAL_RELAY_ON_TIME_INFORMATION,
		};

static  sw_cfg_auto_t sw_cfg_auto_st =
		{
			.en = false,
			.send_last_t = 0,
			.op_index = 0,
			.btn_idx = 0,
			.delay_st = 0,
			.en_delay = false,
			.send_join = false
		};

u8 tmp_response[SWITCH_CONFIG_RESPONSE_MAX_LEN];

sw_config_t sw_config_st;

static int flash_ble_sw_config_idx = FLASH_INDEX_DEFAULT;

#define BLOCK_SIZE_SW_CONFIG     sizeof(sw_config_t)
#define FLASH_SIZE_SW_CONFIG     (FLASH_SECTOR_SIZE - BLOCK_SIZE_SW_CONFIG)

/******************************************************************************/
/*                       PRIVATE FUNCTION DECLERATION                         */
/******************************************************************************/
static void sw_response_enable_disable_group_default(int idx);

/******************************************************************************/
/*                            EXPORT FUNCTION                                 */
/******************************************************************************/

/**
 * @func    sw_config_store
 * @brief
 * @param   None
 * @retval  None
 */
static void sw_config_store(void)
{
	flash_user_store(  \
			(int*)&flash_ble_sw_config_idx,
			FLASH_ADR_BLE_SWITCH_CONFIG, FLASH_SIZE_SW_CONFIG,
			BLOCK_SIZE_SW_CONFIG,(u8*)&sw_config_st  \
		);
}

/**
 * @func    sw_check_valid_params
 * @brief
 * @param   None
 * @retval  None
 */
void sw_check_valid_params(void)
{
	bool save_flag = false;
	foreach(i, NUMBER_RL) {
		if(sw_config_st.switch_mode[i] >= SWITCH_TYPE_UNKNOWN) {
			sw_config_st.switch_mode[i] = TOGGLE_SWITCH_TYPE;
			save_flag = true;
		}
		if(sw_config_st.extended_param_st[i].toggle_type == MAX_U8)	{
			sw_config_st.extended_param_st[i].toggle_type = TOGGLE_UNKNOWN;
			sw_config_st.extended_param_st[i].delay_time_s = AUTO_ON_OFF_MIN_TIME_S;
			save_flag = true;
		}
		if(sw_config_st.link_unlink[i] >= LINK_UNKNOWN) {
			sw_config_st.link_unlink[i] = LINK_ENABLE;
			save_flag = true;
		}
		if(sw_config_st.map_input[i] > NUMBER_RL) {
			sw_config_st.map_input[i] = i + 1;
			save_flag = true;
		}
		if(sw_config_st.en_group_default[i] == MAX_U8) {
			sw_config_st.en_group_default[i] = true;
			save_flag = true;
		}
		if(sw_config_st.power_on_state[i] >= POWER_UP_UNKNOWN) {
			sw_config_st.power_on_state[i] = POWER_UP_RESTORE;
			save_flag = true;
		}
	}
	if(save_flag == true) {
		flash_user_store(  \
				(int*)&flash_ble_sw_config_idx, FLASH_ADR_BLE_SWITCH_CONFIG,  \
				FLASH_SIZE_SW_CONFIG, BLOCK_SIZE_SW_CONFIG,(u8*)&sw_config_st  \
			);
	}
}

/**
 * @func    sw_config_init
 * @brief
 * @param   None
 * @retval  None
 */
void sw_config_init(void)
{
	memset(&sw_config_st, MAX_U8, sizeof(sw_config_t));
	// restore
	flash_user_get_flash_index(  \
			(int*)&flash_ble_sw_config_idx, FLASH_ADR_BLE_SWITCH_CONFIG, \
			FLASH_SIZE_SW_CONFIG, BLOCK_SIZE_SW_CONFIG, (u8*)&sw_config_st \
		);
	flash_user_restore(
			flash_ble_sw_config_idx, FLASH_ADR_BLE_SWITCH_CONFIG, BLOCK_SIZE_SW_CONFIG,(u8*)&sw_config_st
		);
    // Check Valid
	sw_check_valid_params();

	DBG_SW_CONFIG_SEND_STR("\n sw_config_init");
}

/**
 * @func    sw_config_next_op_index
 * @brief
 * @param   None
 * @retval  None
 */
static void sw_config_next_op_index(void)
{
	sw_cfg_auto_st.op_index++;
	sw_cfg_auto_st.btn_idx = 0;
}

/**
 * @func    sw_config_auto_send
 * @brief
 * @param   None
 * @retval  None
 */
void sw_config_auto_send_task(void)
{
    if(sw_cfg_auto_st.en == true) {
        if(clock_time_exceed_ms    \
        		(sw_cfg_auto_st.send_last_t, TIMER_1S + rand()%TIMER_1S)) {
        	if(sw_cfg_auto_st.op_index >= sizeof(opcode_msg_auto_send)) {
        		sw_cfg_auto_st.en = false;
        		return;
        	}
        	u8 op = opcode_msg_auto_send[sw_cfg_auto_st.op_index];
        	switch(op)
        	{
        	    case VD_CONFIG_SWITCH_MODE_OPT:
        	    {

					DBG_SW_CONFIG_SEND_STR("\n VD_CONFIG_SWITCH_MODE_OPT: ");
					DBG_SW_CONFIG_SEND_INT(sw_cfg_auto_st.btn_idx);

        	    	u8 model_idx = sw_cfg_auto_st.btn_idx + ELE_RELAY_OFFSET;
        			sw_cfg_auto_st.btn_idx++;
					sw_cfg_handle_get_message(
								0,
								&model_idx,
								1,
								VD_CONFIG_SWITCH_MODE_OPT
							);
					if(sw_cfg_auto_st.btn_idx >= NUMBER_RL) {
						sw_config_next_op_index();
					}
        	    	break;
				}
        	    case VD_CONFIG_ON_POWER_UP_STATE:
#if MAP_INPUT_OUTPUT_EN
        	    case VD_MAP_INPUT_OUTPUT_OPT:
#endif
        	    case VD_LINK_UNLINK_V2_OPT:
        	    {
        	    	sw_cfg_handle_get_message(
									0,
									0,
									0,
									op
								);
					sw_config_next_op_index();
					DBG_SW_CONFIG_SEND_STR("\n COMMON: ");
        	    	break;
				}
        	    case VD_EN_ON_OFF_GROUP_DEFAULT:
        	    {
					DBG_SW_CONFIG_SEND_STR("\n VD_EN_ON_OFF_GROUP_DEFAULT: ");
					DBG_SW_CONFIG_SEND_INT(sw_cfg_auto_st.btn_idx);
        	    	u8 model_idx = sw_cfg_auto_st.btn_idx + ELE_RELAY_OFFSET;
        	    	sw_cfg_auto_st.btn_idx++;
					sw_cfg_handle_get_message(
								model_idx,
								&model_idx,
								1,
								VD_CONFIG_SWITCH_MODE_OPT
							);
					sw_cfg_auto_st.btn_idx++;
					if(sw_cfg_auto_st.btn_idx >= NUMBER_RL) {
						sw_config_next_op_index();
					}
        	    	break;
				}
        	    case VD_TS_LOCK_SCHEDULE:
				{
					DBG_SW_CONFIG_SEND_STR("\n VD_TS_LOCK_SCHEDULE: ");
					#ifdef  MD_RADAR_ENABLE
					vd_lock_schedule_response(
								sw_cfg_auto_st.btn_idx+1,
								ele_adr_primary + sw_cfg_auto_st.btn_idx+1,
								GATEWAY_UNICAST_ADDR
							);
					#else
					vd_lock_schedule_response(
								sw_cfg_auto_st.btn_idx,
								ele_adr_primary + sw_cfg_auto_st.btn_idx,
								GATEWAY_UNICAST_ADDR
							);
                    #endif
					sw_cfg_auto_st.btn_idx++;
					if(sw_cfg_auto_st.btn_idx >= NUMBER_BUTTON) {
						sw_config_next_op_index();
					}
					break;
				}
        	    case VD_ACTIVE_ENERGY_INFORMATION:
        	    {
        	    	u8 idx = sw_cfg_auto_st.btn_idx;
        	    	energy_setup_publish_energy_delay(idx, 0);
        			sw_cfg_auto_st.btn_idx++;
					if(sw_cfg_auto_st.btn_idx >= NUMBER_RL) {
						sw_config_next_op_index();
					}
        	    	break;
        	    }
        	    case VD_TOTAL_RELAY_ON_TIME_INFORMATION:
        	    {
        	    	u8 idx = sw_cfg_auto_st.btn_idx;
        			relay_response_total_relay_on_time(idx);
        			sw_cfg_auto_st.btn_idx++;
					if(sw_cfg_auto_st.btn_idx >= NUMBER_RL) {
						sw_config_next_op_index();
					}
        	    	break;
        	    }
        	    default:
        	    {
        	    	DBG_SW_CONFIG_SEND_STR("\n ### DEFAULT");
        	    	sw_cfg_auto_st.en = false;
        	        break;
        	    }
        	}
        	sw_cfg_auto_st.send_last_t = clock_time_ms();
        }
    }
    if(sw_cfg_auto_st.en_delay == true) {
    	if(clock_time_exceed_ms(sw_cfg_auto_st.delay_st, TIMER_30S)) {
    		sw_cfg_auto_st.en_delay = false;
    		sw_config_enable_auto_send();
    	}
    }
}

/**
 * @func    sw_config_reset_auto_send
 * @brief
 * @param   None
 * @retval  None
 */
int sw_config_enable_auto_send(void)
{
	DBG_SW_CONFIG_SEND_STR("\n sw_config_enable_auto_send");
	if(sw_cfg_auto_st.en == false)
	{
		sw_cfg_auto_st.en = true;
		sw_cfg_auto_st.send_last_t = clock_time_ms();
		if(sw_cfg_auto_st.send_last_t >= TIMER_1S) {
			sw_cfg_auto_st.send_last_t -= TIMER_1S;
		}
		sw_cfg_auto_st.op_index = 0;
		sw_cfg_auto_st.btn_idx = 0;
		return 0;
	}
	return -1;
}

/**
 * @func    sw_setup_enable_send_config_delay
 * @brief
 * @param
 * @retval  None
 */
void sw_setup_enable_send_config_delay(bool en, bool join_flag)
{
	DBG_SW_CONFIG_SEND_STR("\n sw_setup_enable_send_config_delay");
	sw_cfg_auto_st.send_join = join_flag&0x01;
	if(en == true) {
		sw_cfg_auto_st.en_delay = true;
		sw_cfg_auto_st.delay_st = clock_time_ms();
	}
	else {
		sw_cfg_auto_st.en_delay = false;
	}
}

/**
 * @func    sw_config_get_sw_mode
 * @brief
 * @param   Button Index
 * @retval  Switch Mode Match Button Index
 */
uint8_t sw_config_get_sw_mode(int idx)
{
	if(idx < NUMBER_RL) {
		return sw_config_st.switch_mode[idx];
	}
	return SWITCH_TYPE_UNKNOWN;
}

/**
 * @func    sw_cfg_response_link_unlink
 * @brief
 * @param
 * @retval  None
 */
static void sw_cfg_response_link_unlink(u8 idx)
{
	tmp_response[0] = VD_CONFIG_SW_SET_MAP_UNMAP_INPUT;
	tmp_response[1] = sw_config_st.link_unlink[idx];
	u16 model_idx = get_ele_address_base_type_and_idx(ELE_SUPPORT_RELAY, idx);
	mesh_tx_cmd_rsp(
				 VD_CONFIG_NODE_STATUS,
				 tmp_response,
				 2,
				 ele_adr_primary + model_idx ,
				 GATEWAY_UNICAST_ADDR,
				 0,
				 0
			 );
}

/**
 * @func    sw_config_response_all_switch_mode
 * @brief
 * @param
 * @retval  None
 */
static void sw_config_response_switch_mode_by_bit_mask(u16 mask)
{
	update_sw_mode_t update_sw_mode_st;
	u8 len = 0;
	update_sw_mode_st.op = VD_CONFIG_SWITCH_MODE_OPT;
	foreach(i, NUMBER_RL) {
		if(((mask >> i) & 0x01) == 1) {
			update_sw_mode_st.btn_id = i+1+ELE_RELAY_OFFSET;
			update_sw_mode_st.mode = sw_config_st.switch_mode[i];
			if(update_sw_mode_st.mode == TOGGLE_SWITCH_TYPE) {
				len = sizeof(update_sw_mode_t);
				update_sw_mode_st.toggle_type = sw_config_st.extended_param_st[i].toggle_type;
				update_sw_mode_st.delay_t = sw_config_st.extended_param_st[i].delay_time_s;
			} else {
				len = sizeof(update_sw_mode_t) - 3;
			}
			u16 model_idx = get_ele_address_base_type_and_idx(ELE_SUPPORT_RELAY, i);
			mesh_tx_cmd_rsp(
					 VD_CONFIG_NODE_STATUS,
					 (u8 *)&update_sw_mode_st,
					 len,
					 ele_adr_primary + model_idx,
					 GATEWAY_UNICAST_ADDR,
					 0,
					 0
				 );
		}
	}
}

/**
 * @func    sw_cfg_response_switch_mode
 * @brief
 * @param
 * @retval  None
 */
static void sw_cfg_response_switch_mode(u8 idx)
{
	sw_config_response_switch_mode_by_bit_mask(1 << idx);
}

/**
 * @func    sw_cfg_response_on_power_up_st_config
 * @brief
 * @param
 * @retval  None
 */
static void sw_cfg_response_on_power_up_st_config(u8 idx)
{
	if(idx < NUMBER_RL)
	{
		update_on_power_up_st_t update;
		update.op = VD_CONFIG_ON_POWER_UP_STATE;
		update.on_power_up_st.btn_id = idx+1+ELE_RELAY_OFFSET;
		update.on_power_up_st.mode = sw_config_st.power_on_state[idx];
		u16 model_idx = get_ele_address_base_type_and_idx(ELE_SUPPORT_RELAY, idx);
		mesh_tx_cmd_rsp(
				 VD_CONFIG_NODE_STATUS,
				 (u8 *)&update,
				 sizeof(update_on_power_up_st_t),
				 ele_adr_primary + model_idx,
				 GATEWAY_UNICAST_ADDR,
				 0,
				 0
		 	 );
		DBG_SW_CONFIG_SEND_STR("\n sw_cfg_response_on_power_up_st_config: ");
		DBG_SW_CONFIG_SEND_INT(idx);
	}
}

/**
 * @func    sw_cfg_response_on_power_up_st_config
 * @brief
 * @param
 * @retval  None
 */
static void sw_cfg_response_all_on_power_up_st_config(void)
{
	update_all_on_power_up_st_t update;
	update.op = VD_CONFIG_ON_POWER_UP_STATE;
	foreach(i, NUMBER_RL)
	{
		update.on_power_up_st[i].btn_id = i+1+ELE_RELAY_OFFSET;
		update.on_power_up_st[i].mode = sw_config_st.power_on_state[i];
	}
	mesh_tx_cmd_rsp(
			 VD_CONFIG_NODE_STATUS,
			 (u8 *)&update,
			 sizeof(update_all_on_power_up_st_t),
			 ele_adr_primary + ELE_RELAY_OFFSET,
			 GATEWAY_UNICAST_ADDR,
			 0,
			 0
		 );
	DBG_SW_CONFIG_SEND_STR("\n sw_cfg_response_all_on_power_up_st_config: ");
}

#if MAP_INPUT_OUTPUT_EN
/**
 * @func    sw_config_map_input_is_valid
 * @brief
 * @param
 * @retval  None
 */
static void sw_cfg_response_map_input_output_config(u8 model_idx)
{
	if(model_idx < ELE_CNT)
	{
		update_map_input_t update;
		update.op = VD_MAP_INPUT_OUTPUT_OPT;
		memcpy(update.map_input, sw_config_st.map_input, sizeof(update.map_input));
		mesh_tx_cmd_rsp(
				 VD_CONFIG_NODE_STATUS,
				 (u8 *)&update,
				 sizeof(update_map_input_t),
				 ele_adr_primary + model_idx,
				 GATEWAY_UNICAST_ADDR,
				 0,
				 0
			 );
	}
}
#endif

/**
 * @func    sw_set_enable_disable_group_default
 * @brief
 * @param
 * @retval  None
 */
static void sw_response_enable_disable_group_default(int idx)
{
	if(idx < NUMBER_RL)
	{
		en_dis_group_rsp_t en_dis_group_rsp;
		en_dis_group_rsp.op = VD_EN_ON_OFF_GROUP_DEFAULT;
		en_dis_group_rsp.en = sw_config_st.en_group_default[idx];
		mesh_tx_cmd_rsp(
				 VD_CONFIG_NODE_STATUS,
				 (u8 *)&en_dis_group_rsp,
				 sizeof(en_dis_group_rsp_t),
				 ele_adr_primary + idx + ELE_RELAY_OFFSET,
				 GATEWAY_UNICAST_ADDR,
				 0,
				 0
			 );
	}
}

/**
 * @func    sw_config_map_input_is_valid
 * @brief
 * @param
 * @retval  None
 */
static bool sw_config_map_input_is_valid(map_input_t* p)
{
	// Check the difference between elements and their validity
	foreach(i, NUMBER_RL)
	{
		if(p->map_input[i] > NUMBER_RL)
		{
			return false;
		}
		if(p->map_input[i] == RL_ID_UNMAP)
		{
			continue;
		}
		foreach(j, NUMBER_RL)
		{
			if(p->map_input[j] == RL_ID_UNMAP)
			{
				continue;
			}
			if((i != j) && (p->map_input[i] == p->map_input[j]))
			{
				return false;
			}
		}
	}
	return true;
}

/**
 * @func    sw_config_map_input_is_valid
 * @brief
 * @param
 * @retval  None
 */
static void sw_cfg_response_link_unlink_v2(u8 model_idx)
{
	if(model_idx < ELE_CNT)
	{
		update_link_unlink_v2_t update;
		foreach(i, NUMBER_RL)
		{
			update.link[i].btn_id = i + 1 + ELE_RELAY_OFFSET;
			update.link[i].link = sw_config_st.link_unlink[i];
		}
		update.op = VD_LINK_UNLINK_V2_OPT ;
		mesh_tx_cmd_rsp(
				 VD_CONFIG_NODE_STATUS,
				 (u8 *)&update,
				 sizeof(update_link_unlink_v2_t),
				 ele_adr_primary + model_idx,
				 GATEWAY_UNICAST_ADDR,
				 0,
				 0
			 );
	}
}

/**
 * @func    sw_blink_led_config
 * @brief
 * @param
 * @retval  None
 */
static void sw_blink_led_config(u16 mask, u8 status)
{
	u8 blink_time = 2;
	if(status == SUCCESS) {
		blink_time = 1;
	}
	led_blink_color(mask,
				LED_COLOR_BLUE,
				blink_time,
				LAST_STATE_REFRESH_LED,
				200
			);
}

/**
 * @func    sw_config_set_params
 * @brief
 * @param
 * @retval  None
 */
int sw_cfg_handle_set_message(int model_idx, u8* par, int par_len, u8 cmd)
{
	int err = 0;
	u8 idx;
	ElementSupportType_Enum type = get_ele_support_type(model_idx);
	if(type == ELE_SUPPORT_RELAY)
	{
		DBG_SW_CONFIG_SEND_STR("\n SW - ELE_SUPPORT_RELAY ");
		get_relay_idx_follow_model_index(model_idx, &idx);
		if(idx < NUMBER_RL) {
			switch(cmd) {
				case VD_CONFIG_SW_SET_MAP_UNMAP_INPUT:   // 0x11, Link/Unlink V1
					if(par[0] < LINK_UNKNOWN)
					{
						DBG_SW_CONFIG_SEND_STR("\n LINK/UNLINK: ");
						DBG_SW_CONFIG_SEND_INT(par[0]);
						sw_config_st.link_unlink[idx] = par[0];
						sw_cfg_response_link_unlink(idx);
						sw_blink_led_config(1<<idx, SUCCESS);
					}
					else
					{
						sw_blink_led_config(1<<idx, FAILURE);
					}
					break;

				case VD_CONFIG_SWITCH_MODE_OPT:   // 0x21
				{
					DBG_SW_CONFIG_SEND_STR("\n VD_CONFIG_SWITCH_MODE_OPT: ");
					if((par_len == PAR_LEN_MSG_SET_SW_MODE) \
							|| (par_len == sizeof(toggle_sw_mode_t))) {
						toggle_sw_mode_t* toggle = (toggle_sw_mode_t*)par;
						u8 id = toggle->btn_id - 1 - ELE_RELAY_OFFSET;
						bool success = false;
						if(id < NUMBER_RL)
						{
							u8 tmp_mode = sw_config_st.switch_mode[id];
                            if(toggle->btn_mode < SWITCH_TYPE_UNKNOWN)
                            {
								sw_config_st.switch_mode[id] = toggle->btn_mode;
								success = true;
								if(par_len == sizeof(toggle_sw_mode_t)) {
									toggle_sw_mode_t* toggle = (toggle_sw_mode_t*)par;
									if(toggle->btn_mode == TOGGLE_SWITCH_TYPE) {
										if((toggle->toggle_type >= TOGGLE_UNKNOWN) \
													|| (toggle->toggle_type == TOGGLE_DELAY_OFF)
														|| (toggle->toggle_type == TOGGLE_DELAY_ON)) {
											DBG_SW_CONFIG_SEND_STR("\n TOGG MODE INVALID");
										}
										else {
											if((toggle->delay_s >= AUTO_ON_OFF_MIN_TIME_S)    \
														&& (toggle->delay_s <= AUTO_ON_OFF_MAX_TIME_S)) {
												sw_config_st.extended_param_st[id].toggle_type = toggle->toggle_type;
												sw_config_st.extended_param_st[id].delay_time_s = toggle->delay_s;
												DBG_SW_CONFIG_SEND_STR("\n OK_0");
											}
											else {
												DBG_SW_CONFIG_SEND_STR("\n TOGG TIME INVALID");
											}
										}
									}
								}
								else if(par_len < sizeof(toggle_sw_mode_t)) {
									u8 btn_idx = par[0] - 1;
									if(btn_idx < NUMBER_RL) {
										if(par[1] == TOGGLE_SWITCH_TYPE) {
											sw_config_st.extended_param_st[btn_idx].toggle_type = TOGGLE_DEFAULT;
											DBG_SW_CONFIG_SEND_STR("\n OK_1");
										}
									}
									else {
										DBG_SW_CONFIG_SEND_STR("\n btn_idx invalid");
									}
								}
								// save to flash
								sw_config_store();
                            }
							// response to gateway
							sw_cfg_response_switch_mode(id);
							// Notify
							if(success == true) {
								sw_blink_led_config(1<<id, SUCCESS);
							} else {
								sw_blink_led_config(1<<id, FAILURE);
							}
							// Check Lighting and momentary
							if(tmp_mode != sw_config_st.switch_mode[id]) {
								if(pvConfig_handle_btn_change_mode != NULL) {
									pvConfig_handle_btn_change_mode(id, sw_config_st.switch_mode[id]);
								}
								DBG_SW_CONFIG_SEND_STR("\n Switch mode is change -> force control relay: ");
								DBG_SW_CONFIG_SEND_INT(id);
							}
						}
					}
					else {
						DBG_SW_CONFIG_SEND_STR("\n PAR Len invalid");
					}
					break;
				}
				case VD_CONFIG_ON_POWER_UP_STATE:   // 0x2A
					if(par_len == PAR_LEN_MSG_SET_ON_PW_UP_STATE)
					{
						on_power_up_st_set_t *on_power_up_st_set = (on_power_up_st_set_t*)par;
						u8 id = on_power_up_st_set->btn_id - 1;
						if(id < NUMBER_RL) {
							if(on_power_up_st_set->state < POWER_UP_UNKNOWN) {
								sw_config_st.power_on_state[id] = on_power_up_st_set->state;
								// save to flash
								sw_config_store();
								sw_blink_led_config(1<<id, SUCCESS);
							}
							else
							{
								sw_blink_led_config(1<<id, FAILURE);
							}
							// response to gateway
							sw_cfg_response_on_power_up_st_config(id);
						}
					}
					break;

#if MAP_INPUT_OUTPUT_EN
				#if NUMBER_RL > 1
				case VD_MAP_INPUT_OUTPUT_OPT:   // 0x2C
				{
					map_input_t* p_set = (map_input_t*)par;
					if(par_len == NUMBER_RL)
					{
						if(sw_config_map_input_is_valid(p_set) == true)
						{
							bool is_change = false;
							foreach(i, NUMBER_RL)
							{
								if(sw_config_st.map_input[i] != p_set->map_input[i]) {
									is_change = true;
								}
							}
							if(is_change == true)
							{
								memcpy(sw_config_st.map_input, p_set, NUMBER_RL);
								// save to flash
								sw_config_store();
								// Refresh
								relay_control_refresh_all();
							}
							// Notify
							sw_blink_led_config(BACKUP_MASK_RL, SUCCESS);
							// Response to gateway
							sw_cfg_response_map_input_output_config(model_idx);
							break;
						}
					}
					// Response to gateway
					sw_cfg_response_map_input_output_config(model_idx);
					// Notify
					sw_blink_led_config(BACKUP_MASK_RL, FAILURE);
					break;
				}
				#endif
#endif

				case VD_LINK_UNLINK_V2_OPT:  // 0x2D, Link/Unlink V2
				{
					u8 valid = false;
					u8 item_size = sizeof(link_unlink_v2_t);
					u16 mask = 0;
					foreach(i, par_len/item_size)
					{
						link_unlink_v2_t *p_link_unlink = (link_unlink_v2_t*)&par[i*item_size];
						u8 id = p_link_unlink->btn_id - 1 - ELE_RELAY_OFFSET;
						if(id < NUMBER_RL)
						{
							if(p_link_unlink->link < LINK_UNKNOWN)
							{
								sw_config_st.link_unlink[id] = p_link_unlink->link;
								valid = true;
							}
							mask |= (1 << id);
						}
					}
					if(valid == true) {
						// save to flash
						sw_config_store();
						// response to gateway
						sw_cfg_response_link_unlink_v2(model_idx);
						// Notify
						sw_blink_led_config(mask, SUCCESS);
					}
					else {
						sw_blink_led_config(mask, FAILURE);
					}
					break;
				}

				case VD_EN_ON_OFF_GROUP_DEFAULT:  // 0x31
				{
					u8 enable = par[0];
					if(idx < NUMBER_RL)
					{
						DBG_SW_CONFIG_SEND_STR("\n Enable: ");
						DBG_SW_CONFIG_SEND_INT(enable);
						if(enable <= max2(true, false))
						{
							sw_blink_led_config(1<<idx, SUCCESS);
							if(enable == sw_config_st.en_group_default[idx]) {
								sw_response_enable_disable_group_default(idx);
								break;
							}
							sw_config_st.en_group_default[idx]= enable;
							// save to flash
							sw_config_store();
						}
						else
						{
							sw_blink_led_config(1<<idx, FAILURE);
						}
						// response
						sw_response_enable_disable_group_default(idx);
					}
					else
					{
						sw_blink_led_config(BACKUP_MASK_RL, FAILURE);
					}
					break;
				}
				default: break;
			}
		}
		else {
			err = -1;
		}
	}
	else
	{
		DBG_SW_CONFIG_SEND_STR("\n SW_CF Set - Invalid Endpoint");
		err = -1;
	}
    return err;
}

/**
 * @func    sw_cfg_get_mcu_opt
 * @brief
 * @param
 * @retval  None
 */
int sw_cfg_handle_get_message(int idx, u8* par, int par_len, u8 cmd)
{
	switch(cmd)
	{
	    case VD_CONFIG_ALL_SWITCH_OPT:
	    	sw_config_enable_auto_send();
	    	break;

		case VD_CONFIG_SW_SET_MAP_UNMAP_INPUT:
		{
			u8 id = 0;
			get_relay_idx_follow_model_index(idx, &id);
			if(id < NUMBER_RL) {
				sw_cfg_response_link_unlink(id);
			}
			break;
		}
		case VD_CONFIG_SWITCH_MODE_OPT:
		{
			u8 btn_id = par[0];
			u8 internal = 0;
			get_relay_idx_follow_model_index(btn_id, &internal);
			if(internal < NUMBER_RL) {
				sw_cfg_response_switch_mode(btn_id - 1);
			}
			break;
		}
		case VD_CONFIG_ON_POWER_UP_STATE:
			sw_cfg_response_all_on_power_up_st_config();
			break;

#if MAP_INPUT_OUTPUT_EN
		case VD_MAP_INPUT_OUTPUT_OPT:
			sw_cfg_response_map_input_output_config(ELE_RELAY_OFFSET);
			break;
#endif

		case VD_LINK_UNLINK_V2_OPT:
			sw_cfg_response_link_unlink_v2(ELE_RELAY_OFFSET);
			break;

		case VD_EN_ON_OFF_GROUP_DEFAULT:
			sw_response_enable_disable_group_default(par[0]);
			break;

		default: return -1;
	}
	return 0;
}

/**
 * @func    sw_get_en_disable_group_default
 * @brief
 * @param
 * @retval  None
 */
bool sw_get_en_disable_group_default(int idx)
{
	if(idx < NUMBER_RL) {
		DBG_SW_CONFIG_SEND_STR("\n G_DF: ");
		DBG_SW_CONFIG_SEND_INT(idx);
		return sw_config_st.en_group_default[idx];
	}
	return false;
}



#if 0
/**
 * @func    sw_config_reset_all_device_config
 * @brief
 * @param
 * @retval  None
 */
void sw_config_reset_all_device_config(void)
{
	foreach(i, ELE_CNT) {
		btn_mode_set_t mode_set;
		mode_set.btn_id = i+1;
		mode_set.btn_mode = TOGGLE_SWITCH_TYPE;
		mode_set.toggle_type = TOGGLE_DEFAULT;
		mode_set.delay_t = AUTO_ON_OFF_MIN_TIME_S;
		sw_cfg_set_mcu_opt(
			    i, (u8*)&mode_set, sizeof(btn_mode_set_t), VD_CONFIG_SWITCH_MODE_OPT
			);
	}
	u8 led_intensity = 1;
	sw_cfg_set_mcu_opt(
			    0, (u8*)&led_intensity, 1, VD_CONFIG_LED_INTENSITY_OPT
			);
	u8 lock = 0;
	sw_cfg_set_mcu_opt(
			    0, (u8*)&lock, 1, VD_CONFIG_LOCK_ALL_SWITCH_OPT
			);

}
#endif


/**
 * @func    user_cmd_sig_g_on_powerup_get
 * @brief
 * @param
 * @retval  None
 */
int user_cmd_sig_g_on_powerup_get(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	u8 idx;
	if(get_relay_idx_follow_model_index(cb_par->model_idx, &idx) == true) {
		if(idx < NUMBER_RL) {
			u8 rsp = sw_config_st.power_on_state[idx];
			mesh_tx_cmd_rsp(
					G_ON_POWER_UP_STATUS,
					 &rsp,
					 1,
					 ele_adr_primary + cb_par->model_idx,
					 GATEWAY_UNICAST_ADDR,
					 0,
					 0
			 	 );
			DBG_SW_CONFIG_SEND_STR("\n user_cmd_sig_g_on_powerup_get: ");
			DBG_SW_CONFIG_SEND_INT(idx);
		}
	}
	return 0;
}

/**
 * @func    user_cmd_sig_g_on_powerup_set
 * @brief
 * @param
 * @retval  None
 */
int user_cmd_sig_g_on_powerup_set(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	ElementSupportType_Enum type = get_ele_support_type(cb_par->model_idx);
	if(type == ELE_SUPPORT_RELAY)
	{
		DBG_SW_CONFIG_SEND_STR("\n SW - ELE_SUPPORT_RELAY ");
		u8 idx;
		if(get_relay_idx_follow_model_index(cb_par->model_idx, &idx) == true)
		{
			u8 on_power_up_st_set = par[0];
			if(on_power_up_st_set < POWER_UP_UNKNOWN) {
				sw_config_st.power_on_state[idx] = on_power_up_st_set;
				sw_config_store();
				sw_blink_led_config(1<<idx, SUCCESS);
			}
			else {
				sw_blink_led_config(1<<idx, FAILURE);
			}
			// response to gateway
			user_cmd_sig_g_on_powerup_get(par, par_len, cb_par);
			return 0;
		}
	}
	return -1;
}

/**
 * @func    sw_config_callback_init
 * @brief
 * @param
 * @retval  None
 */
void sw_config_callback_init(typeConfig_handle_btn_change_mode func)
{
	if(func != NULL) {
		pvConfig_handle_btn_change_mode = func;
	}
}

// End File
#endif
