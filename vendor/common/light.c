/********************************************************************************************************
 * @file     light.c
 *
 * @brief    for TLSR chips
 *
 * @author	 telink
 * @date     Sep. 30, 2010
 *
 * @par      Copyright (c) 2010, Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary property of Telink
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/sig_mesh/app_mesh.h"
#include "../../proj_lib/ble/service/ble_ll_ota.h"
#include "../../vendor/common/lighting_model.h"
#include "../../vendor/common/lighting_model_HSL.h"
#include "../../vendor/common/lighting_model_xyL.h"
#include "../../vendor/common/lighting_model_LC.h"
#include "../../vendor/common/generic_model.h"
#include "../../vendor/common/scene.h"
#include "../mesh/user/factory_reset.h"
#include "../mesh/user/net_message.h"
#include "../mesh/user/led_ev.h"
#include "light.h"
#if HOMEKIT_EN
#include "../../vendor/common/led_cfg.h"
#endif
#if WIN32
#include <stdlib.h>
#else
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/pm.h"
#endif

#include "../mesh/user/debug.h"
#ifdef LIGHT_DBG_EN
#define DBG_LIGHT_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_LIGHT_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_LIGHT_SEND_HEX(x)   Dbg_sendHex(x)
#else
#define DBG_LIGHT_SEND_STR(x)
#define DBG_LIGHT_SEND_INT(x)
#define DBG_LIGHT_SEND_HEX(x)
#endif

#if MD_LEVEL_EN
type_handle_send_dev_st_callback_func  pv_handle_response_dev_state = NULL;
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
static bool power_up_flag[LIGHT_CNT] =
		{
				true,
			#if LIGHT_CNT > 1
				true,
			#endif
			#if LIGHT_CNT > 2
				true,
			#endif
			#if LIGHT_CNT > 3
				true,
			#endif
			#if LIGHT_CNT > 4
				true,
			#endif
		};

#define LIGHT_ADJUST_INTERVAL       (20)   // unit :ms;     min:20ms; max 100ms

light_res_sw_save_t light_res_sw_save[LIGHT_CNT] = {{{{0}}}};
light_res_sw_trans_t light_res_sw[LIGHT_CNT] ;

#define P_SW_LEVEL_SAVE(idx, type)	(&light_res_sw_save[idx].level[type])
#define P_ST_TRANS(idx, type)		(&light_res_sw[idx].trans[type])

u8 light_pub_trans_step = ST_PUB_TRANS_IDLE;    // 0
u8 *light_pub_model_priority = 0;

STATIC_ASSERT(LIGHTNESS_DEFAULT != 0);	// if want to set 0, please set ONOFF_DEFAULT to 0,
STATIC_ASSERT(LIGHTNESS_MIN != 0);
//STATIC_ASSERT(sizeof(light_res_sw_t) % 4 == 0); // for align
STATIC_ASSERT(LIGHT_ADJUST_INTERVAL <= 100);
static u32 tick_light_save;
publish_status_delay_t publish_status_delay[LIGHT_CNT];

u8 ct_flag = 0;

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/

/******************************************************************************/
/*                           EXPORT FUNCTIONS                                 */
/******************************************************************************/

/**
 * @func    light_par_save
 * @brief   None
 * @param
 * @retval
 */
void light_par_save(int quick)
{
	tick_light_save = (quick ? (clock_time() - BIT(31)) : clock_time()) | 1;
}
/**
 * @func    light_par_save_proc
 * @brief   None
 * @param
 * @retval
 */
void light_par_save_proc()
{
	// save proc
	if(tick_light_save && clock_time_exceed(tick_light_save, US_DELAY_TIME_TO_SAVE_LIGHT_STATE)){
		tick_light_save = 0;
		if(!is_activated_factory_test_mode()){
		    mesh_common_store(FLASH_ADR_SW_LEVEL);
		}
	}
}

#if (MD_SCENE_EN)
/**
 * @func    scene_status_change_check_all
 * @brief   None
 * @param
 * @retval
 */
void scene_status_change_check_all()
{
	foreach_arr(i,light_res_sw){
		foreach_arr(trans_type,light_res_sw[i].trans){
			st_transition_t *p_trans = P_ST_TRANS(i, trans_type);
        	scene_status_change_check(i, p_trans->present, trans_type);
	    }
	}
}
#endif

/**
 * @func    light_res_sw_g_level_set
 * @brief   None
 * @param
 * @retval  None
 */
void light_res_sw_g_level_set(int idx, s16 level, int init_time_flag, int st_trans_type)
{
	set_level_current_type(idx, st_trans_type);
	st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);
	p_trans->present = level;
	if(init_time_flag){
		p_trans->target = level;
		p_trans->remain_t_ms = 0;
		p_trans->delay_ms = 0;
	}
}
/**
 * @func    light_res_sw_g_level_target_set
 * @brief   None
 * @param
 * @retval  None
 */
void light_res_sw_g_level_target_set(int idx, s16 level, int st_trans_type)	// only for move set command
{
	//set_level_current_type(idx, st_trans_type);
	st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);
	p_trans->target = level;
	p_trans->remain_t_ms = 0;
	p_trans->delay_ms = 0;
}
/**
 * @func    light_state_refresh
 * @brief   None
 * @param
 * @retval  None
 */
void light_set_hardware(u8 idx, u8 val)
{
	/*
	if(idx >= LIGHT_CNT) return;
	if(val > 0){
    	send_control_dev_manual_to_mcu(idx, G_ON, TRUE);
    }else{
    	send_control_dev_manual_to_mcu(idx, G_OFF, TRUE);
    }
	*/
}

/**
 * @func    light_state_refresh
 * @brief   None
 * @param
 * @retval  None
 */
void light_state_refresh(int idx) // idx: index of LIGHT_CNT.
{
	st_transition_t *p_trans = P_ST_TRANS(idx, ST_TRANS_LIGHTNESS);
	u8 lum_100 = level2lum(p_trans->present);
	CB_NL_PAR_NUM_3(p_nl_level_state_changed,idx * ELE_CNT_EVERY_LIGHT + ST_TRANS_LIGHTNESS, p_trans->present, p_trans->target);
	light_set_hardware(idx, lum_100);
}

/**
 * @func    get_light_pub_list
 * @brief   None
 * @param
 * @retval  None
 */
void get_light_pub_list(int st_trans_type, s16 present_level, s16 target_level, int pub_trans_flag, st_pub_list_t *pub_list)
{
    if(pub_trans_flag){
        pub_list->st[st_trans_type] = ST_G_LEVEL_SET_PUB_TRANS;
    }else{
        pub_list->st[st_trans_type] = (target_level != present_level) ? ST_G_LEVEL_SET_PUB_NOW : ST_G_LEVEL_SET_PUB_NONE;
    }
    if(ST_TRANS_LIGHTNESS == st_trans_type){
        if(pub_list->st[ST_TRANS_LIGHTNESS] && ((LEVEL_OFF == present_level) || (LEVEL_OFF == target_level))){
            pub_list->st[ST_TRANS_PUB_ONOFF] = pub_list->st[ST_TRANS_LIGHTNESS];
        }
    }
}

/**
 * @func    light_g_level_set
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_set(int idx, s16 level, int init_time_flag, int st_trans_type, st_pub_list_t *pub_list)
{
	st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);

	if(pub_list){
		get_light_pub_list(st_trans_type, p_trans->present, level, 0, pub_list);
	}
	light_res_sw_g_level_set(idx, level, init_time_flag, st_trans_type);
	light_state_refresh(idx);
    return 0;
}

//---------------------------------- GET FUNCTIONS

/**
 * @func    light_remain_time_get
 * @brief   None
 * @param
 * @retval
 */
u8 light_remain_time_get(st_transition_t *p_trans)
{
	u32 remain_ms = p_trans->remain_t_ms;
	u32 delay_ms = p_trans->delay_ms;

	u8 remain_t = get_transition_step_res(remain_ms/100);
	if(0 == remain_t){
		remain_t = get_transition_step_res((delay_ms+99)/100);
	}
	return remain_t;
}
/**
 * @func    light_g_level_get
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_get(u8 *rsp, int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		set_level_current_type(idx, st_trans_type);
		mesh_cmd_g_level_st_t *p_st = (mesh_cmd_g_level_st_t *)rsp;
		st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);
		p_st->present_level = p_trans->present;
		p_st->target_level = p_trans->target;
		p_st->remain_t = light_remain_time_get(p_trans);
		return 0;
	}
	return -1;
}

/**
 * @func    light_g_level_def_get
 * @brief   None
 * @param
 * @retval
 */
s16 light_g_level_def_get(int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		sw_level_save_t *p_save = P_SW_LEVEL_SAVE(idx, st_trans_type);
		return p_save->def;
	}
	return 0;
}

/**
 * @func    light_g_level_def_get_u16
 * @brief   None
 * @param
 * @retval
 */
u16 light_g_level_def_get_u16(int idx, int st_trans_type)
{
	return s16_to_u16(light_g_level_def_get(idx, st_trans_type));
}

/**
 * @func    light_g_level_def_set
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_def_set(s16 val, int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		//set_level_current_type(idx, st_trans_type);
		sw_level_save_t *p_save = P_SW_LEVEL_SAVE(idx, st_trans_type);
		p_save->def = val;
		light_par_save(1);
		return 0;
	}
	return -1;
}

/**
 * @func    light_g_level_def_set_u16
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_def_set_u16(u16 val, int idx, int st_trans_type)
{
	return light_g_level_def_set(u16_to_s16(val), idx, st_trans_type);
}

/**
 * @func    light_g_level_range_get
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_range_get(light_range_s16_t *p_range, int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		//set_level_current_type(idx, st_trans_type);
		sw_level_save_t *p_save = P_SW_LEVEL_SAVE(idx, st_trans_type);
		p_range->min = p_save->min;
		p_range->max = p_save->max;
		return 0;
	}else{
		memset(p_range, 0, sizeof(light_range_s16_t));
		return -1;
	}
}

/**
 * @func    light_g_level_range_get_u16
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_range_get_u16(light_range_u16_t *p_range, int idx, int st_trans_type)
{
	light_range_s16_t range_s16 = {0};
	int err = light_g_level_range_get(&range_s16, idx, st_trans_type);
	p_range->min = get_lightness_from_level(range_s16.min);
	p_range->max = get_lightness_from_level(range_s16.max);
	return err;
}

/**
 * @func    light_g_level_range_set
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_range_set(u16 min, u16 max, int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		//set_level_current_type(idx, st_trans_type);
		sw_level_save_t *p_save = P_SW_LEVEL_SAVE(idx, st_trans_type);
		p_save->min = get_level_from_lightness(min);
		p_save->max = get_level_from_lightness(max);
		light_par_save(1);
		return 0;
	}
	return -1;
}

/**
 * @func    light_g_level_target_get
 * @brief   None
 * @param
 * @retval
 */
s16 light_g_level_target_get(int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		set_level_current_type(idx, st_trans_type);
		st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);
		return p_trans->target;
	}
	return 0;
}

/**
 * @func    light_g_level_present_get
 * @brief   None
 * @param
 * @retval
 */
s16 light_g_level_present_get(int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		set_level_current_type(idx, st_trans_type);
		st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);
		return p_trans->present;
	}
	return 0;
}

/**
 * @func    light_g_level_present_get_u16
 * @brief   None
 * @param
 * @retval
 */
u16 light_g_level_present_get_u16(int idx, int st_trans_type)
{
	return s16_to_u16(light_g_level_present_get(idx, st_trans_type));
}

/**
 * @func    light_g_onoff_present_get
 * @brief   None
 * @param
 * @retval
 */
u8 light_g_onoff_present_get(int idx)
{
    return (light_g_level_present_get(idx, ST_TRANS_LIGHTNESS) != LEVEL_OFF);
}
/**
 * @func    light_g_level_last_get
 * @brief   None
 * @param
 * @retval
 */
s16 light_g_level_last_get(int idx, int st_trans_type)
{
	if(idx < LIGHT_CNT){
		//set_level_current_type(idx, st_trans_type);
		sw_level_save_t *p_save = P_SW_LEVEL_SAVE(idx, st_trans_type);
		return p_save->last;
	}
	return 0;
}

/**
 * @func    get_light_g_level_by_onoff
 * @brief   None
 * @param
 * @retval
 */
s16 get_light_g_level_by_onoff(int idx, int on, int st_trans_type, int force_last)
{
	if(on){
	    s16 last = light_g_level_last_get(idx, st_trans_type);
		if(ST_TRANS_LIGHTNESS == st_trans_type){
            s16 def = light_g_level_def_get(idx, st_trans_type);
		    return ((force_last || (LEVEL_OFF == def)) ? last : def); // refer to Lightness LLN-BV11-C
		}else{
			return last;
		}
	}else{
		return LEVEL_OFF;
	}
}

/**
 * @func    lum2level
 * @brief   None
 * @param
 * @retval
 */
s16 lum2level(u8 lum)
{
	if(lum > 100){
		lum  = 100;
	}
	return (-32768 + division_round(65535*lum,100));
}

/**
 * @func    level2lum
 * @brief   None
 * @param
 * @retval
 */
u8 level2lum(s16 level)
{
	u16 lightness = level + 32768;
	u32 fix_1p2 = 0;
	if(lightness){	// fix decimals
		#define LEVEL_UNIT_1P2	(65535/100/2)
		if(lightness < LEVEL_UNIT_1P2 + 2){     // +2 to fix accuracy missing
			lightness = LEVEL_UNIT_1P2 * 2;		// make sure lum is not zero when light on.
		}
		fix_1p2 = LEVEL_UNIT_1P2;
	}
	return (((lightness + fix_1p2)*100)/65535);
}

/**
 * @func    lum2_lightness
 * @brief   None
 * @param
 * @retval
 */
u16 lum2_lightness(u8 lum)
{
	return (get_lightness_from_level(lum2level(lum)));
}

/**
 * @func    lightness2_lum
 * @brief   None
 * @param
 * @retval
 */
u8 lightness2_lum(u16 lightness)
{
	return (level2lum(get_level_from_lightness(lightness)));
}

/**
 * @func    temp100_to_temp
 * @brief   None
 * @param
 * @retval
 */
u16 temp100_to_temp(u8 temp100)
{
	if(temp100 > 100){
		temp100  = 100;
	}
	return (CTL_TEMP_MIN + ((CTL_TEMP_MAX - CTL_TEMP_MIN)*temp100)/100);
}

/**
 * @func    temp_to_temp100_hw
 * @brief   None
 * @param
 * @retval
 */
u8 temp_to_temp100_hw(u16 temp) // use for driver pwm, 0--100 is absolute value, not related to temp range
{
	if(temp < CTL_TEMP_MIN){
		temp = CTL_TEMP_MIN;
	}
	
	if(temp > CTL_TEMP_MAX){
		temp = CTL_TEMP_MAX;
	}
	u32 fix_1p2 = (CTL_TEMP_MAX - CTL_TEMP_MIN)/100/2;	// fix decimals
	return (((temp - CTL_TEMP_MIN + fix_1p2)*100)/(CTL_TEMP_MAX - CTL_TEMP_MIN));   // temp100 can be zero.
}

/**
 * @func    temp_to_temp100
 * @brief   None
 * @param
 * @retval
 */
u8 temp_to_temp100(u16 temp)
{
	return temp_to_temp100_hw(temp);   // comfirm later, related with temp range
}

/**
 * @func    light_lum_get
 * @brief   None
 * @param
 * @retval
 */
u8 light_lum_get(int idx, int target_flag)
{
	st_transition_t *p_trans = P_ST_TRANS(idx, ST_TRANS_LIGHTNESS);
    return level2lum(target_flag? p_trans->target : p_trans->present);
}

//---------------------------------- SET FUNCTIONS

/**
 * @func    light_onoff_idx
 * @brief   None
 * @param
 * @retval
 */
int light_onoff_idx(int idx, int on, int init_time_flag){
    if(idx < LIGHT_CNT){
    	int st_trans_type = ST_TRANS_LIGHTNESS;
    	st_pub_list_t pub_list = {{0}};
    	light_g_level_set(idx, get_light_g_level_by_onoff(idx, on, st_trans_type, 1), init_time_flag, st_trans_type, &pub_list);
    }
    return 0;
}

/**
 * @func    light_g_level_set_idx
 * @brief   None
 * @param
 * @retval
 */
int light_g_level_set_idx(int idx, s16 level, int init_time_flag, int st_trans_type, st_pub_list_t *pub_list)
{
    if(idx < LIGHT_CNT){
        light_g_level_set(idx, level, init_time_flag, st_trans_type, pub_list);
    }
    return 0;
}

#if MD_SERVER_EN
/**
 * @func    light_g_level_set_idx_with_trans
 * @brief   None
 * @param
 * @retval
 */
void light_g_level_set_idx_with_trans(u8 *set_trans, int idx, int st_trans_type)
{
    if(idx < LIGHT_CNT){
		set_level_current_type(idx, st_trans_type);
		st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);
		mesh_set_trans_t *p_set = (mesh_set_trans_t *)set_trans;
		p_trans->target = p_set->target_val;
		if(0x3F == (p_set->transit_t & 0x3F)){
			p_trans->remain_t_ms = -1;
		}else{
			p_trans->remain_t_ms = 100 * get_transition_100ms((trans_time_t *)&p_set->transit_t);
			if(p_trans->remain_t_ms){
                if(is_level_move_set_op(p_set->op)){
                    // have been make sure (target_val != present_val) and (level_move != 0) before.
				    p_trans->step_1p32768 = ((p_set->level_move * 32768) /(s32)(p_trans->remain_t_ms)) * LIGHT_ADJUST_INTERVAL;
                    u32 abs_step = abs(p_set->level_move);
                    u32 abs_delta = (p_set->target_val - p_set->present_val);
                    u32 mod = abs_delta % abs_step;
                    u32 remain_t_ms_org = p_trans->remain_t_ms;
                    u32 val;
                    if(remain_t_ms_org >= 65536){
                        // remain_t_ms_org is less than 37800000, so (remain_t_ms_org * 100) is less than 0xffffffff
                        val = (((remain_t_ms_org * 100)/abs_step)*(mod))/100;
                    }else{
                        // make sure not overflow
                        val = (remain_t_ms_org * mod) / abs_step;
                    }
                    p_trans->remain_t_ms = remain_t_ms_org * (abs_delta / abs_step) + val;
                }else{
				    p_trans->step_1p32768 = (((p_trans->target - p_trans->present) * 32768) /(s32)(p_trans->remain_t_ms)) * LIGHT_ADJUST_INTERVAL;
				}
			}else{
				p_trans->step_1p32768 = 0;
			}
			p_trans->present_1p32768 = 0; // init
		}
		p_trans->delay_ms = p_set->delay * 5;
    }
}
#endif

/**
 * @func    light_onoff_all
 * @brief   None
 * @param
 * @retval
 */
void light_onoff_all(u8 on){
    foreach(i, LIGHT_CNT){
        light_onoff_idx(i, on, 1);
    }
}

/**
 * @func    set_light_linear_flag
 * @brief   None
 * @param
 * @retval
 */
int set_light_linear_flag(int idx,u16 linear)
{
	if(idx < LIGHT_CNT){
	    light_res_sw[idx].linear_set_flag = 1;
	    light_res_sw[idx].linear = linear;
	}
	return 0;
}

/**
 * @func    clear_light_linear_flag
 * @brief   None
 * @param
 * @retval
 */
int clear_light_linear_flag(int idx)
{
	if(idx < LIGHT_CNT){
	    light_res_sw[idx].linear_set_flag = 0;
	    light_res_sw[idx].linear = 0;
	}
	return 0;
}

/**
 * @func    get_light_linear_val
 * @brief   None
 * @param
 * @retval
 */
u16 get_light_linear_val(int idx)
{
    return light_res_sw[idx].linear;
}

/**
 * @func    is_linear_flag
 * @brief   None
 * @param
 * @retval
 */
int is_linear_flag(int idx)
{
	if(idx < LIGHT_CNT){
	    return light_res_sw[idx].linear_set_flag;
	}
	return 0;
}

/**
 * @func    app_led_en
 * @brief   None
 * @param
 * @retval  None
 */
void app_led_en (int id, int en)
{
    if(id < LIGHT_CNT){
        light_onoff_idx(id, en, 1);
    }
}

#if MD_SERVER_EN
/**
 * @func   light_handle_publish_status_delay
 * @brief  None
 * @param  None
 * @retval None
 */
void light_handle_publish_status_delay(void)
{
	foreach(i, LIGHT_CNT) {
		if(publish_status_delay[i].flag == true){
			if(clock_time_exceed_ms(publish_status_delay[i].delay_start_time,
							  publish_status_delay[i].delay_time)) {
				publish_status_delay[i].flag = false;
            #if (DEV_TYPE_SEL == G_TYPE_SWITCH_BUTTON) || (DEV_TYPE_SEL == G_TYPE_LM_GATE)
				mesh_tx_cmd_g_onoff_st(
						i, ele_adr_primary + i, GATEWAY_UNICAST_ADDR, 0, 0, G_ONOFF_STATUS
					);
            #else
				#if MD_LEVEL_EN
				if(pv_handle_response_dev_state != NULL) {
					pv_handle_response_dev_state(i);
				}
				#endif
            #endif
			}
		}
    }
}

/**
 * @func    light_publish_status_delay
 * @brief   None
 * @param
 * @retval  None
 */
void light_publish_status_delay(u8 idx, u32 delay_time)
{
	if(idx < LIGHT_CNT) {
		publish_status_delay[idx].flag = true;
		if(power_up_flag[idx] == 1){
			publish_status_delay[idx].delay_time = 5000 + ((rand()% 20000));
			power_up_flag[idx] = 0;
		}
		else {
		    publish_status_delay[idx].delay_time = delay_time + PUBLISH_DELAY_OFFSET;
		}
		publish_status_delay[idx].delay_start_time = clock_time_ms();
	}
}
#endif

/**
 * @func    rf_link_light_event_callback
 * @brief   None
 * @param
 * @retval  None
 */
void rf_link_light_event_callback (u8 status, u16 mask)
{
	if(status == LGT_CMD_SUC_ADD_APPKEY){
		send_led_evt_to_mcu(
				LED_SUC_ADD_APPKEY, 0xFFFF
			);
    }
}

/**
 * @func    show_ota_result
 * @brief   None
 * @param
 * @retval
 */
void show_ota_result(int result)
{
	DBG_LIGHT_SEND_STR("\n show_ota_result");

	if(result == OTA_SUCCESS){
		send_led_evt_to_mcu(
					LED_OTA_SUCESS, 0xFFFF
				);
	}
	else {
		send_led_evt_to_mcu(
				    LED_OTA_FAIL, 0xFFFF
				);
	}
}

#if MD_LEVEL_EN
/**
 * @func    light_callback_init
 * @brief
 * @param
 * @retval  None
 */
void light_callback_init(type_handle_send_dev_st_callback_func callback_func)
{
	if(callback_func != NULL) {
		pv_handle_response_dev_state = callback_func;
	}
}
#endif
