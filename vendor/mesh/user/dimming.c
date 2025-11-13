/*
 * dimming.c
 *
 *  Created on: Jan 22, 2022
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "proj/tl_common.h"
#include "vendor/common/vendor_model.h"
#include "vendor/common/system_time.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "utilities.h"
#include "sw_binding.h"
#include "net_message.h"
#include "execution_scene.h"
#include "led_ev.h"
#include "relay.h"
#include "led.h"
#include "flash_user.h"
#include "dimming.h"

#include "debug.h"
#ifdef  DIMMING_DBG_EN
#define DBG_DIMMING_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_DIMMING_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_DIMMING_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_DIMMING_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_DIMMING_SEND_STR(x)
#define DBG_DIMMING_SEND_INT(x)
#define DBG_DIMMING_SEND_HEX(x)
#define DBG_DIMMING_SEND_BYTE(x)
#endif

#define P_ST_TRANS(idx, type)		(&light_res_sw[idx].trans[type])

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
u8 cotrol_binding_locally_flag[NUMBER_INPUT];
static u8 dimming_direc_arr[NUMBER_INPUT];
btn_evt_t touch_btn_before_st[NUMBER_INPUT];

dim_config_set_t dimming_config[NUMBER_INPUT];

#if EN_SWITCH_CONFIG_DIMMING
const dim_config_set_t const_dimming_config = {
		.tran_t = DIMMING_TRANS_T_DEFAULT, .on_type = ON_RESTORE, .on_value = DIMMING_LIGHTNESS_MAX
	};
static int flash_dimming_config_idx = FLASH_INDEX_DEFAULT;
#define FLASH_SIZE_DIMMING_CONFIG     4000
#define BLOCK_SIZE_DIMMING_CONFIG     sizeof(dimming_config)
#endif

/******************************************************************************/
/*                       PRIVATE FUNCTION DECLERATION                         */
/******************************************************************************/


/******************************************************************************/
/*                            EXPORT FUNCTION                                 */
/******************************************************************************/

/**
 * @func    dimming_configuration_get
 * @brief
 * @param   None
 * @retval  None
 */
bool dimming_mod_lightness(u8 idx, u16* lightness)
{
#if EN_SWITCH_CONFIG_DIMMING
	if(idx >= NUMBER_INPUT) {
		return false;
	}
	u16 binding_adr = get_group_binding_adr(idx);
	if(binding_adr == ADR_UNASSIGNED) {
		return false;
	}
	if(dimming_config[idx].on_type == ON_SET_UP_VALUE) {
		if(dimming_config[idx].on_value >= DIMMING_LIGHTNESS_MIN) {
			*lightness = dimming_config[idx].on_value;
			return true;
		}
	}
#endif
	return false;
}

/**
 * @func    dimming_configuration_get
 * @brief
 * @param   None
 * @retval  None
 */
int dimming_configuration_get(u8 idx)
{

#if EN_SWITCH_CONFIG_DIMMING

	if(idx >= NUMBER_INPUT) {
		return -1;
	}

	u8 size = sizeof(dim_config_set_t);
	u8 resp[size+1];
	memcpy(&resp[1], (u8*)&dimming_config[idx], size);
	resp[0] = VD_DIMMING_CONFIGURATION;

	mesh_tx_cmd_rsp(
			 VD_CONFIG_NODE_STATUS,
			 resp,
			 size+1,
			 ele_adr_primary + idx,
			 GATEWAY_UNICAST_ADDR,
			 0,
			 0
		 );
#endif
	return 0;
}

/**
 * @func    dimming_direct_arr_update
 * @brief
 * @param   None
 * @retval  None
 */
int dimming_configuration_set(u8 idx, u8* par, u8 par_len)
{
#if EN_SWITCH_CONFIG_DIMMING

	if((par_len < sizeof(dim_config_set_t)) || (idx >= NUMBER_INPUT)) {
		return -1;
	}
	dim_config_set_t* dim_config_set = (dim_config_set_t*)par;
	if((dim_config_set->on_type >= ON_INVALID) \
					|| (dim_config_set->on_value < DIMMING_LIGHTNESS_MIN)
								|| (dim_config_set->tran_t < DIMMING_TRANS_T_MIN)) {
		return -1;
	}
	memcpy(&dimming_config[idx], dim_config_set, sizeof(dim_config_set_t));
	// Save to flash
	flash_user_store(
			(int*)&flash_dimming_config_idx, FLASH_ADR_DIMMING_CONFIG,
			FLASH_SIZE_DIMMING_CONFIG, BLOCK_SIZE_DIMMING_CONFIG, (u8*)&dimming_config
		);
	// Send response
	return dimming_configuration_get(idx);
#endif
	return 0;
}

/**
 * @func    dimming_init
 * @brief
 * @param   None
 * @retval  None
 */
void dimming_init(void)
{
	foreach(i, NUMBER_INPUT) {
		dimming_direc_arr[i] = UD_UNKNOWN;
		touch_btn_before_st[i].evt = NO_PRESS;
		touch_btn_before_st[i].update_st_t = 0;
		touch_btn_before_st[i].hold_500ms_flag = false;
		cotrol_binding_locally_flag[i] = 0;
	}

#if EN_SWITCH_CONFIG_DIMMING
	// Get flash Index
	flash_user_get_flash_index(
				(int*)&flash_dimming_config_idx, FLASH_ADR_DIMMING_CONFIG,
				FLASH_SIZE_DIMMING_CONFIG, BLOCK_SIZE_DIMMING_CONFIG, (u8*)&dimming_config
			);
	// Restore Dimming Configuration
	flash_user_restore(
			    flash_dimming_config_idx, FLASH_ADR_DIMMING_CONFIG,
			    BLOCK_SIZE_DIMMING_CONFIG, (u8*)&dimming_config
			);
	if(dimming_config[0].on_type >= ON_INVALID) {
		foreach(i, NUMBER_INPUT) {
			memcpy(&dimming_config[i], &const_dimming_config, sizeof(dim_config_set_t));
		}
		flash_user_store(
				(int*)&flash_dimming_config_idx, FLASH_ADR_DIMMING_CONFIG,
				FLASH_SIZE_DIMMING_CONFIG, BLOCK_SIZE_DIMMING_CONFIG, (u8*)&dimming_config
			);
		DBG_DIMMING_SEND_STR("\nDIMMING_SAVE_DEFAULT_VALUE: ");
	}
#endif
}

/**
 * @func    dimming_active
 *
 * @brief
 * @param   None
 * @retval  None
 */
void dimming_active(u8 idx, u8 dir, u16 dst_addr)
{
	if(idx < NUMBER_INPUT) {
		vd_up_down_control_t up_down_control;
		up_down_control.model_id = SIG_MD_LIGHTNESS_S;
#if EN_SWITCH_CONFIG_DIMMING
		up_down_control.trans_t_ms = dimming_config[idx].tran_t;
#else
		up_down_control.trans_t_ms = DIMMING_TRANS_T_DEFAULT;
#endif
		up_down_control.dir = dir;
		up_down_control.step = 0x3020;
		mesh_tx_cmd_rsp(
				VD_DIMMING_CONTROL_SET,
				(u8*)&up_down_control,
				sizeof(vd_up_down_control_t),
				ele_adr_primary,
				dst_addr,
				0,
				0
			);
	}
}

/**
 * @func    dimming_handle_btn_st
 * @brief
 * @param   None
 * @retval  None
 */
void dimming_handle_btn_st(u8 model_idx, u8 evt)
{
	u16 binding_adr = get_group_binding_adr(model_idx);

	if(model_idx >= NUMBER_INPUT) {
		DBG_DIMMING_SEND_STR("\n Index invalid");
		return;
	}
	if(binding_adr == ADR_UNASSIGNED) {
		if(evt == START_PRESS) {
			relay_toggle_state(model_idx - ELE_RELAY_OFFSET, SRC_DEVICE);
		}
		DBG_DIMMING_SEND_STR("\n Binding ADR_UNASSIGNED");
		return;
	}
	st_transition_t *p_trans = P_ST_TRANS(model_idx, ST_TRANS_LIGHTNESS);
	u8 present_st = (p_trans->present == LEVEL_OFF)?G_OFF:G_ON;

	DBG_DIMMING_SEND_STR("\n dimming_handle_btn_st");

	switch (evt)
	{
		case HOLD_500MS:
		{
			dimming_direc_arr[model_idx] = \
						(dimming_direc_arr[model_idx]^0x01)&0x01;
			if(present_st == G_OFF) {
				dimming_direc_arr[model_idx] = UD_UP;
			}
			if(dimming_direc_arr[model_idx] == UD_UP) {
				mesh_cmd_g_onoff_set_t onoff_set;
				onoff_set.onoff = G_ON;
				onoff_set.delay = onoff_set.transit_t = 0;
				onoff_set.tid = 0;

				mesh_cb_fun_par_t cb_par;
				cb_par.op = G_ONOFF_SET_NOACK;
				cb_par.adr_src = GATEWAY_UNICAST_ADDR;
				cb_par.adr_dst = ele_adr_primary + model_idx;
				cb_par.model_idx = model_idx;

				my_handle_mesh_cmd_sig_g_on_off_set(
							(u8*)&onoff_set, sizeof(mesh_cmd_g_onoff_set_t), &cb_par
						);
				cotrol_binding_locally_flag[model_idx] = 1;

				dimming_active(
						model_idx, DIR_UP_SMOOTH, binding_adr
					);
				send_led_evt_to_mcu(LED_DIMMING_UP, 1 << (model_idx - ELE_RELAY_OFFSET));
			}
			else {
				if(present_st == G_ON) {
					dimming_active(
							model_idx, DIR_DOWN_SMOOTH_LIMIT, binding_adr
						);
					send_led_evt_to_mcu(LED_DIMMING_DOWN_LIMIT, 1 << (model_idx - ELE_RELAY_OFFSET));
				}
			}
			touch_btn_before_st[model_idx].hold_500ms_flag = true;
			break;
		}

		case RELEASE:
		{
			DBG_DIMMING_SEND_STR("\n *** Release: ");
			if(touch_btn_before_st[model_idx].hold_500ms_flag == true) {
				dimming_active(
						model_idx, DIR_STOP, binding_adr
					);
				touch_btn_before_st[model_idx].hold_500ms_flag = false;
				cotrol_binding_locally_flag[model_idx] = 0;
				DBG_DIMMING_SEND_STR(" - 1");
				break;
			}
			if(touch_btn_before_st[model_idx].evt == START_PRESS  \
					|| touch_btn_before_st[model_idx].evt == HOLD_50MS) {
				relay_toggle_state((model_idx - ELE_RELAY_OFFSET), SRC_DEVICE);
				DBG_DIMMING_SEND_STR(" - 2");
			}
			break;
		}
	}
	touch_btn_before_st[model_idx].evt = evt;
	touch_btn_before_st[model_idx].update_st_t = clock_time_ms();
}
