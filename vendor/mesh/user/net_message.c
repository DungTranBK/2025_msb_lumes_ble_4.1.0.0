/*
 * handle_rx_network.c
 *
 *  Created on: Sep 19, 2020
 *      Author: DungTran BK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "../../../proj_lib/ble/blt_config.h"
#include "../../common/system_time.h"
#include "../../../proj_lib/sig_mesh/app_mesh.h"
#include "../../common/lighting_model.h"
#include "../../common/lighting_model_HSL.h"
#include "../../common/mesh_config.h"
#include "../../common/light.h"
#include "config_board.h"
#include "sw_config.h"
#include "utilities.h"
#include "sw_binding.h"
#include "relay.h"
#include "execution_scene.h"
#include "sw_auto.h"
#include "dimming.h"
#include "energy.h"
#include "button.h"

#include "net_message.h"

#include "debug.h"
#ifdef NET_MSG_DBG_EN
#define DBG_NET_MSG_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_NET_MSG_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_NET_MSG_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_NET_MSG_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_NET_MSG_SEND_STR(x)
#define DBG_NET_MSG_SEND_INT(x)
#define DBG_NET_MSG_SEND_HEX(x)
#define DBG_NET_MSG_SEND_BYTE(x)
#endif

#define P_ST_TRANS(idx, type)		(&light_res_sw[idx].trans[type])


/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

u8 incomming_st[ELE_CNT] = {
		G_ONOFF_RSV,
		#if ELE_CNT > 1
		G_ONOFF_RSV,
		#endif
		#if ELE_CNT > 2
		G_ONOFF_RSV,
		#endif
		#if ELE_CNT > 3
		G_ONOFF_RSV,
		#endif
		#if ELE_CNT > 4
		G_ONOFF_RSV,
		#endif
		#if ELE_CNT > 5
		G_ONOFF_RSV,
		#endif
};

u8 scene_active_arr[ELE_CNT] = {
		false,
	#if ELE_CNT > 1
		false,
	#endif
	#if ELE_CNT > 2
		false,
	#endif
	#if ELE_CNT > 3
		false,
	#endif
	#if ELE_CNT > 4
		false,
	#endif
	#if ELE_CNT > 5
		false,
	#endif
};

static u8 gen_execution_scene_st[ELE_CNT] = {
		G_ONOFF_RSV,
	#if ELE_CNT > 1
		G_ONOFF_RSV,
	#endif
	#if ELE_CNT > 2
		G_ONOFF_RSV,
	#endif

	#if ELE_CNT > 3
		G_ONOFF_RSV,
	#endif

	#if ELE_CNT > 4
		G_ONOFF_RSV,
	#endif

	#if ELE_CNT > 5
		G_ONOFF_RSV,
	#endif
};

u8 state_of_group_binding[NUMBER_INPUT] =
{
		SUB_UNKNOWN,
	#if ELE_CNT > 1
		SUB_UNKNOWN,
	#endif
	#if ELE_CNT > 2
		SUB_UNKNOWN,
	#endif
	#if ELE_CNT > 3
		SUB_UNKNOWN,
	#endif
	#if ELE_CNT > 4
		SUB_UNKNOWN,
	#endif
	#if ELE_CNT > 5
		SUB_UNKNOWN,
	#endif
};


bool relay_control_first_time_flag[NUMBER_RL] =
{
		true,
	#if NUMBER_RL > 1
		true,
	#endif

	#if NUMBER_RL > 2
		true,
	#endif
};

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/
static bool check_en_control(u16 model_idx, u16 adr_dst);
/******************************************************************************/
/*                           EXPORT FUNCTIONS DECLERATION                     */
/******************************************************************************/
/**
 * @func    net_message_get_target_state
 * @brief
 * @param
 * @retval  None
 */
u16 net_message_get_target_state(void)
{
	return relay_get_target_state();
}

/**
 * @func    send_on_off_control_to_ble_group
 * @brief
 * @param
 * @retval  None
 */
static void send_on_off_control_to_ble_group(u8 model_idx, BOOL status)
{
	DBG_NET_MSG_SEND_STR("\n send_on_off_control_to_ble_group: ");
	if(model_idx < ELE_CNT) {
		u16 binding_adr = get_group_binding_adr(model_idx);
		if(binding_adr != ADR_UNASSIGNED) {
			bool en_binding = false;
			mesh_cmd_g_onoff_set_t on_off_set;
			// CLEAN force control
			if(nwk_control_msg_para[model_idx].dst == binding_adr) {
				force_control_binding[model_idx] = false;
			}
			// POWER ON
			if(state_of_group_binding[model_idx] == SUB_UNKNOWN) {
				state_of_group_binding[model_idx] = status;
				return;
			}
			DBG_NET_MSG_SEND_STR("\n STATUS: ");
			// DBG_NET_MSG_SEND_INT(state_of_group_binding[model_idx]);

#ifdef NET_MSG_DBG_EN
			for(u8 i = ELE_RELAY_OFFSET; i < ELE_CNT; i++){
				DBG_NET_MSG_SEND_INT(i);
				DBG_NET_MSG_SEND_STR(", ");
				DBG_NET_MSG_SEND_INT(status);
				DBG_NET_MSG_SEND_STR(", ");
				DBG_NET_MSG_SEND_INT(state_of_group_binding[model_idx]);
				if(i < ELE_CNT-1){
					DBG_NET_MSG_SEND_STR(" - ");
				}
			}
			DBG_NET_MSG_SEND_STR("\n DST: ");
			DBG_NET_MSG_SEND_HEX(nwk_control_msg_para[model_idx].dst);
#endif
			// Check Binding
			if(((state_of_group_binding[model_idx] != status)   \
						&& (nwk_control_msg_para[model_idx].dst != binding_adr)   \
								&& (nwk_control_msg_para[model_idx].dst < ADR_ALL_PROXY))
										|| (force_control_binding[model_idx] == true)) {
				en_binding = true;
				state_of_group_binding[model_idx] = status;
				// INTERNAL
				for(u8 i = ELE_RELAY_OFFSET; i < ELE_CNT; i++)
				{
					if(i != model_idx) {
						u16 temp_binding = get_group_binding_adr(i);
						if((temp_binding != ADR_UNASSIGNED) && (temp_binding == binding_adr)) {
							// MANUAL CONTROL
							update_control_message_params(
										i, ele_adr_primary + model_idx, binding_adr, G_ONOFF_SET
									);
							send_control_dev_manual_to_mcu(i, status, true);
							state_of_group_binding[i] = status;
							DBG_NET_MSG_SEND_STR("\n *** UPDATE GRP state: ");
							DBG_NET_MSG_SEND_INT(i);
							DBG_NET_MSG_SEND_STR(", ");
							DBG_NET_MSG_SEND_INT(status);
						}
					}
				}
			}
			else if((nwk_control_msg_para[model_idx].dst == binding_adr)    \
						|| (nwk_control_msg_para[model_idx].dst >= ADR_ALL_PROXY)) {
				state_of_group_binding[model_idx] = status;
			}
			DBG_NET_MSG_SEND_STR("\n ENABLE BINDING: ");
			DBG_NET_MSG_SEND_INT(en_binding);
			// Control
			if(en_binding == true) {

#if EN_SWITCH_CONFIG_DIMMING
				u8 flag_control = 0;

				if(status == G_ON) {
					if(sw_config_st.switch_mode[model_idx] == LIGHTING_SWITCH_TYPE) {
						u16 lightness = LIGHTNESS_MAX;
						if(dimming_mod_lightness(model_idx, &lightness) == true) {
							flag_control = true;
							mesh_cmd_lightness_set_t lightness_set;
							lightness_set.lightness = dimming_config[model_idx].on_value;
							lightness_set.transit_t = (lightness_set.lightness*0x0A)/LIGHTNESS_MAX;
							lightness_set.tid = 0;
	#if CTL_WITH_FLAG_DEFAULT_POSITION_EN
							lightness_set.delay = 1;
    #else
							lightness_set.delay = 0;
	#endif
							// Control locally between switch
							if(touch_btn_before_st[model_idx].hold_500ms_flag && cotrol_binding_locally_flag[model_idx]) {
								u8 len = sizeof(mesh_cmd_lightness_set_t) + 1;
								u8 response[len];
								response[0] = TYPE_CTL_LIGHTNESS;
								memcpy(&response[1],   \
									(u8*)&lightness_set, sizeof(mesh_cmd_lightness_set_t));
								mesh_tx_cmd_rsp(
										 VD_BINDING_CONTROL_LOCALLY,
										 response, len, ele_adr_primary + model_idx, binding_adr, 0, 0
									 );
								cotrol_binding_locally_flag[model_idx] = 0;
								DBG_NET_MSG_SEND_STR("\n CONTROL LOCALLY LIGHTNESS");
								return;
							}
							// Normal
							mesh_tx_cmd_rsp(
									LIGHTNESS_SET_NOACK,
									(u8*)&lightness_set, sizeof(mesh_cmd_lightness_set_t), ele_adr_primary + model_idx, binding_adr, 0, 0
								);
							DBG_NET_MSG_SEND_STR("\n BINDING_LIGHTNESS");
						}
					}
				}
				if(flag_control == 0)
				{
					on_off_set.onoff = status;
					on_off_set.tid = 0;
					on_off_set.transit_t = 0x0A;
					on_off_set.delay = 1;
					u8 len = sizeof(mesh_cmd_g_onoff_set_t);
					u8 par[len+1];
					memcpy((u8*)&par, (u8*)&on_off_set, len);
					if(force_control_binding[model_idx] == true) {
						par[len] = 0x0;
						len++;
						force_control_binding[model_idx] = false;
					}

					// Control locally between switch
					if(touch_btn_before_st[model_idx].hold_500ms_flag && cotrol_binding_locally_flag[model_idx]) {
						len++;
						u8 response[len];
						response[0] = TYPE_CTL_ON_OFF;
						memcpy(&response[1], &par, len - 1);
						mesh_tx_cmd_rsp(
								VD_BINDING_CONTROL_LOCALLY,
								 response, len, ele_adr_primary + model_idx, binding_adr, 0, 0
							 );
						cotrol_binding_locally_flag[model_idx] = 0;
						DBG_NET_MSG_SEND_STR("\n CONTROL LOCALLY ON_OFF");
						return;
					}
					// Normal
					mesh_tx_cmd_rsp(
							G_ONOFF_SET_NOACK,
							(u8*)par, len, ele_adr_primary + model_idx, binding_adr, 0,0
						);
					DBG_NET_MSG_SEND_STR("\n BINDING_ON_OFF");
				}
#else
				on_off_set.onoff = status;
				on_off_set.tid = 0;
				on_off_set.transit_t = 0x0A;
				on_off_set.delay = 1;
				u8 len = sizeof(mesh_cmd_g_onoff_set_t), par[len+1];
				memcpy((u8*)&par, (u8*)&on_off_set, len);
				if(force_control_binding[model_idx] == TRUE) {
					par[len] = 0x0;
					len++;
					force_control_binding[model_idx] = FALSE;
				}
				mesh_tx_cmd_rsp(
						G_ONOFF_SET_NOACK,
						(u8 *)par,
						len,
						ele_adr_primary + model_idx,
						binding_adr,
						0,
						0
					);
#endif
			}
		}
	}
}

/**
 * @func    net_message_handle_state_change
 * @brief
 * @param
 * @retval  None
 */
int net_message_handle_state_change(u8 idx, u8 status)
{
	if(idx >= NUMBER_RL)
	{
		return -1;
	}
	u16 model_idx = get_ele_address_base_type_and_idx(ELE_SUPPORT_RELAY, idx);

	DBG_NET_MSG_SEND_STR("\n net_message_handle_state_change: ");
	DBG_NET_MSG_SEND_INT(idx);
	DBG_NET_MSG_SEND_STR(", ");
	DBG_NET_MSG_SEND_INT(status);
	DBG_NET_MSG_SEND_STR("\n ");

	if(status < G_ONOFF_RSV)
	{
		/*
		DBG_NET_MSG_SEND_STR("\n 1");
		if(clock_time_exceed_ms(0, TIMER_5S)){
			foreach(i, ELE_CNT) {
				en_binding_and_execution[i] = true;
			}
		}
		*/
		#if EN_EVT_ON_OFF_CONTROL_EXECUTION
		u16 ele_addr =  \
				get_ele_address_base_type_and_idx(ELE_SUPPORT_RELAY, model_idx);
		if(ele_addr != ADR_UNASSIGNED)
		{
			u8 key_code =   \
					(status == G_ON)?EV_SW_ON:EV_SW_OFF;
			if((status != gen_execution_scene_st[model_idx])  \
								&& (gen_execution_scene_st[model_idx] != G_ONOFF_RSV))
			{
				if(nwk_control_msg_para[model_idx].dst < ADR_FIXED_GROUP_START)
				{
					execution_scene_active(model_idx, key_code);
					DBG_NET_MSG_SEND_STR("\n ### execution_scene_active: model_idx - ");
					DBG_NET_MSG_SEND_INT(model_idx);
					DBG_NET_MSG_SEND_STR(", st - ");
					DBG_NET_MSG_SEND_INT(status);
					DBG_NET_MSG_SEND_STR(", ex_st - ");
					DBG_NET_MSG_SEND_INT(gen_execution_scene_st[model_idx]);
				}
			}
			gen_execution_scene_st[model_idx] = status;
		}
		#endif
		#if SWITCH_ENABLE_BINDING
		// POWER ON
		if(state_of_group_binding[model_idx] == SUB_UNKNOWN)
		{
			state_of_group_binding[model_idx] = status^1;
		}
		send_on_off_control_to_ble_group(model_idx, status);
		#endif

		if(nwk_control_msg_para[model_idx].dst < ADR_GROUP_START_POINT)
		{
			DBG_NET_MSG_SEND_STR("\n 2");
			#if EN_CHECK_SOURCE_CONTROL
			if(get_incomming_st(model_idx) != status)
			{
				mesh_tx_cmd_lightness_st(
						model_idx, ele_adr_primary + model_idx, GATEWAY_UNICAST_ADDR, LIGHTNESS_STATUS, 0, 0
						);
				set_incomming_st(model_idx, status);
			}
			else
			#endif
			{
				light_publish_status_delay(model_idx, 0);
			}
			// if(relay_control_first_time_flag[idx] == false)
			{
				energy_handle_relay_state_change(idx, status, true);
			}
			relay_control_first_time_flag[idx] = false;
		}
		else {
			DBG_NET_MSG_SEND_STR("\n 3");
			if(nwk_control_msg_para[model_idx].dst < ADR_FIXED_GROUP_START)
			{
				light_publish_status_delay(model_idx, TIMER_1S + (rand()%TIMER_3S));
			}
			else
			{
				light_publish_status_delay(model_idx, TIMER_5S + (rand()%TIMER_30S));
			}
			energy_handle_relay_state_change(idx, status, false);
			nwk_control_msg_para[model_idx].dst = ele_adr_primary + model_idx;
		}
		auto_reset_time_trans(idx, status);
	}
	return 0;
}

/**
 * @func    func_handle_control_message
 * @brief
 * @param
 * @retval  None
 */
int func_handle_control_message(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par, u16 op)
{
	u16 binding_adr = get_group_binding_adr(cb_par->model_idx);
	DBG_NET_MSG_SEND_STR("\n___func_handle_control_message");

	if(binding_adr != ADR_UNASSIGNED) {
		if(cb_par->adr_dst == binding_adr)
		{
			u8 st_control = G_ONOFF_RSV;
			if(op == LIGHTNESS_SET) {
				mesh_cmd_lightness_set_t* p_set = (mesh_cmd_lightness_set_t*)par;
				st_control = (p_set->lightness == LUM_OFF)?G_OFF:G_ON;

                #if EN_SWITCH_CONFIG_DIMMING

				foreach(i, NUMBER_RL) {
					if(cb_par->adr_src == (ele_adr_primary + ELE_RELAY_OFFSET+i)) {
						return -1;
					}
				}
                #endif

			#ifdef NET_MSG_DBG_EN
				DBG_NET_MSG_SEND_STR("\nLIGHTNESS_SET, src: ");
				DBG_NET_MSG_SEND_HEX(cb_par->adr_src);
				DBG_NET_MSG_SEND_STR(", dst: ");
				DBG_NET_MSG_SEND_HEX(cb_par->adr_dst);
				DBG_NET_MSG_SEND_STR(", lightness: ");
				DBG_NET_MSG_SEND_HEX(p_set->lightness );
				DBG_NET_MSG_SEND_STR(", st: ");
				DBG_NET_MSG_SEND_INT(st_control);
				DBG_NET_MSG_SEND_STR(", tid: ");
				DBG_NET_MSG_SEND_INT(p_set->tid);
			#endif

			}
			else if(op == LIGHT_CTL_SET) {
				mesh_cmd_light_ctl_set_t *p_set = (mesh_cmd_light_ctl_set_t *)par;
				st_control = (p_set->lightness == LUM_OFF)?G_OFF:G_ON;
			}
			else if(op == LIGHT_HSL_SET) {
				mesh_cmd_light_hsl_set_t *p_set = (mesh_cmd_light_hsl_set_t *)par;
				st_control = (p_set->lightness == LUM_OFF)?G_OFF:G_ON;
			}
			if(st_control < G_ONOFF_RSV)
			{
				bool en_send = true;
				st_transition_t *p_trans = P_ST_TRANS(cb_par->model_idx, ST_TRANS_LIGHTNESS);
				if(p_trans->target == LEVEL_OFF) {
					if(st_control == G_OFF) {
						control_binding_st[cb_par->model_idx].cnt_same++;
						if(control_binding_st[cb_par->model_idx].cnt_same >= CONTROL_BDG_SAME_ST_CNT_MAX) {
							en_send = false;
						}
					}
					else {
						control_binding_st[cb_par->model_idx].cnt_same = 0;
					}
				}
				else if(p_trans->target == LEVEL_MAX) {
					if(st_control == G_ON) {
						control_binding_st[cb_par->model_idx].cnt_same++;
						if(control_binding_st[cb_par->model_idx].cnt_same >= CONTROL_BDG_SAME_ST_CNT_MAX) {
							en_send = false;
						}
					}
					else {
						control_binding_st[cb_par->model_idx].cnt_same = 0;
					}
				}
				if(en_send == true) {
					if(check_en_control(cb_par->model_idx, cb_par->adr_dst) == true) {
					    send_control_dev_manual_to_mcu(cb_par->model_idx, st_control, true);
						update_control_message_params(
									cb_par->model_idx,cb_par->adr_src,cb_par->adr_dst,G_ONOFF_SET
								);
					}
				}
			}
			return 0;
		}
	}
	return -1;
}

/**
 * @func    my_handle_mesh_cmd_sig_g_on_off_set
 * @brief
 * @param
 * @retval  None
 */
int my_handle_mesh_cmd_sig_g_on_off_set(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	DBG_NET_MSG_SEND_STR("\n my_handle_mesh_cmd_sig_g_on_off_set: ");
	if(get_ele_support_type(cb_par->model_idx) != ELE_SUPPORT_RELAY) {
		return -1;
	}
	DBG_NET_MSG_SEND_STR("\n - 1");
	if(check_en_control(cb_par->model_idx, cb_par->adr_dst) == false) {
		return -1;
	}
	DBG_NET_MSG_SEND_STR("\n - 2");
	if((sw_get_en_disable_group_default(cb_par->model_idx - ELE_RELAY_OFFSET) == false)
			&& (cb_par->adr_dst > ADR_ALL_PROXY)) {
		return -1;
	}
	DBG_NET_MSG_SEND_STR("\n - 3");
#if (SWITCH_ENABLE_BINDING || EN_EVT_ON_OFF_CONTROL_EXECUTION)
	foreach(i, NUMBER_RL)
	{
		if(cb_par->adr_src == ele_adr_primary + ELE_RELAY_OFFSET + i) {
			return -1;
		}
	}
	DBG_NET_MSG_SEND_STR("\n - 4");
	mesh_cmd_g_onoff_set_t *p_set = (mesh_cmd_g_onoff_set_t *)par;
	if(par_len == sizeof(mesh_cmd_g_onoff_set_t)+1) {
	    if(par[par_len - 1] == 0x0) {
			st_transition_t *p_trans =   \
					P_ST_TRANS(cb_par->model_idx, ST_TRANS_LIGHTNESS);
			if(get_onoff_from_level(p_trans->present) == p_set->onoff) {
				__delay_ms(50);
				force_control_binding[cb_par->model_idx] = false;
				return -1;
			}
	    }
	}
#endif
	DBG_NET_MSG_SEND_STR("\n - 5");
	update_control_message_params(cb_par->model_idx, cb_par->adr_src, cb_par->adr_dst, G_ONOFF_SET);
	// TODO control relay
	send_control_dev_manual_to_mcu(cb_par->model_idx, p_set->onoff, true);

	DBG_NET_MSG_SEND_STR("\n send_control_dev_manual_to_mcu: ");
	DBG_NET_MSG_SEND_INT(cb_par->model_idx);

	DBG_NET_MSG_SEND_STR(", ");
	DBG_NET_MSG_SEND_INT(p_set->onoff);

	return 0;
}

/**
 * @func    send_on_off_status
 * @brief
 * @param
 * @retval  None
 */
int send_on_off_status(u8 idx, u8 status)
{
	int err = 0;
	if(( idx <= NUMBER_INPUT ) && ( status < G_ONOFF_RSV ))
	{
		mesh_cmd_g_onoff_st_t rsp = {0};
		rsp.present_onoff = status;
		mesh_tx_cmd_rsp(
				G_ONOFF_STATUS, (u8 *)&rsp,
				sizeof(mesh_cmd_g_onoff_st_t) - 2, ele_adr_primary + idx, GATEWAY_UNICAST_ADDR, 0,0
			);
	}
	else
	{
		err = -1;
	}
	return err;
}

/**
 * @func    my_handle_mesh_cmd_sig_g_on_off_get
 * @brief
 * @param
 * @retval  None
 */
int my_handle_mesh_cmd_sig_g_on_off_get(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	u8 idx;
	if(get_relay_idx_follow_model_index(cb_par->model_idx, &idx) == false) {
		return -1;
	}
	light_publish_status_delay(cb_par->model_idx, 0);
	return 0;
}

/**
 * @func    send_get_device_status_manual
 * @brief
 * @param
 * @retval  None
 */
void send_get_device_status_manual(u8 model_idx)
{
	u8 idx;
	DBG_NET_MSG_SEND_STR("\n GET manual: ");
	if(get_relay_idx_follow_model_index(model_idx, &idx) == true) {
		light_publish_status_delay(model_idx, 0);
	}
}

/**
 * @func    check_en_control
 * @brief
 * @param   None
 * @retval  None
 */
static bool check_en_control(u16 model_idx, u16 adr_dst)
{
	if((sw_config_get_sw_mode(model_idx - ELE_RELAY_OFFSET) == TOGGLE_SWITCH_TYPE) ||
			(sw_config_get_sw_mode(model_idx - ELE_RELAY_OFFSET) == LIGHTING_SWITCH_TYPE)) {
		return true;
	}
	else {
		if((adr_dst <= ADR_FIXED_GROUP_START)) {
			return true;
		}
	}
	return false;
}

/**
 * @func    send_control_dev_manual_to_mcu
 * @brief
 * @param
 * @retval  None
 */
void send_control_dev_manual_to_mcu(u16 model_idx, bool state, bool update_en)
{
	u8 idx;
	if(get_relay_idx_follow_model_index(model_idx, &idx) == true) {
		if(idx < NUMBER_RL) {
			if((check_en_control(idx, nwk_control_msg_para[idx].dst) == true) || (scene_active_arr[idx] == true)) {
				scene_active_arr[idx] = false;
				u16 target = relay_get_target_state();
				// Control same state
				if(((target >> idx)&0x01) == state) {
					u16 time_len = 0;
					if(nwk_control_msg_para[model_idx].dst >= ADR_GROUP_START_POINT)
					{
						if(nwk_control_msg_para[model_idx].dst < ADR_FIXED_GROUP_START) {
							time_len = TIMER_1S + (rand()%TIMER_3S);
							DBG_NET_MSG_SEND_INT(0);
						}
						else {
							time_len = TIMER_5S + (rand()%TIMER_30S);
							DBG_NET_MSG_SEND_INT(1);
						}
		            }
					light_publish_status_delay(model_idx, time_len);
				}
				else {
					DBG_NET_MSG_SEND_STR("\n New state");
					relay_set_target_state(idx, state, SRC_APP, true);
				}
				if(update_en == true) {
					incomming_st[idx + ELE_RELAY_OFFSET] = state;
				}
			}
		}
	}
}

/**
 * @func    get_incomming_st
 * @brief
 * @param
 * @retval  None
 */
u8 get_incomming_st(u16 model_idx)
{
	if(model_idx < ELE_CNT) {
	    return incomming_st[model_idx];
	}
	return G_ONOFF_RSV;
}
/**
 * @func    set_incomming_st
 * @brief
 * @param
 * @retval  None
 */
void set_incomming_st(u16 model_idx, bool st)
{
	if(model_idx < ELE_CNT) {
	    incomming_st[model_idx] = st;
	}
}
// End file
