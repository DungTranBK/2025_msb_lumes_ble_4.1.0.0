/*
 * default_network.c
 *
 *  Created on: 2021
 *  Author: Thuan Nguyen
 *
 *  Mod 3/22/222
 */
#include "proj/tl_common.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/app_provison.h"
#include "vendor/common/generic_model.h"
#include "vendor/common/mesh_node.h"
#include "vendor/common/fast_provision_model.h"
#include "vendor/mesh/user/fact/fact_handle.h"
#include "factory_reset.h"
#include "default_network.h"

#include "debug.h"
#ifdef  DEFAULT_NETWORK_DBG_EN
#define DBG_DF_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_DF_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_DF_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_DF_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
#else
#define DBG_DF_SEND_STR(x)
#define DBG_DF_SEND_INT(x)
#define DBG_DF_SEND_HEX(x)
#define DBG_DF_SEND_BYTE(x)
#endif

u8 is_sub_fast_mode=0;

u8 tmp_ivi[] = {0x00, 0x00, 0x00, 0x00};
u8 tmp_nk[]  = {0x34,0xD0,0x2F,0x83,0x0B,0x5E,0x76,0xB0,0x39,0xE1,0x35,0x21,0xDE,0x73,0x1A,0x48};
u8 tmp_ak[]  = {0x15,0xF7,0x41,0xCB,0xB7,0xA0,0x74,0xC4,0xC6,0x8E,0x1D,0xE2,0xC3,0xB6,0x5D,0xF9};
u8 tmp_net_app_id[]  = {0x01, 0x10, 0x00};          // NK: 0x001     AK: 0x001
u8 default_dev_key[] = {0x9d,0x6d,0xd0,0xe9,0x6e,0xb2,0x5d,0xc1, 0x9a,0x40,0xed,0x99,0x14,0xf8,0xf0,0x3f};

static u8 set_tmp_keys_flag = 0;
static u8 del_tmp_keys_flag = 1;

static u8 temp_key_is_in_flash = 0;

typedef struct {
	bool en;
	u32  time_len_s;
	u32  st_time_s;
}m_default_nw_t;

static m_default_nw_t m_default_nw = { .en = false };

/**
 * @func   default_network_is_present
 * @brief
 * @param  None
 * @retval None
 */
u8 default_network_is_present(void)
{
	return set_tmp_keys_flag;
}

/**
 * @func   proc_default_network
 * @brief
 * @param  None
 * @retval None
 */
void proc_default_network()
{
	// FACT mode
	if(fact_is_activate()) {
		if(get_provision_state() == STATE_DEV_UNPROV) {
			del_tmp_keys();
			return;
		}
	}
	if(!is_provision_success()) {
		set_tmp_keys(1);
	}
	else {
		u8 del_temp = 1;
		if(m_default_nw.en == true) {
			if(clock_time_exceed_s(m_default_nw.st_time_s, m_default_nw.time_len_s)) {
				m_default_nw.en = false;
			} else {
				del_temp = 0;
			}
		}
		if(del_temp == 1) {
			del_tmp_keys();
		}
	}
}
/**
 * @func    set_tmp_keys
 * @brief
 * @param
 * @retval  None
 */
void set_tmp_keys(u8 tmp)
{
	if(tmp)
	{
		if(set_tmp_keys_flag)
			return;

		DBG_DF_SEND_STR("\n SET TMP KEYS");
		// Get Index
		set_tmp_keys_flag = 1;
		del_tmp_keys_flag = 0;

		u16 nk_idx = GET_NETKEY_INDEX(tmp_net_app_id);
		u16 ak_idx = GET_APPKEY_INDEX(tmp_net_app_id);

		// Add netkey
		mesh_net_key_set(NETKEY_ADD, tmp_nk, nk_idx, 0);
		// Add appkey and bind all model
		u8 st = mesh_app_key_set(APPKEY_ADD, tmp_ak, ak_idx, nk_idx, 0);
		// Bind appkey
		if(ST_SUCCESS == st){
			appkey_bind_all(1, ak_idx, 1);
		}
	}
}

/**
 * @func    del_tmp_keys
 * @brief
 * @param
 * @retval  None
 */
void del_tmp_keys()
{
	// DELETE TMP KEYS
	if(del_tmp_keys_flag)
		return;

	DBG_DF_SEND_STR("\n DEL TMP KEYS");

	del_tmp_keys_flag = 1;
	set_tmp_keys_flag = 0;

	// Get Index
	u16 nk_idx = GET_NETKEY_INDEX(tmp_net_app_id);
	u16 ak_idx = GET_APPKEY_INDEX(tmp_net_app_id);

	// Old version
	if(temp_key_is_in_flash) {
		// Add netkey
		mesh_net_key_set(NETKEY_DEL, tmp_nk, nk_idx, 0);
		// Add appkey and bind all model
		u8 st = mesh_app_key_set(APPKEY_DEL, tmp_ak, ak_idx, nk_idx, 0);
		// Unbind appkey
		if(ST_SUCCESS == st){
			appkey_bind_all(0, ak_idx, 1);
		}
		temp_key_is_in_flash = 0;
	}
	else {
		u8 st;
		mesh_net_key_t *p_key = is_mesh_net_key_exist(nk_idx);
		if(p_key){
			if(get_net_key_cnt() <= 1){
				st = ST_CAN_NOT_REMOVE;
			} else {
				foreach_arr(i,p_key->app_key){
					if(p_key->app_key[i].valid){
						mesh_unbind_by_del_appkey(p_key->app_key[i].index);
						memset(p_key->app_key[i].key, 0, sizeof(mesh_app_key_t));
					}
				}
				memset(p_key, 0, sizeof(mesh_net_key_t));
				st = ST_SUCCESS;
			}
		}
	}
}

/**
 * @func   set_default_network_manual
 * @brief
 * @param  None
 * @retval None
 */
void set_default_network_manual(bool en, u16 time_s)
{
	if(time_s > OPEN_DF_NW_TIMEOUT) time_s = OPEN_DF_NW_TIMEOUT;
	m_default_nw.en = en&0x01;
	m_default_nw.st_time_s = clock_time_s();
	m_default_nw.time_len_s = time_s;
}

/**
 * @func   check_and_del_default_power_on
 * @brief
 * @param  None
 * @retval None
 */
void check_and_del_default_power_on(void)
{
	// Delete default network, if it's avalable
	u8 n_pos;
	if(net_key_is_valid(&n_pos, \
			GET_NETKEY_INDEX(tmp_net_app_id))) {
		DBG_DF_SEND_STR("\n Manual delete temp key power on");
		temp_key_is_in_flash = 1;
		del_tmp_keys_flag = 0;
		del_tmp_keys();
	}
}
