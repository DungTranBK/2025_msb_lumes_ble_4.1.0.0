/********************************************************************************************************
 * @file	fast_provision_model.c
 *
 * @brief	for TLSR chips
 *
 * @author	telink
 * @date	Sep. 30, 2010
 *
 * @par     Copyright (c) 2017, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#if !WIN32
#include "proj/mcu/watchdog_i.h"
#endif 
#include "proj_lib/ble/ll/ll.h"
#include "proj_lib/ble/blt_config.h"
#include "vendor/common/user_config.h"
#include "app_health.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "smart_provision.h"
#include "generic_model.h"
#include "version.h"

#include "vendor/mesh/user/default_network.h"
#include "vendor/mesh/user/sw_config.h"
#include "vendor/mesh/user/pre_update.h"

#include "fast_provision_model.h"

#include "../mesh/user/debug.h"
#ifdef  FAST_PROVISION_DBG_EN
#define DBG_FAST_PROVISION_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_FAST_PROVISION_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_FAST_PROVISION_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_FAST_PROVISION_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
#else
#define DBG_FAST_PROVISION_SEND_STR(x)
#define DBG_FAST_PROVISION_SEND_INT(x)
#define DBG_FAST_PROVISION_SEND_HEX(x)
#define DBG_FAST_PROVISION_SEND_BYTE(x)
#endif

#define FAST_PROVISION_TIMEOUT 		120*1000*1000
#if FAST_PROVISION_ENABLE
//----------------------fast provision proc---------------------------
fast_prov_par_t fast_prov;

mesh_cb_fun_par_t cb_para;
u32 get_mac_time = 0, delay_time = 0;
const u8 mac_header[] = {  \
		0x04, 0x95, 0xF1, 0x48 , 0x15 , 0xC7 , 0x53 , 0x7B , 0xC9 , 0x0F  \
	};
static u8 en_fast_revert_network = 0;
int cb_vd_mesh_get_addr_st_rsp(mesh_cb_fun_par_t *cb_par);
#endif

void mesh_gatt_adv_beacon_enable(u8 enable){
	#if !WIN32
	beacon_send.en = enable;
	gatt_adv_send_flag = enable;
	#endif
}

void mesh_revert_network()
{
	node_need_store_misc = 1;
	if(provision_mag.gatt_mode == GATT_PROVISION_MODE){
		#if FAST_PROVISION_ENABLE
		if((!fast_prov.not_need_prov)&&(mesh_fast_prov_sts_get() == FAST_PROV_COMPLETE)){
			mesh_provision_par_set(&fast_prov.net_info.pro_data);
			//set app_key
			mesh_set_start_wait_add_appkey_msg_after_set_node_info(false);
			mesh_appkey_set_t *p_set = (mesh_appkey_set_t *)&fast_prov.net_info.appkey_set;
			u8 err = mesh_app_key_set(APPKEY_ADD, p_set->appkey, GET_APPKEY_INDEX(p_set->net_app_idx), GET_NETKEY_INDEX(p_set->net_app_idx), 1);

            if(err == 0){
            	u16 appkey_idx = 0;
                appkey_idx = GET_APPKEY_INDEX(p_set->net_app_idx);
                appkey_bind_all(MODE_APP_BIND, appkey_idx, 1);
            }
            if(get_all_appkey_cnt() == 1){
				u16 app_key_idx = GET_NETKEY_INDEX(p_set->net_app_idx);
				ev_handle_traversal_cps(EV_TRAVERSAL_BIND_APPKEY, (u8 *)&app_key_idx);
			}
            del_tmp_keys();
            DBG_FAST_PROVISION_SEND_STR("\n Blink");
        	// BLINK LED
        	static u32 blink_led_add_appkey_t = 0;
        	if((blink_led_add_appkey_t == 0)
        				|| (clock_time_exceed_ms(blink_led_add_appkey_t, 30000))){
        		rf_link_light_event_callback(LGT_CMD_SUC_ADD_APPKEY, DEVICE_MASK);
        		blink_led_add_appkey_t = clock_time_ms();
        	}
        	DBG_FAST_PROVISION_SEND_STR("\n Publish");
            // PUBLISH
        	foreach(i, NUMBER_RL)
        	{
        		nwk_control_msg_para[i].dst = ADR_ALL_NODES;
        		send_get_device_status_manual(i + ELE_RELAY_OFFSET);
        	}
        	sw_setup_enable_send_config_delay(true, true);
		}
		else
		#endif
		{
			memset(mesh_key.net_key, 0, sizeof(mesh_key_t)-OFFSETOF(mesh_key_t,net_key));
	        u8 node_ident_random[8];    // because it will be used in both mesh_flash_retrieve_()->mesh_net_key_set_() and mesh_provision_para_init_()
	        prov_random_proc(node_ident_random);
			mesh_flash_retrieve();
			mesh_provision_para_init(node_ident_random);
			del_tmp_keys();   // need to call before retrieve default keys
			set_tmp_keys(1);
			DBG_FAST_PROVISION_SEND_STR("\n REVERT PROV NOT COMPLETE");
		}

		#if WIN32
		App_key_bind_end_callback(MESH_APP_KEY_BIND_EVENT_SUC);
		#endif
		mesh_gatt_adv_beacon_enable(1);
		mesh_node_init();

		#if FAST_PROVISION_ENABLE
		mesh_fast_prov_val_init();
		#endif
	}
	else {
		del_tmp_keys();
	}
}

u8 mesh_fast_prov_get_ele_cnt_callback(u16 pid)
{
	u8 node_ele_cnt = 1;
	switch(pid){
	case LIGHT_TYPE_CT:
		node_ele_cnt = 1;
		break;
	case LIGHT_TYPE_HSL:
		node_ele_cnt = 3;
		break;
	case LIGHT_TYPE_XYL:
		node_ele_cnt = 3;
		break;
	case LIGHT_TYPE_CT_HSL:
		node_ele_cnt = 4;
		break;
	case LIGHT_TYPE_PANEL:
		break;
	default:
		break;
	}
	return node_ele_cnt;
}

#if FAST_PROVISION_ENABLE
void mesh_device_key_set_default()
{
	memcpy(mesh_key.dev_key, tbl_mac, sizeof(tbl_mac));
	memcpy(mesh_key.dev_key + sizeof(tbl_mac), mac_header, sizeof(mac_header));
}

void mesh_fast_prov_disable(void)
{
	if(is_provision_success()){
		fast_prov.not_need_prov = 1;
		DBG_FAST_PROVISION_SEND_STR("\n ___FAST PROV DISABLE___\n");
	}
}

void mesh_fast_prov_val_init()
{
	memset(&fast_prov, 0x00, sizeof(fast_prov));
	if(is_provision_success()){
		fast_prov.not_need_prov = 1;
	}
	else{
		mesh_device_key_set_default();
		fast_prov.get_mac_en = 1;
	}
}

int mesh_fast_prov_sts_set(u8 sts_set)
{
	if(FAST_PROV_IDLE == sts_set){
		fast_prov.start_tick = 0;
	}
	else{
		fast_prov.start_tick = clock_time()|1;
	}

	if(sts_set != fast_prov.cur_sts){
		fast_prov.last_sts = fast_prov.cur_sts;
		fast_prov.cur_sts = sts_set;
	}
	return 1;
}

int mesh_fast_prov_sts_get()
{
	return fast_prov.cur_sts;
}

int mesh_fast_prov_rcv_op(u16 rcv_op)
{
	fast_prov.rcv_op = rcv_op;
	return 0;
}

void mesh_fast_provision_timeout()
{
	if(fast_prov.start_tick &&
			clock_time_exceed(fast_prov.start_tick,FAST_PROVISION_TIMEOUT)){
		LOG_MSG_INFO(TL_LOG_NODE_BASIC, 0, 0,"FAST_PROV_TIME_OUT",0);
		mesh_fast_prov_sts_set(FAST_PROV_TIME_OUT);
		LOG_MSG_LIB(TL_LOG_NODE_SDK, 0, 0,"time out",0);
		fast_prov.start_tick = 0;
		DBG_FAST_PROVISION_SEND_STR("\n...........PROVISION TIMEOUT........... ");
	}
}

void mesh_fast_prov_proc()
{
	if(is_provision_success()){
		return ;
	}
	if(is_busy_tx_segment_or_reliable_flow()){
		return ;
	}
	mesh_fast_provision_timeout();
	if(((mesh_fast_prov_sts_get() == FAST_PROV_COMPLETE)
			&& clock_time_exceed(fast_prov.start_tick, fast_prov.delay*1000))
				||(mesh_fast_prov_sts_get() == FAST_PROV_TIME_OUT))
	{
		fast_prov.delay = 0;
		mesh_adv_txrx_to_self_en(0);
		mesh_revert_network();	
		LOG_MSG_INFO(TL_LOG_COMMON, 0, 0,"FAST_PROV_REVERT_NETWORK",0);
		mesh_fast_prov_sts_set(FAST_PROV_IDLE);
		fast_prov.pending = 0;
		DBG_FAST_PROVISION_SEND_STR("\n ...FAST REVERT NETWORK");
	}

	static u32 fast_prov_st_time = 0;
	if(clock_time_exceed_ms(fast_prov_st_time, 1000))
	{
		DBG_FAST_PROVISION_SEND_STR("\n FAST_PROV in progress");
		fast_prov_st_time = clock_time_ms();
	}

	switch(fast_prov.rcv_op)
	{
		case VD_MESH_ADDR_GET:
			if(fast_prov.cur_sts == FAST_PROV_IDLE){
				mesh_adv_txrx_to_self_en(1);
				mesh_fast_prov_sts_set(FAST_PROV_GET_ADDR);
				mesh_gatt_adv_beacon_enable(0);
				LOG_MSG_LIB(TL_LOG_NODE_SDK, 0, 0,"VD_MESH_ADDR_GET",0);
				DBG_FAST_PROVISION_SEND_STR("\n VD_MESH_ADDR_GET");
			}
			if(get_mac_time && clock_time_exceed_ms(get_mac_time, delay_time)){
				get_mac_time = 0;
				cb_vd_mesh_get_addr_st_rsp(&cb_para);
			}
			break;
		case VD_MESH_ADDR_SET:
			if(fast_prov.cur_sts == FAST_PROV_GET_ADDR){
				cache_init(ADR_ALL_NODES);
				mesh_fast_prov_sts_set(FAST_PROV_SET_ADDR);
				LOG_MSG_LIB(TL_LOG_NODE_SDK, 0, 0,"VD_MESH_ADDR_SET",0);
				DBG_FAST_PROVISION_SEND_STR("\n ***VD_MESH_ADDR_SET");
			}
			break;
		case VD_MESH_PROV_DATA_SET:
			if(fast_prov.cur_sts == FAST_PROV_SET_ADDR){				
				mesh_fast_prov_sts_set(FAST_PROV_NET_INFO);
				LOG_MSG_LIB(TL_LOG_NODE_SDK, 0, 0,"VD_MESH_PROV_DATA_SET",0);

				DBG_FAST_PROVISION_SEND_STR("\n ***VD_MESH_PROV_DATA_SET");
				if(en_fast_revert_network == 1) {
					mesh_fast_prov_rcv_op(VD_MESH_PROV_COMPLETE);
					fast_prov.delay = 0;
					fast_prov.cur_sts = FAST_PROV_CONFIRM;
				}
				DBG_FAST_PROVISION_SEND_STR("\n ***FAST_PROV_SET_ADDR");
			}
			break;
		case VD_MESH_PROV_CONFIRM:
			if(fast_prov.cur_sts == FAST_PROV_NET_INFO){
				mesh_fast_prov_sts_set(FAST_PROV_CONFIRM);
				LOG_MSG_LIB(TL_LOG_NODE_SDK, 0, 0,"VD_MESH_PROV_CONFIRM",0);
			}
			break;
		case VD_MESH_PROV_COMPLETE:
			if(fast_prov.cur_sts == FAST_PROV_CONFIRM){
				mesh_fast_prov_sts_set(FAST_PROV_COMPLETE);
				LOG_MSG_LIB(TL_LOG_NODE_SDK, 0, 0,"VD_MESH_PROV_COMPLETE, delay:%x",fast_prov.delay);
				DBG_FAST_PROVISION_SEND_STR("\n ***VD_MESH_PROV_COMPLETE");
			}
			break;
		default:
			break;
	}
}

int cb_vd_mesh_reset_network(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	bool enable = true;
	uint16_t time_s = OPEN_DF_NW_DEFAULT_TIME;

	if(par_len == sizeof(open_default_nw_t)) {
		open_default_nw_t *p_set = (open_default_nw_t*)par;
		enable = (p_set->time_len[1]>>7)&0x1;
		time_s = p_set->time_len[0]|((p_set->time_len[1]&0x7F) << 8);
	}
	if(enable == true) {
		set_default_network_manual(enable, time_s);
		set_tmp_keys(1);
		DBG_FAST_PROVISION_SEND_STR("\n ON Default network");
	}
	else {
		set_default_network_manual(false, 0);
	}
	if(!fast_prov.not_need_prov){
		return -1;
	}
	fast_prov.start_tick = clock_time()|1;
	return 0;
}

int mesh_tx_cmd_get_mac_st(u8 idx, u16 ele_adr, u16 dst_adr, u8 *uuid, model_common_t *pub_md, u16 op_rsp)
{
    fast_prov_mac_st rsp;
   	memcpy(rsp.mac, tbl_mac, sizeof(rsp.mac));
	rsp.pid= MESH_PID_SEL;
    return mesh_tx_cmd_rsp(op_rsp, (u8 *)&rsp, OFFSETOF(fast_prov_mac_st, default_addr), ele_adr, dst_adr, uuid, pub_md);
}

int cb_vd_mesh_get_addr_st_rsp(mesh_cb_fun_par_t *cb_par)
{
    model_g_light_s_t *p_model = (model_g_light_s_t *)cb_par->model;
    return mesh_tx_cmd_get_mac_st(cb_par->model_idx, p_model->com.ele_adr, cb_par->adr_src, 0, 0, cb_par->op_rsp);
}

int cb_vd_mesh_get_addr(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	DBG_FAST_PROVISION_SEND_STR("\n cb_vd_mesh_get_addr: ");
	DBG_FAST_PROVISION_SEND_STR("not_need_prov: ");
	DBG_FAST_PROVISION_SEND_INT(fast_prov.not_need_prov);
	DBG_FAST_PROVISION_SEND_STR(", get_mac_en: ");
	DBG_FAST_PROVISION_SEND_INT(fast_prov.get_mac_en);
	DBG_FAST_PROVISION_SEND_STR(", cur_sts: ");
	DBG_FAST_PROVISION_SEND_INT(fast_prov.cur_sts);

	int err = -1;
	if(fast_prov.not_need_prov || (!fast_prov.get_mac_en)){
		return err;
	}
	if(fast_prov.cur_sts == FAST_PROV_SET_ADDR){
		return err;
	}
	model_common_t *p_model = (model_common_t*) cb_par->model;
	if(p_model->ele_adr != ele_adr_primary){
		return err;
	}
	mac_addr_get_t mac_get;
	memcpy(&mac_get, par, par_len);
	if((mac_get.pid == MESH_PID_SEL) || (mac_get.pid == 0xffff)){
		static u8 default_addr_random = 0;
		if((par_len >= sizeof(mac_addr_get_t)) && (!default_addr_random)){//for default ele_adr_primary conflict
			default_addr_random = 1;
			u16 tmp_ele_addr = mac_get.ele_addr+256+(u16)clock_time()%(0x8000-256-mac_get.ele_addr);
			tmp_ele_addr &= 0x7fff;
			mesh_set_ele_adr_ll(tmp_ele_addr, 0, 0);
		}
		get_mac_time = clock_time_ms();
		delay_time = rand()%5000;
		cb_para = *cb_par;
		err = 0;
	}
	mesh_fast_prov_rcv_op(cb_par->op);
	return err;

}

int cb_vd_mesh_set_addr(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	DBG_FAST_PROVISION_SEND_STR("\n ### \n cb_vd_mesh_set_addr: \n###\n");
	int err = -1;
	if(fast_prov.not_need_prov){
		return err;
	}
	if(!memcmp(par, tbl_mac, 6)){
		err = cb_vd_mesh_get_addr_st_rsp(cb_par);
		u16 addr = par[6] +(par[7]<<8);
		DBG_FAST_PROVISION_SEND_STR("\n PASS_0");
		if(addr&&is_unicast_adr(addr)){
			mesh_fast_prov_rcv_op(cb_par->op);
			mesh_set_ele_adr_ll(addr, 0, 0);
			DBG_FAST_PROVISION_SEND_STR("\n PASS_1");
		}
		fast_prov.get_mac_en = 0;
	}

	return err;
}

int cb_vd_mesh_set_provision_data(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	DBG_FAST_PROVISION_SEND_STR("\n cb_vd_mesh_set_provision_data");

	DBG_FAST_PROVISION_SEND_STR(".not_need_prov: ");
	DBG_FAST_PROVISION_SEND_INT(fast_prov.not_need_prov);
	DBG_FAST_PROVISION_SEND_STR(", get_mac_en: ");
	DBG_FAST_PROVISION_SEND_INT(fast_prov.get_mac_en);
	DBG_FAST_PROVISION_SEND_STR(", cur_sts: ");
	DBG_FAST_PROVISION_SEND_INT(fast_prov.cur_sts);

	DBG_FAST_PROVISION_SEND_STR("\n rcv_op: ");
	DBG_FAST_PROVISION_SEND_INT(fast_prov.rcv_op);

	if(fast_prov.not_need_prov){
		return -1;
	}
	mesh_fast_prov_rcv_op(cb_par->op);
	memset(&fast_prov.net_info.pro_data, 0x00, sizeof(provison_net_info_str));
	memcpy(&fast_prov.net_info.pro_data, par, sizeof(provison_net_info_str));
	fast_prov.net_info.pro_data.unicast_address = ele_adr_primary;//must, provision data's dst addrs is broadcast.
	memcpy(&fast_prov.net_info.appkey_set,par+sizeof(provison_net_info_str),sizeof(mesh_appkey_set_t));

	// Check enable fast revert network
	uint8_t normal_len =  \
			sizeof(provison_net_info_str) + sizeof(mesh_appkey_set_t);
	if(par_len == normal_len + 1) {
		en_fast_revert_network =  \
				(par[normal_len] == true)?true:false;
		DBG_FAST_PROVISION_SEND_STR("\n ______ ENABLE FAST REVERT NETWORK ______");
	} else {
		en_fast_revert_network = false;
	}

	return 0;
}

int cb_vd_mesh_provision_confirm(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	DBG_FAST_PROVISION_SEND_STR("\n cb_vd_mesh_provision_confirm");
	int err = -1;
	if(fast_prov.not_need_prov){
		return err;
	}
	mesh_fast_prov_rcv_op(cb_par->op);
	if(fast_prov.cur_sts == FAST_PROV_SET_ADDR){
		err = mesh_tx_cmd_rsp(cb_par->op_rsp, 0, 0, ele_adr_primary, cb_par->adr_src, 0, 0);
	}
	
	return err;
}

int cb_vd_mesh_provision_complete(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
	DBG_FAST_PROVISION_SEND_STR("\n cb_vd_mesh_provision_complete");
	mesh_fast_prov_rcv_op(cb_par->op);
	fast_prov.delay = par[0] + (par[1]<<8);
	del_tmp_keys();
	return 0;
}

//client
int cb_vd_mesh_addr_sts(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
    int err = 0;
    if(cb_par->model){  // model may be Null for status message
        //model_client_common_t *p_model = (model_client_common_t *)(cb_par->model);
    }
    return err;
}

int cb_vd_mesh_primary_addr_sts(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
    int err = 0;
    if(cb_par->model){  // model may be Null for status message
        //model_client_common_t *p_model = (model_client_common_t *)(cb_par->model);
    }
    return err;
}

int cb_vd_mesh_provison_data_sts(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
    int err = 0;
    if(cb_par->model){  // model may be Null for status message
        //model_client_common_t *p_model = (model_client_common_t *)(cb_par->model);
    }
    return err;
}

int cb_vd_mesh_provision_sts(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
    int err = 0;
    if(cb_par->model){  // model may be Null for status message
        //model_client_common_t *p_model = (model_client_common_t *)(cb_par->model);
    }
    return err;
}

int is_fast_prov_mode(){
	return fast_prov.cur_sts > FAST_PROV_IDLE ? 1 : 0;
}

#endif


