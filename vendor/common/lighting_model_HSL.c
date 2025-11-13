/********************************************************************************************************
 * @file     lighting_model_HSL.c
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
#include "../../proj/tl_common.h"
#ifndef WIN32
#include "../../proj/mcu/watchdog_i.h"
#endif 
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../vendor/common/user_config.h"
#include "app_health.h"
#include "../../proj_lib/sig_mesh/app_mesh.h"
#include "lighting_model_HSL.h"
#include "lighting_model.h"

#include "../mesh/user/net_message.h"
#include "../mesh/user/debug.h"
#ifdef MODEL_HSL_DBG_EN
#define DBG_MODEL_HSL_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_MODEL_HSL_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_MODEL_HSL_SEND_HEX(x)   Dbg_sendHex(x)
#else
#define DBG_MODEL_HSL_SEND_STR(x)
#define DBG_MODEL_HSL_SEND_INT(x)
#define DBG_MODEL_HSL_SEND_HEX(x)
#endif

#if (LIGHT_TYPE_HSL_EN)

model_light_hsl_t       model_sig_light_hsl;
u32 mesh_md_light_hsl_addr = FLASH_ADR_MD_LIGHT_HSL;

int mesh_cmd_sig_light_hsl_set(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	func_handle_control_message(
				par, par_len, cb_par, LIGHT_HSL_SET
			);
	return 0;
}

int mesh_light_hsl_st_publish(u8 idx)
{
	return 0;
}

int mesh_light_hue_st_publish(u8 idx)
{
	return 0;
}

int mesh_light_sat_st_publish(u8 idx)
{
	return 0;
}

#endif

