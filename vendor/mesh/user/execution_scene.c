/*
 * execution_scene.c
 *
 *  Created on: Apr 15, 2020
 *      Author: DungTran BK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "vendor/common/vendor_model.h"
#include "vendor/common/lighting_model_HSL.h"
#include "vendor/common/lighting_model.h"
#include "vendor/common/system_time.h"
#include "vendor/common/vendor_model.h"
#include "vendor/mesh/user/led_scene.h"
#include "vendor/mesh/user/led.h"
#include "utilities.h"
#include "key_report.h"
#include "execution_scene.h"

#include "debug.h"
#ifdef EXECUTION_SCENE_DBG_EN
	#define DBG_EXECUTION_SCENE_SEND_STR(x)          Dbg_sendString((s8*)x)
	#define DBG_EXECUTION_SCENE_SEND_INT(x)          Dbg_sendInt(x)
	#define DBG_EXECUTION_SCENE_SEND_HEX(x)          Dbg_sendHex(x)
    #define DBG_EXECUTION_SCENE_SEND_BYTE(x)         Dbg_sendHexOneByte(x)
#else
	#define DBG_EXECUTION_SCENE_SEND_STR(x)
	#define DBG_EXECUTION_SCENE_SEND_INT(x)
	#define DBG_EXECUTION_SCENE_SEND_HEX(x)
    #define DBG_EXECUTION_SCENE_SEND_BYTE(x)
#endif

#if EN_EXECUTION_SCENE
/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static par_execution_scene_t par_execution_scene[NUMBER_BUTTON][EV_BTN_MAX];
static setup_execution_scene_set_t setup_exe_scene_set;

const u16 op_sig_support_scene[] = {
	// HSL
	LIGHT_HSL_SET,
	LIGHT_HSL_SET_NOACK,
	// CTL
	LIGHT_CTL_SET,
	LIGHT_CTL_SET_NOACK,
	LIGHT_CTL_TEMP_SET,
	LIGHT_CTL_TEMP_SET_NOACK,
	LIGHTNESS_SET,
	LIGHTNESS_SET_NOACK,
	// Generic ON/OFF
	G_ONOFF_SET,
	G_ONOFF_SET_NOACK,
	// Generic Level
	G_LEVEL_SET,
	G_LEVEL_SET_NOACK
};

const u16 op_vd_support_scene[] = {
	// Auto Transition RGB
	VD_AUTO_TRANS_SET,
	VD_AUTO_TRANS_SET_NOACK
};
const u8 auto_send_arr[] =
	{
			BTN_KEY_PRESS_1_TIME,
			BTN_KEY_PRESS_2_TIMES,
			BTN_KEY_HOLD_2_SECONDS,
			EV_SW_ON,
			EV_SW_OFF,
			BTN_KEY_HOLD_UP,
			BTN_KEY_HOLD_DOWN,
			BTN_KEY_HOLD_UP_DOWN,
	};

#define support_event_arr   auto_send_arr

const u8 dimming_evt_arr[] = {
		BTN_KEY_HOLD_UP,
		BTN_KEY_HOLD_DOWN,
		BTN_KEY_HOLD_UP_DOWN,
};

static send_exe_scene_delay_t  send_exe_scene_delay_st = {
		.send_last_t_ms = 0,
		.en_send 	    = false,
		.btn_key	    = NUMBER_BUTTON,
		.send_ev_idx  	= 0xFF
};

#define PAR_SCENE        par_execution_scene[key_number][key_event]
#define SCENES_OUT_MAX   MAX_SCENE_DEST_ADR

static uint8_t dim_direc_arr[ELE_CNT][SCENES_OUT_MAX];

/******************************************************************************/
/*                        PRIVATE FUNCTIONS DECLERATION                       */
/******************************************************************************/

static int check_valid_event_and_key_number(u8 key_number, u8 key_ev);
static bool is_dimming_event(u8 evt);

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

/**
 * @func    get_key_event_index
 * @brief
 * @param   None
 * @retval  None
 */
static bool get_key_event_index(u8 *index, u8 evt)
{
	foreach_arr(i, support_event_arr) {
		if(evt == support_event_arr[i]) {
			*index = i;
			return true;
		}
	}
	return false;
}

/**
 * @func    restore_execution_scene
 * @brief
 * @param   None
 * @retval  None
 */
static void restore_execution_scene(void)
{
	flash_read_page(
			FLASH_ADR_EXECUTION_SCENE, sizeof(par_execution_scene), (u8 *)(&par_execution_scene)
		);
}

/**
 * @func    save_execution_scene_to_flash
 * @brief
 * @param   None
 * @retval  None
 */
static void save_execution_scene_to_flash(void)
{
	flash_erase_sector(FLASH_ADR_EXECUTION_SCENE);
	flash_write_page (
			FLASH_ADR_EXECUTION_SCENE, sizeof(par_execution_scene), (u8 *)(&par_execution_scene)
		);
}

/**
 * @func    delete_all_execution_scene
 * @brief
 * @param   None
 * @retval  None
 */
static void delete_all_execution_scene(void)
{
	memset(
		&par_execution_scene, 0xFF, sizeof(par_execution_scene)
	    );
	// Save To Flash
	save_execution_scene_to_flash();
}

/**
 * @func    delete_execution_scene
 * @brief
 * @param   None
 * @retval  None
 */
static void delete_execution_scene(u8 key_number, u8 key_idx)
{
	memset(
		&par_execution_scene[key_number][key_idx], 0xFF, sizeof(par_execution_scene_t)
		);
	save_execution_scene_to_flash();
}

/**
 * @func    event_is_support
 * @brief
 * @param   None
 * @retval  None
 */
static bool event_is_support(u8 event)
{
	foreach_arr(i, support_event_arr) {
		if(event == support_event_arr[i]) {
			return true;
		}
	}
	return false;
}

/**
 * @func    check_valid_event_and_key_number
 * @brief
 * @param   None
 * @retval  None
 */
static int check_valid_event_and_key_number(u8 key_number, u8 key_ev)
{
	if((key_number >= NUMBER_BUTTON) \
			||(event_is_support(key_ev) == false)){
		return -1;
	}
	return 0;
}

#ifdef EXECUTION_SCENE_DBG_EN
/**
 * @func    debug_execution_scene
 * @brief
 * @param   None
 * @retval  None
 */
static void debug_execution_scene(void)
{
	DBG_EXECUTION_SCENE_SEND_STR("\n EXECUTION SCENE: ");
	u8 *p = (u8*)&par_execution_scene;
	foreach(i, sizeof(par_execution_scene))
	{
		DBG_EXECUTION_SCENE_SEND_STR(" ");
		DBG_EXECUTION_SCENE_SEND_BYTE(p[i]);
	}
}
#endif

/**
 * @func    execution_scene_blink_led_status
 * @brief
 * @param   None
 * @retval  None
 */
static void execution_scene_blink_led_status(u16 mask, u8 status)
{
	u16 mask_rl = (mask >> NUMBER_SCENE)&BACKUP_MASK_RL;
	u8 blink_time = (status == EX_STATUS_SUCCESS)?1:2;
	if(mask_rl != 0)
	{
		led_blink_color(mask_rl, LED_COLOR_BLUE, blink_time, LAST_STATE_REFRESH_LED, TIMER_200MS);
	}
	else
	{
		led_scene_push_normal_blink_led_cmd_to_fifo(mask&BACKUP_MASK_SCENE, blink_time);
	}
}
/**
 * @func   execution_scene_handle_vendor_setup_execution_scene_set
 * @brief  None
 * @param
 * @retval Status code
 */
static int handle_vendor_setup_execution_scene_set(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{
    int err = 0;
    u8 index = 0;
    // Copy from Message Type And Message Id To Number Of Destination Address
    memcpy(
    	&setup_exe_scene_set.msg_type_and_id, &par[index], 5
    	);
    index += 4;

    u8 key_number = setup_exe_scene_set.setup_event_st.key_nbr;
	u8 key_ev  = setup_exe_scene_set.setup_event_st.key_event;

	uint8_t ev_idx = 0;
	(void)get_key_event_index(&ev_idx, key_ev);

	switch(setup_exe_scene_set.msg_type_and_id_st.msg_type)
	{
		case MSG_ADD:
		{
			DBG_EXECUTION_SCENE_SEND_STR("\n MSG_ADD");

			if(check_valid_event_and_key_number(key_number, key_ev) == 0) {
				if(setup_exe_scene_set.scene_id == SCENE_ID_INVALID) {
					DBG_EXECUTION_SCENE_SEND_STR("\n SCENE_ID_INVALID");
					if(setup_exe_scene_set.nums_dest > MAX_SCENE_DEST_ADR) {
						// Message Invalid
						err = -1;
					}
					else {
						// Delete, Not Save To Flash
						memset(
							&par_execution_scene[key_number][ev_idx], 0xFF, sizeof(par_execution_scene_t)
							);
						par_execution_scene[key_number][ev_idx].btn_event = key_ev;
						par_execution_scene[key_number][ev_idx].scene_id = setup_exe_scene_set.scene_id;

						u8 payload_len;

						foreach(i, setup_exe_scene_set.nums_dest) {
							par_execution_scene[key_number][ev_idx].control_infor[i].dest_addr = par[index+1] | (u16)(par[index+2] << 8);
							index += 2;
							payload_len = par[++index];
							if(payload_len > MAX_PAYLOAD_LEN)  payload_len = MAX_PAYLOAD_LEN;
							memcpy(
								&par_execution_scene[key_number][ev_idx].control_infor[i].payload, &par[++index], payload_len
								);
							par_execution_scene[key_number][ev_idx].control_infor[i].payload_len = payload_len;
							index += (payload_len - 1);

	#ifdef EXECUTION_SCENE_DBG_EN
							DBG_EXECUTION_SCENE_SEND_STR("\n Payload Len: ");
							DBG_EXECUTION_SCENE_SEND_INT(payload_len);
							DBG_EXECUTION_SCENE_SEND_STR("\n Payload: ");
							for(u8 j = 0; j < payload_len; j++) {
								DBG_EXECUTION_SCENE_SEND_BYTE(
										par_execution_scene[key_number][key_ev].control_infor[i].payload[j]
										);
								DBG_EXECUTION_SCENE_SEND_STR(" ");
							}
	#endif
						}
					}
					par_execution_scene[key_number][ev_idx].trans_time = par[++index];
					DBG_EXECUTION_SCENE_SEND_STR("\n trans time: ");
					DBG_EXECUTION_SCENE_SEND_BYTE(par_execution_scene[key_number][key_ev].trans_time);
				}
				else {
					// SINGLE SCENE
					if(setup_exe_scene_set.scene_id != 0xFFFF) {
						DBG_EXECUTION_SCENE_SEND_STR("\n SCENE_ID_VALID");
						// Delete
						delete_execution_scene(key_number, ev_idx);
						par_execution_scene[key_number][ev_idx].btn_event = key_ev;
						par_execution_scene[key_number][ev_idx].scene_id = setup_exe_scene_set.scene_id;
						par_execution_scene[key_number][ev_idx].trans_time = par[5];
					}
					 // MULTIPLE SCENE
					else {
						if((setup_exe_scene_set.nums_scene != 0)
								&& (setup_exe_scene_set.nums_scene <= MAX_SCENE_DEST_ADR)) {
							DBG_EXECUTION_SCENE_SEND_STR("\n MULTI SCENE");
							delete_execution_scene(key_number, ev_idx);

							par_execution_scene[key_number][ev_idx].btn_event = key_ev;
							par_execution_scene[key_number][ev_idx].scene_id = setup_exe_scene_set.scene_id;

							foreach(i, setup_exe_scene_set.nums_scene) {
								par_execution_scene[key_number][ev_idx].scene_control[i].in_scene_id = par[index + 1] | (u16)(par[index + 2] << 8);
								index += 2;
								par_execution_scene[key_number][ev_idx].scene_control[i].in_trans_t = par[++index];
								par_execution_scene[key_number][ev_idx].scene_control[i].in_delay_t = par[++index];
							}
						}
						par_execution_scene[key_number][ev_idx].trans_time = par[++index];
					}
				}
                // Dimming SET
                if(is_dimming_event(key_ev) == true) {
					u8 temp;

					DBG_EXECUTION_SCENE_SEND_STR("\n DIMMING SET: ");

					foreach_arr(i, dimming_evt_arr) {
						if(dimming_evt_arr[i] != key_ev) {
							DBG_EXECUTION_SCENE_SEND_STR("1, ");
							if(check_valid_event_and_key_number(key_number, dimming_evt_arr[i]) == 0) {
								DBG_EXECUTION_SCENE_SEND_STR("2, ");
								if(get_key_event_index(&temp, dimming_evt_arr[i]) == true) {
									DBG_EXECUTION_SCENE_SEND_STR("3\n");
									memset(&par_execution_scene[key_number][temp], 0xFF, sizeof(par_execution_scene_t));
								}
							}
						}
					}
				}
				// Flash
				save_execution_scene_to_flash();

				#ifdef EXECUTION_SCENE_DBG_EN
				DBG_EXECUTION_SCENE_SEND_STR("\n___check_ev: ");
				DBG_EXECUTION_SCENE_SEND_BYTE(setup_exe_scene_set.event);
				// debug_execution_scene();
                #endif
			}
			else {
				err = -1;
			}
			// Response Add
			execution_scene_add_rsp_str exe_scene_add_rsp;
			exe_scene_add_rsp.msg_type_and_id_st.msg_id = setup_exe_scene_set.msg_type_and_id_st.msg_id;
			exe_scene_add_rsp.msg_type_and_id_st.msg_type = MSG_ADD_RSP;
			exe_scene_add_rsp.status_code = ((err == 0)?(EX_STATUS_SUCCESS):(EX_STATUS_FAIL));
			exe_scene_add_rsp.event = setup_exe_scene_set.event;
			exe_scene_add_rsp.scene_id = setup_exe_scene_set.scene_id;

			err = mesh_tx_cmd_rsp(
						 VD_SETUP_EXECUTION_SCENE_STATUS,
						 (u8 *)&exe_scene_add_rsp,
						 sizeof(execution_scene_add_rsp_str),
						 ele_adr_primary,
						 GATEWAY_UNICAST_ADDR,
						 0,
						 0 );
			execution_scene_blink_led_status(1 << key_number, exe_scene_add_rsp.status_code);
			break;
		}

		case MSG_DELETE:
		{
			DBG_EXECUTION_SCENE_SEND_STR("\n MSG_DELETE: ");

			execution_scene_del_rsp_str exe_scene_del_rsp;
			exe_scene_del_rsp.event = setup_exe_scene_set.event;

			if(check_valid_event_and_key_number(key_number, key_ev) == 0) {
				delete_execution_scene(key_number, ev_idx);
				DBG_EXECUTION_SCENE_SEND_STR("\n DELETE COMPLETE");
			}
			else {
				// Message Invalid
				err = -1;
			}
			// Response Delete
			exe_scene_del_rsp.msg_type_and_id_st.msg_id = setup_exe_scene_set.msg_type_and_id_st.msg_id;
			exe_scene_del_rsp.msg_type_and_id_st.msg_type = MSG_DELETE_RSP;
			exe_scene_del_rsp.status_code = ((err == 0)?(EX_STATUS_SUCCESS):(EX_STATUS_FAIL));

			err = mesh_tx_cmd_rsp(
						 VD_SETUP_EXECUTION_SCENE_STATUS,
						 (u8 *)&exe_scene_del_rsp,
						 sizeof(execution_scene_del_rsp_str),
						 ele_adr_primary,
						 GATEWAY_UNICAST_ADDR,
						 0,
						 0 );
			execution_scene_blink_led_status(1 << key_number, exe_scene_del_rsp.status_code);
			break;
		}

		case MSG_GET:
		{
			DBG_EXECUTION_SCENE_SEND_STR("\n MSG_GET");

			u8 rsp[84];
			u8 len = 0;
			u8 idx = 0;
			header_execution_scene_get_rsp_t  header_get_rsp_st;

			header_get_rsp_st.msg_type_and_id_st.msg_id = setup_exe_scene_set.msg_type_and_id_st.msg_id;
			header_get_rsp_st.msg_type_and_id_st.msg_type = MSG_GET_RSP;
			header_get_rsp_st.event = setup_exe_scene_set.event;

			if(check_valid_event_and_key_number(key_number, key_ev) != 0) {
				// Event Not Configuration Before, return fail
				DBG_EXECUTION_SCENE_SEND_STR("\n EX_STATUS_FAIL, Event Invalid ");
				header_get_rsp_st.status_code = EX_STATUS_FAIL;
				len = 3;
				memcpy(&rsp, &header_get_rsp_st, len);
			}
			else {
				if(par_execution_scene[key_number][ev_idx].btn_event == 0xFF) {
					// Event Not Configuration Before, return fail
					DBG_EXECUTION_SCENE_SEND_STR("\n EX_STATUS_NOT_SET_BEFORE ");
					header_get_rsp_st.status_code = EX_STATUS_NOT_SET_BEFORE;
					len = 3;
					memcpy(&rsp, &header_get_rsp_st,len);
				}
				else {
					/*
					 * Response payload:
					 * - Message type and message id: 1 byte
					 * - Status Code: 1 byte
					 * - Event: 1 byte
					 * - Scene ID: 2 byte
					 * - Number of destination address: 1 byte
					 * - Control Information: variable
					 * - Transition Time: 1 byte
					 * - Additional Condition: variable
					 */
					DBG_EXECUTION_SCENE_SEND_STR("\n EX_STATUS_SUCCESS ");
					header_get_rsp_st.status_code = EX_STATUS_SUCCESS;
					header_get_rsp_st.scene_id = par_execution_scene[key_number][ev_idx].scene_id;

					// SINGLE SCENE OR CONTROL DIRECTLY

					if(header_get_rsp_st.scene_id != 0xFFFF) {
						header_get_rsp_st.nbr_of_dest = 0;

						foreach(i, MAX_SCENE_DEST_ADR){
							if(par_execution_scene[key_number][ev_idx].control_infor[i].payload_len > MAX_PAYLOAD_LEN) {
								continue;
							}
							header_get_rsp_st.nbr_of_dest++;
						}
						idx = sizeof(header_execution_scene_get_rsp_t);
						len = idx;
						memcpy(&rsp, &header_get_rsp_st, idx);

						foreach(i, header_get_rsp_st.nbr_of_dest) {
							// Destination Address
							rsp[idx++] = (u8)par_execution_scene[key_number][ev_idx].control_infor[i].dest_addr;
							rsp[idx++] = (u8)(par_execution_scene[key_number][ev_idx].control_infor[i].dest_addr >> 8);
							// payload len
							rsp[idx++] = par_execution_scene[key_number][ev_idx].control_infor[i].payload_len;
							len += 3;
							// payload
							foreach(j, par_execution_scene[key_number][ev_idx].control_infor[i].payload_len) {
								u8 *tmp = (u8*)&par_execution_scene[key_number][ev_idx].control_infor[i].payload;
								rsp[idx++] = *(tmp + j);
								len++;
							}
						}
					}

					// MULTI SCENE
					else {
						header_get_rsp_st.nbr_of_scene = 0;
						foreach(i, MAX_SCENE_DEST_ADR){
							if(par_execution_scene[key_number][ev_idx].scene_control[i].in_scene_id == LM_SCENE_IN_INVALID) {
								continue;
							}
							header_get_rsp_st.nbr_of_scene++;
						}
						idx = sizeof(header_execution_scene_get_rsp_t);
						len = idx;
						memcpy(&rsp, &header_get_rsp_st, idx);

						foreach(i, header_get_rsp_st.nbr_of_scene) {
							// Scene ID
							rsp[idx++] = (u8)par_execution_scene[key_number][ev_idx].scene_control[i].in_scene_id;
							rsp[idx++] = (u8)(par_execution_scene[key_number][ev_idx].scene_control[i].in_scene_id >> 8);
							rsp[idx++] = par_execution_scene[key_number][ev_idx].scene_control[i].in_trans_t;
							rsp[idx++] = par_execution_scene[key_number][ev_idx].scene_control[i].in_delay_t;
							len += 4;
						}
					}
					rsp[len++] = par_execution_scene[key_number][ev_idx].trans_time;
				}
			}

			err = mesh_tx_cmd_rsp(
					 VD_SETUP_EXECUTION_SCENE_STATUS,
					 (u8 *)&rsp,
					 len,
					 ele_adr_primary,
					 GATEWAY_UNICAST_ADDR,
					 0,
					 0);
			break;
		}

		case MSG_DELETE_ALL:
		{
			DBG_EXECUTION_SCENE_SEND_STR("\n MSG_DELETE_ALL");

			delete_all_execution_scene();
			// Response Delete All
			execution_scene_del_all_rsp_str exe_scene_del_all_rsp;
			exe_scene_del_all_rsp.msg_type_and_id_st.msg_id = setup_exe_scene_set.msg_type_and_id_st.msg_id;
			exe_scene_del_all_rsp.msg_type_and_id_st.msg_type = MSG_DELETE_ALL_RSP;
			exe_scene_del_all_rsp.status_code = EX_STATUS_SUCCESS;

			err = mesh_tx_cmd_rsp(
					 VD_SETUP_EXECUTION_SCENE_STATUS,
					 (u8 *)&exe_scene_del_all_rsp,
					 sizeof(execution_scene_del_all_rsp_str),
					 ele_adr_primary,
					 GATEWAY_UNICAST_ADDR,
					 0,
					 0);
			execution_scene_blink_led_status(0xFFFF, EX_STATUS_SUCCESS);
			break;
		}
	}
    return err;
}

/**
 * @func   execution_scene_init
 * @brief  None
 * @param
 * @retval Status code
 */
void execution_scene_init(void)
{
    vendor_handle_func_callback_init(
				handle_vendor_setup_execution_scene_set
			);
    restore_execution_scene();

	// first state
	foreach(i, ELE_CNT) {
		foreach(j, SCENES_OUT_MAX) {
			dim_direc_arr[i][j] = UD_UP;
		}
	}
}


/**
 * @func   execution_scene_active
 * @brief  None
 * @param
 * @retval Status code
 */
void execution_scene_active(u8 key_number, u8 key_ev)
{
	static uint8_t tid, vd_tid;
	uint8_t ev_idx = 0;

	(void)get_key_event_index(&ev_idx, key_ev);

	DBG_EXECUTION_SCENE_SEND_STR("\n ...sizeof(par_execution_scene): ");
	DBG_EXECUTION_SCENE_SEND_INT(sizeof(par_execution_scene));

	if(check_valid_event_and_key_number(key_number, key_ev) == 0) {
		if(par_execution_scene[key_number][ev_idx].btn_event == key_ev) {
			if(par_execution_scene[key_number][ev_idx].scene_id != SCENE_ID_INVALID) {
				if(par_execution_scene[key_number][ev_idx].scene_id != 0xFFFF) {
					// Control Directly By Scene ID
					vendor_scene_recall_t vendor_scene_recall;
					vendor_scene_recall.msg_type = VENDOR_SCENE_RECALL_NOACK;
					vendor_scene_recall.scene_id = par_execution_scene[key_number][ev_idx].scene_id;
					vendor_scene_recall.tid = ++vd_tid;
					vendor_scene_recall.trans_time = par_execution_scene[key_number][ev_idx].trans_time;
#if CTL_WITH_FLAG_DEFAULT_POSITION_EN
					vendor_scene_recall.delay_time = 1;
#else
					vendor_scene_recall.delay_time = 0;
#endif
					DBG_EXECUTION_SCENE_SEND_STR("\n >> Send Scene Recall");

					mesh_tx_cmd_rsp(
							VD_SCENE_REQUEST_NOACK,(u8 *)&vendor_scene_recall, sizeof(vendor_scene_recall_t), ele_adr_primary, 0xFFFF, 0, 0
							);
				}
				else {
					foreach(j, MAX_SCENE_DEST_ADR)
					{
						DBG_EXECUTION_SCENE_SEND_STR("\n RETRY");

						if(par_execution_scene[key_number][ev_idx].scene_control[j].in_scene_id != LM_SCENE_IN_INVALID) {
							// Control Directly By Scene ID
							vendor_scene_recall_t vendor_scene_recall;
							vendor_scene_recall.msg_type = VENDOR_SCENE_RECALL_NOACK;
							vendor_scene_recall.scene_id = par_execution_scene[key_number][ev_idx].scene_control[j].in_scene_id;
							vendor_scene_recall.tid = ++vd_tid;
							vendor_scene_recall.trans_time = par_execution_scene[key_number][ev_idx].scene_control[j].in_trans_t;
#if CTL_WITH_FLAG_DEFAULT_POSITION_EN
							vendor_scene_recall.delay_time = 1;
#else
							vendor_scene_recall.delay_time = par_execution_scene[key_number][ev_idx].scene_control[j].in_delay_t;
#endif
							DBG_EXECUTION_SCENE_SEND_STR("\n >> Recall \n");
							DBG_EXECUTION_SCENE_SEND_HEX(par_execution_scene[key_number][key_ev].scene_control[j].in_scene_id);

#ifdef EXECUTION_SCENE_DBG_EN
							DBG_EXECUTION_SCENE_SEND_STR("\n");
							u8 *temp = (u8*)&vendor_scene_recall.msg_type;
							foreach(i, sizeof(vendor_scene_recall_t)){
								DBG_EXECUTION_SCENE_SEND_BYTE(*(temp + i));
								DBG_EXECUTION_SCENE_SEND_STR(" ");
							}
#endif
							// -->
							mesh_tx_cmd_rsp(
										VD_SCENE_REQUEST_NOACK,
										(u8 *)&vendor_scene_recall,
										sizeof(vendor_scene_recall_t),
										ele_adr_primary,
										0xFFFF,
										0,
										0
									);
						}
					}
				}
			}
			else {
				// Control Directly By Opcode
				for(u8 i = 0; i < MAX_SCENE_DEST_ADR; i++) {
					if(par_execution_scene[key_number][ev_idx].control_infor[i].dest_addr != ADR_UNASSIGNED) {
						// Check Payload Len Valid Or Invalid
						if(par_execution_scene[key_number][ev_idx].control_infor[i].payload_len > MAX_PAYLOAD_LEN) {
							DBG_EXECUTION_SCENE_SEND_STR("\n PAR LEN INVALID");
							continue;
						}
						// Destination Address
						DBG_EXECUTION_SCENE_SEND_STR("\n Dest Adr: ");
						DBG_EXECUTION_SCENE_SEND_HEX(par_execution_scene[key_number][key_ev].control_infor[i].dest_addr);

						foreach_arr(j, op_sig_support_scene){
							u16 op_sig = (par_execution_scene[key_number][ev_idx].control_infor[i].payload[1] << 8) |     \
									par_execution_scene[key_number][ev_idx].control_infor[i].payload[0];
							if(op_sig == op_sig_support_scene[j]) {
								// Sig Opcode
								DBG_EXECUTION_SCENE_SEND_STR("\n Sig Opcode: ");
								DBG_EXECUTION_SCENE_SEND_HEX(op_sig);

								u8 par_control[MAX_PAYLOAD_LEN];
								u8 par_len = par_execution_scene[key_number][ev_idx].control_infor[i].payload_len - 2;

								// Main Par
								memcpy(
									&par_control,
									&par_execution_scene[key_number][ev_idx].control_infor[i].payload[2],
									par_len
								   );
								// Tid + Transition Time + Delay Time
								tid++;
								par_control[par_len++] = tid;  // Transition Identify
								par_control[par_len++] = par_execution_scene[key_number][ev_idx].trans_time; // Transition Time (step 100 ms)
                                #if CTL_WITH_FLAG_DEFAULT_POSITION_EN
								par_control[par_len++] = 1;   // Delay Time
                                #else
								par_control[par_len++] = 0;   // Delay Time
                                #endif
								// Control
								mesh_tx_cmd_rsp(
										op_sig,
										(u8 *)&par_control,
										par_len,
										ele_adr_primary,
										par_execution_scene[key_number][ev_idx].control_infor[i].dest_addr,
										0,
										0
									);
							}
						}
						// Vendor Opcode
						u8 op_vd = par_execution_scene[key_number][ev_idx].control_infor[i].payload[0];

						if(op_vd >= START_VENDOR_OPCODE) {
							DBG_EXECUTION_SCENE_SEND_STR("\n Vendor Opcode: ");
							DBG_EXECUTION_SCENE_SEND_HEX(op_vd);
							// Control
							mesh_tx_cmd_rsp(
									op_vd,
									(u8 *)&par_execution_scene[key_number][ev_idx].control_infor[i].payload[3],
									par_execution_scene[key_number][ev_idx].control_infor[i].payload_len - 3,
									ele_adr_primary,
									par_execution_scene[key_number][ev_idx].control_infor[i].dest_addr,
									0,
									0
								);
						}
					}
				}
			}
		}
	}
}

/**
 * @func   execution_set_up_auto_send
 * @brief  None
 * @param
 * @retval None
 */
void execution_set_up_auto_send(void)
{
	send_exe_scene_delay_st.send_last_t_ms = clock_time_ms() - TIMER_2S;
	send_exe_scene_delay_st.en_send = true;
	send_exe_scene_delay_st.btn_key = 0;
	send_exe_scene_delay_st.send_ev_idx = 0;
}

/**
 * @func   execution_loop_task
 * @brief  None
 * @param
 * @retval None
 */
void execution_loop_task(void)
{
	if(send_exe_scene_delay_st.en_send == true) {
        if(clock_time_exceed_ms(send_exe_scene_delay_st.send_last_t_ms, TIMER_2S)) {
        	send_exe_scene_delay_st.send_last_t_ms = clock_time_ms();
        	if((send_exe_scene_delay_st.btn_key < NUMBER_BUTTON)
        			&& (send_exe_scene_delay_st.send_ev_idx < sizeof(auto_send_arr))) {

        		msg_get_execution_scene_t  exe_scene_get_st;
        		exe_scene_get_st.msg_type_and_id_st.msg_type = MSG_GET;
        		exe_scene_get_st.msg_type_and_id_st.msg_id = 0;
        		exe_scene_get_st.event =
        				((send_exe_scene_delay_st.btn_key << 4)&0xF0)|(auto_send_arr[send_exe_scene_delay_st.send_ev_idx]&0x0F);

        		handle_vendor_setup_execution_scene_set(
									(u8*)&exe_scene_get_st,
									sizeof(msg_get_execution_scene_t),
									NULL
        						);
        		send_exe_scene_delay_st.send_ev_idx++;
        		if(send_exe_scene_delay_st.send_ev_idx >= sizeof(auto_send_arr)){
        			send_exe_scene_delay_st.send_ev_idx = 0;
        			send_exe_scene_delay_st.btn_key++;
        		}
        	}
        	else{
        		send_exe_scene_delay_st.en_send = false;
        		DBG_EXECUTION_SCENE_SEND_STR("\nExit auto send");
        	}
        }
	}
}

/**
 * @func   is_dimming_event
 * @brief  None
 * @param
 * @retval Status code
 */
static bool is_dimming_event(u8 evt)
{
	foreach_arr(i, dimming_evt_arr) {
		if(evt == dimming_evt_arr[i]) {
			return true;
		}
	}
	return false;
}

/**
 * @func   execution_event_is_available
 * @brief
 * @param  byte event_id
 * @retval TBL_INDEX if it's available
 */
static bool execution_event_is_available(u8 idx, u8 event)
{
	foreach(i, EV_BTN_MAX) {
		if(par_execution_scene[idx][i].btn_event == event) {
			return true;
		}
	}
	return false;
}

/**
 * @func   execution_dimming_is_exist
 * @brief
 * @param
 * @retval DIMMING control is exist or not
 */
bool execution_dimming_is_exist(u8 idx, u8* evt)
{
	bool ret_code;
	// dimming up
	ret_code = execution_event_is_available(idx, BTN_KEY_HOLD_UP);
	if(ret_code == true) {
		DBG_EXECUTION_SCENE_SEND_STR("\n BTN_KEY_HOLD_UP");
		*evt = BTN_KEY_HOLD_UP;
		return TRUE;
	}
	// dimming down
	ret_code = execution_event_is_available(idx, BTN_KEY_HOLD_DOWN);
	if(ret_code == true) {
		DBG_EXECUTION_SCENE_SEND_STR("\n BTN_KEY_HOLD_DOWN");
		*evt = BTN_KEY_HOLD_DOWN;
		return TRUE;
	}
	// dimming up down
	ret_code = execution_event_is_available(idx, BTN_KEY_HOLD_UP_DOWN);
	if(ret_code == true) {
		DBG_EXECUTION_SCENE_SEND_STR("\n BTN_KEY_HOLD_UP_DOWN");
		*evt = BTN_KEY_HOLD_UP_DOWN;
		return TRUE;
	}
	return FALSE;
}

/**
 * @func   execution_dimming_control
 * @brief
 * @param  None
 * @retval None
 */
static void execution_dimming_control(
		u8 idx, u8 dst_pos, u16 dst, vd_up_down_control_t* vd_up_down, u8 st
) {
	u8 len = sizeof(vd_up_down_control_t);

	if(st >= B_ST_UNKNOWN) {
		return;
	}
	if(st == B_START_PRESS) {
		if((vd_up_down->dir == DIR_UP)||(vd_up_down->dir == DIR_UP_SMOOTH)) {
			vd_up_down->dir = DIR_UP;
		}
		else if((vd_up_down->dir == DIR_DOWN)||(vd_up_down->dir == DIR_DOWN_SMOOTH)) {
			vd_up_down->dir = DIR_DOWN;
		}
		else {
			return;
		}
		mesh_tx_cmd_rsp(
				VD_DIMMING_CONTROL_SET, (u8*)vd_up_down, len, ele_adr_primary, dst, 0, 0
			);
	}
	else if(st == B_HOLD_500MS) {
		if((vd_up_down->dir == DIR_UP)||(vd_up_down->dir == DIR_UP_SMOOTH)) {
			vd_up_down->dir = DIR_UP_SMOOTH;
		}
		else if((vd_up_down->dir == DIR_DOWN)||(vd_up_down->dir == DIR_DOWN_SMOOTH)) {
#if EXECUTION_SCENE_DIMMING_LIMIT_EN

			vd_up_down->dir = DIR_DOWN_SMOOTH_LIMIT;
#else
			vd_up_down->dir = DIR_DOWN_SMOOTH;
#endif
		}
		else if(vd_up_down->dir == DIR_UP_DOWN_SMOOTH) {
#if EXECUTION_SCENE_DIMMING_LIMIT_EN
			vd_up_down->dir = (dim_direc_arr[idx][dst_pos] == UD_UP)? DIR_UP_SMOOTH:DIR_DOWN_SMOOTH_LIMIT;
#else
			vd_up_down->dir = (dim_direc_arr[idx][dst_pos] == UD_UP)? DIR_UP_SMOOTH:DIR_DOWN_SMOOTH;
#endif
			dim_direc_arr[idx][dst_pos] = (dim_direc_arr[idx][dst_pos]^0x01)&0x01;
		}
		else {
			return;
		}
		// Led notify dimming direction
#if 0
		LedColor_enum color =  \
				(vd_up_down->dir == DIR_DOWN_SMOOTH)?LED_COLOR_WHITE:LED_COLOR_GREEN;
		execution_scene_blink_led_status(1 << idx, color);
#else
#if EXECUTION_SCENE_DIMMING_LIMIT_EN
		if(vd_up_down->dir == DIR_DOWN_SMOOTH_LIMIT) {
#else
		if(vd_up_down->dir == DIR_DOWN_SMOOTH) {
#endif
			led_scene_push_normal_blink_led_cmd_to_fifo(1 << idx, 2);
			DBG_EXECUTION_SCENE_SEND_STR("\n***DIR_DOWN_SMOOTH");
		}
		else {
			led_scene_push_normal_blink_led_cmd_to_fifo(1 << idx, 1);
			DBG_EXECUTION_SCENE_SEND_STR("\n***DIR_UP_SMOOTH");
		}
#endif
		// CONTROL
		mesh_tx_cmd_rsp(
				VD_DIMMING_CONTROL_SET, (u8*)vd_up_down, len, ele_adr_primary, dst, 0, 0
			);
	}
	else if(st == B_RELEASE) {
		if((vd_up_down->dir == DIR_UP)
				||(vd_up_down->dir == DIR_UP_SMOOTH)  \
					|| (vd_up_down->dir == DIR_DOWN)  \
						|| (vd_up_down->dir == DIR_DOWN_SMOOTH) \
							|| (vd_up_down->dir == DIR_UP_DOWN_SMOOTH)) {
			vd_up_down->dir = DIR_STOP;
			mesh_tx_cmd_rsp(
					VD_DIMMING_CONTROL_SET, (u8*)vd_up_down, len, ele_adr_primary, dst, 0, 0
				);
		}
	}
#ifdef  EXECUTION_SCENE_DBG_EN
	DBG_EXECUTION_SCENE_SEND_STR("\n execution_dimming_control: ");
	u8* temp = (u8*)vd_up_down;
	foreach(i, len) {
		DBG_EXECUTION_SCENE_SEND_BYTE(*(temp+i));
		DBG_EXECUTION_SCENE_SEND_STR(" ");
	}
	DBG_EXECUTION_SCENE_SEND_STR("\nDST: ");
	DBG_EXECUTION_SCENE_SEND_HEX(dst);
#endif
}

/**
 * @func   scenes_dimming_control_by_table_index
 * @brief
 * @param  None
 * @retval None
 */
int execution_dimming_control_by_key_number(u8 key_number, u8 evt, u8 st)
{
	u8 dim_direct;
	u8 ev_idx = 0xFF;
	switch(evt)
	{
		case BTN_KEY_HOLD_UP:
			dim_direct = DIR_UP_SMOOTH;
			if(false == get_key_event_index(&ev_idx, BTN_KEY_HOLD_UP))
			{
				return -1;
			}
			break;
		case BTN_KEY_HOLD_DOWN:
			dim_direct = DIR_DOWN_SMOOTH;
			if(false == get_key_event_index(&ev_idx, BTN_KEY_HOLD_DOWN))
			{
				return -1;
			}
			break;
		case BTN_KEY_HOLD_UP_DOWN:
			dim_direct = DIR_UP_DOWN_SMOOTH;
			if(false == get_key_event_index(&ev_idx, BTN_KEY_HOLD_UP_DOWN))
			{
				return -1;
			}
			break;
		default:
			return -1;
	}
	for(u8 dev_index = 0; dev_index < MAX_SCENE_DEST_ADR; dev_index++) {
		if(par_execution_scene[key_number][ev_idx].control_infor[dev_index].dest_addr != ADR_UNASSIGNED)
		{
			control_infor_t ctl_info =  \
					par_execution_scene[key_number][ev_idx].control_infor[dev_index];
			u8 opcode = ctl_info.payload[0];
			if(opcode == VD_DIMMING_CONTROL_SET) {
				vd_up_down_control_t ud_control_st;
				memcpy((u8*)&ud_control_st, (u8*)&ctl_info.payload[3], sizeof(vd_up_down_control_t));
				ud_control_st.dir = dim_direct;
				execution_dimming_control(key_number, dev_index,  \
						ctl_info.dest_addr, (vd_up_down_control_t*)&ud_control_st, st);
			}
		}
	}
	return 0;
}

#endif
// End File
