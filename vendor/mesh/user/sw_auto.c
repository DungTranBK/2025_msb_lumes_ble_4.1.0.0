/*
 * sw_auto.c
 *
 *  Created on: Feb 22, 2021
 *      Author: DungTranBK
 */
/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../../proj_lib/sig_mesh/app_mesh.h"
#include "../../common/system_time.h"
#include "utilities.h"
#include "sw_config.h"
#include "sw_auto.h"

#if DEV_TYPE_SEL == G_TYPE_SWITCH_BUTTON

#include "debug.h"
#ifdef  SW_AUTO_DBG_EN
	#define DBG_IN_OUT_SEND_STR(x)   Dbg_sendString((s8*)x)
	#define DBG_IN_OUT_SEND_INT(x)   Dbg_sendInt(x)
	#define DBG_IN_OUT_SEND_HEX(x)   Dbg_sendHex(x)
	#define DBG_IN_OUT_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
#else
	#define DBG_IN_OUT_SEND_STR(x)
	#define DBG_IN_OUT_SEND_INT(x)
	#define DBG_IN_OUT_SEND_HEX(x)
	#define DBG_IN_OUT_SEND_BYTE(x)
#endif

type_control_dev_callback_func pv_handle_control_device = NULL;

#define P_ST_TRANS(idx, type) (&light_res_sw[idx].trans[type])
/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

typedef struct {
	u32 auto_trans_st_t;
	u8  present_st;
}auto_trans_par_t;


static auto_trans_par_t auto_trans_par[ELE_CNT] =
{
		{ 0, G_ONOFF_RSV },
#if NUMBER_RL > 1
		{ 0, G_ONOFF_RSV },
#endif
#if NUMBER_RL > 2
		{ 0, G_ONOFF_RSV },
#endif
};

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/
/**
 * @func   auto_reset_time_trans
 * @brief
 * @param  None
 * @retval None
 */
void auto_reset_time_trans(u16 idx, u8 st)
{
	if(idx < NUMBER_RL) {
		if(auto_trans_par[idx].present_st != st) {
			auto_trans_par[idx].auto_trans_st_t = clock_time_ms();
			auto_trans_par[idx].present_st = st;
		}
	}
}
/**
 * @func   auto_loop_task
 * @brief
 * @param  None
 * @retval None
 */
void auto_loop_task(void)
{
	foreach(i, NUMBER_RL) {
		if(sw_config_st.switch_mode[i] == TOGGLE_SWITCH_TYPE) {
			if((sw_config_st.extended_param_st[i].toggle_type == TOGGLE_DEFAULT)
				|| (sw_config_st.extended_param_st[i].toggle_type >= TOGGLE_UNKNOWN)
						|| (sw_config_st.extended_param_st[i].toggle_type == TOGGLE_DELAY_ON)
								|| (sw_config_st.extended_param_st[i].toggle_type == TOGGLE_DELAY_OFF)) {
				continue;
			}

			DBG_IN_OUT_SEND_STR("\n AUTO: ");
			DBG_IN_OUT_SEND_INT(i);

    		st_transition_t *p_trans = P_ST_TRANS(i + ELE_RELAY_OFFSET, ST_TRANS_LIGHTNESS);
			if(sw_config_st.extended_param_st[i].toggle_type == TOGGLE_AUTO_ON) {
				DBG_IN_OUT_SEND_STR("\n 1, ");
				if(p_trans->present == LEVEL_MAX) {
					continue;
				}
				else {
					DBG_IN_OUT_SEND_STR("\n 2 ");
					if(clock_time_exceed_ms(
							auto_trans_par[i].auto_trans_st_t,
							s_to_ms(sw_config_st.extended_param_st[i].delay_time_s)))
					{
						DBG_IN_OUT_SEND_STR("\n 3 ");
						if(pv_handle_control_device != NULL) {
							pv_handle_control_device(i + ELE_RELAY_OFFSET, G_ON, false);
							DBG_IN_OUT_SEND_STR("\n 4");
						}
					}

					DBG_IN_OUT_SEND_STR("\n ST: ");
					DBG_IN_OUT_SEND_INT(auto_trans_par[i].auto_trans_st_t);
					DBG_IN_OUT_SEND_STR(", DELAY: ");
					DBG_IN_OUT_SEND_INT(sw_config_st.extended_param_st[i].delay_time_s);
				}
			}
			else if(sw_config_st.extended_param_st[i].toggle_type == TOGGLE_AUTO_OFF) {
				if(p_trans->present == LEVEL_OFF) {
					continue;
				}
				else {
					if(clock_time_exceed_ms(
							auto_trans_par[i].auto_trans_st_t,
							s_to_ms(sw_config_st.extended_param_st[i].delay_time_s)))
					{
						if(pv_handle_control_device != NULL) {
							pv_handle_control_device(i + ELE_RELAY_OFFSET, G_OFF, false);
						}
					}
				}
			}
		}
	}
}
/**
 * @func   auto_callback_init
 * @brief
 * @param  None
 * @retval None
 */
void auto_callback_init(type_control_dev_callback_func callbackFunc)
{
	if(callbackFunc != NULL) {
		pv_handle_control_device = callbackFunc;
	}
}
#endif
// End File
