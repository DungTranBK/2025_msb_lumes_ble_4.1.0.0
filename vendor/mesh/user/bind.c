/*
 * bind.c
 *
 *  Created on: Apr 15, 2022
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "vendor/common/light.h"
#include "vendor/common/system_time.h"
#include "vendor/common/mesh_config.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/mesh_node.h"
#include "default_network.h"
#include "bind.h"

#include "debug.h"
#ifdef BIND_DBG_EN
#define DBG_BIND_SEND_STR(x)     Dbg_sendString((s8*)x)
#define DBG_BIND_SEND_INT(x)     Dbg_sendInt(x)
#define DBG_BIND_SEND_HEX(x)     Dbg_sendHex(x)
#define DBG_BIND_SEND_HEX_ONE(x) Dbg_sendHexOneByte(x);
#else
#define DBG_BIND_SEND_STR(x)
#define DBG_BIND_SEND_INT(x)
#define DBG_BIND_SEND_HEX(x)
#define DBG_BIND_SEND_HEX_ONE(x)
#endif

#if BIND_APPKEY_PERIODIC
/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

typedef struct {
	u32 start_t_s;
	u16 period_s;
	u8  cnt;
	u8  is_init;
}bind_periodic_t;

static bind_periodic_t  bind_periodic = { .is_init = 0 };

#define MAIN_APPKEY_IDX    0

#define AUTO_BIND_TIME_LEN    	(20*60)   // 20 mins
#define AUTO_BIND_RAND_TIME   	(10*60)   // rand: 10 mins

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/

/******************************************************************************/
/*                           EXPORT FUNCTIONS                                 */
/******************************************************************************/

/**
 * @func    bind_print_info
 * @brief
 * @param   None
 * @retval  None
 */
static void bind_print_info(void)
{
#ifdef BIND_DBG_EN
	DBG_BIND_SEND_STR("\n BIND_KEY_INFO:");
	foreach(i, LIGHT_CNT) {
		foreach(j, BIND_KEY_MAX) {
			DBG_BIND_SEND_INT(model_sig_light_ctl.srv[i].com.bind_key[j].idx);
			DBG_BIND_SEND_STR(" ");
			DBG_BIND_SEND_INT(model_sig_light_ctl.srv[i].com.bind_key[j].bind_ok);
			if(j < (BIND_KEY_MAX - 1)) {
				DBG_BIND_SEND_STR(", ");
			}
		}
	}
	DBG_BIND_SEND_STR("\n OTA: ");
	foreach(i, BIND_KEY_MAX) {
		DBG_BIND_SEND_INT(model_mesh_ota.fw_update_srv.com.bind_key[i].idx);
		DBG_BIND_SEND_STR(" ");
		DBG_BIND_SEND_INT(model_mesh_ota.fw_update_srv.com.bind_key[i].bind_ok);
		if(i < (BIND_KEY_MAX - 1)) {
			DBG_BIND_SEND_STR(", ");
		}
	}
#endif
}

/**
 * @func    grp_check_error_group
 * @brief
 * @param   None
 * @retval  None
 */
void bind_periodic_init(void)
{
	bind_periodic.start_t_s = 0;
	if(is_provision_success()) {
		bind_periodic.period_s  = 10 + rand()%10;
	}
	else {
		bind_periodic.period_s  =   \
				AUTO_BIND_TIME_LEN + rand()%AUTO_BIND_RAND_TIME;
	}
	bind_periodic.cnt = 0;
	bind_periodic.is_init = 1;

#ifdef BIND_DBG_EN
	DBG_BIND_SEND_STR("\n A_BIND POWER ON");
	bind_print_info();
#endif

}

/**
 * @func    grp_check_error_group
 * @brief
 * @param   None
 * @retval  None
 */
void bind_periodic_handle(void)
{
	if((!is_provision_success())  \
				|| (!bind_periodic.is_init)  \
						|| default_network_is_present()) {
#ifdef BIND_DBG_EN
		static u8 loop_cnt= 0;
		if(loop_cnt++ >= 10) {
			DBG_BIND_SEND_STR("\n CAN'T bind_periodic_handle");
			loop_cnt = 0;
		}
#endif
		if(!is_provision_success()) {
			bind_periodic.start_t_s = clock_time_s();
		}
		return;
	}
	if(clock_time_exceed_s(   \
			bind_periodic.start_t_s, bind_periodic.period_s)) {

		DBG_BIND_SEND_STR("\n AUTO_BIND");
		bind_print_info();

		/*
		 * MAIN Network
		 */
		u8 auto_bind_en = 0, bind_success;
		// OTA
		bind_success = 0;
		foreach(i, BIND_KEY_MAX) {
			if(model_sig_light_ctl.srv[0].com.bind_key[i].idx == MAIN_APPKEY_IDX) {
				if(model_sig_light_ctl.srv[0].com.bind_key[i].bind_ok == BIND_OK) {
					bind_success = 1;
					DBG_BIND_SEND_STR("\n BIND_CTL");
					break;
				}
			}
		}
		if(!bind_success) {
			auto_bind_en = 1;
		}
		// LIGHT Control
		if(!auto_bind_en) {
			foreach(i, LIGHT_CNT) {
				foreach(j, BIND_KEY_MAX) {
                    // Clear
					bind_success = 0;
					// CTL, HSL, ON_OFF, LIGHTNESS, VENDOR
					#if LIGHT_TYPE_CT_EN
					if(model_sig_light_ctl.srv[i].com.bind_key[j].idx == MAIN_APPKEY_IDX) {
						if(model_sig_light_ctl.srv[i].com.bind_key[j].bind_ok == BIND_OK) {
							bind_success = 1;
							DBG_BIND_SEND_STR("\n BIND_CTL");
							break;
						}
					}
					#endif
					#if LIGHT_TYPE_HSL_EN
					if(model_sig_light_hsl.srv[i].com.bind_key[j].idx == MAIN_APPKEY_IDX) {
						if(model_sig_light_hsl.srv[i].com.bind_key[j].bind_ok == BIND_OK) {
							bind_success = 1;
							DBG_BIND_SEND_STR("\n BIND_HSL");
							break;
						}
					}
					#endif
					#if MD_LIGHTNESS_EN
					if(model_sig_lightness.srv[i].com.bind_key[j].idx == MAIN_APPKEY_IDX) {
						if(model_sig_lightness.srv[i].com.bind_key[j].bind_ok == BIND_OK) {
							bind_success = 1;
							DBG_BIND_SEND_STR("\n BIND_LIGHTNESS");
							break;
						}
					}
                    #endif
					// ON_OFF
					if(model_sig_g_onoff_level.onoff_srv[i].com.bind_key[j].idx == MAIN_APPKEY_IDX) {
						if(model_sig_g_onoff_level.onoff_srv[i].com.bind_key[j].bind_ok == BIND_OK) {
							bind_success = 1;
							DBG_BIND_SEND_STR("\n BIND_ON_OFF");
							break;
						}
					}
					// Vendor
					if(model_vd_light.srv[i].com.bind_key[j].idx == MAIN_APPKEY_IDX) {
						if(model_vd_light.srv[i].com.bind_key[j].bind_ok == BIND_OK) {
							bind_success = 1;
							DBG_BIND_SEND_STR("\n BIND_VENDOR");
							break;
						}
					}
				}
				if(!bind_success) {
					auto_bind_en = 1;
					DBG_BIND_SEND_STR("\n BIND_CTL");
					break;
				}
			}
		}
		if(auto_bind_en) {
			appkey_bind_all(1, MAIN_APPKEY_IDX, 1);
			bind_periodic.cnt++;
			DBG_BIND_SEND_STR("\n AUTO_BIND_MODEL: ");
			DBG_BIND_SEND_INT(bind_periodic.cnt);
		}
#ifdef BIND_DBG_EN
		else {
			DBG_BIND_SEND_STR("\n - DON'T NEED AUTO BIND");
		}
#endif
		bind_periodic.period_s = \
				AUTO_BIND_TIME_LEN + rand()%AUTO_BIND_RAND_TIME;
		bind_periodic.start_t_s = clock_time_s();
	}
}
#endif
// End bind
