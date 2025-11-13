/*
 * sw_config.h
 *
 *  Created on: Jan 25, 2021
 *      Author: DungTranBK
 */

#ifndef SW_CONFIG_H_
#define SW_CONFIG_H_
/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj/tl_common.h"
#include "config_board.h"

#if DEV_TYPE_SEL == G_TYPE_SWITCH_BUTTON
/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define SWITCH_CONFIG_RESPONSE_MAX_LEN   16

#define PAR_LEN_MSG_SET_SW_MODE          0x2
#define PAR_LEN_MSG_SET_LED_INTENSITY    0x1
#define PAR_LEN_MSG_LOCK_SWITCH          0x1
#define PAR_LEN_MSG_LOCK_BIT_SWITCH      0x2
#define PAR_LEN_MSG_SET_TIMESTAMP        0x4
#define PAR_LEN_MSG_SET_ON_PW_UP_STATE   0x2

#define RL_ID_UNMAP     0

typedef struct
	{
		u8  toggle_type;
		u16 delay_time_s;
	}extended_param_t;

typedef struct
	{
		u8  switch_mode[NUMBER_RL];
		extended_param_t extended_param_st[NUMBER_RL];
		u16 link_unlink[NUMBER_RL];
		u8  map_input[NUMBER_RL];
		u8  en_group_default[NUMBER_RL];
		u8  power_on_state[NUMBER_RL];
		u8  rsv[64];
	}sw_config_t;

typedef struct
	{
		u8 map_input[NUMBER_RL];
	}map_input_t;

typedef struct
	{
		u8 link;
	}link_unlink_t;

typedef struct
	{
		u8 link:2;
		u8 btn_id:6;
	}link_unlink_v2_t;

typedef struct
	{
		u8  btn_id;
		u8  btn_mode;
		u8  toggle_type;
		u16 delay_s;
	}toggle_sw_mode_t;

typedef struct
	{
		bool en;
		u32  send_last_t;
		u8   op_index;
		u8   btn_idx;
		bool en_fast;
		u32  delay_st;
		bool en_delay;
		bool send_join;
	}sw_cfg_auto_t;

typedef struct
	{
		u8  btn_id;
		u8  btn_mode;
		u8  toggle_type;
		u16 delay_t;
	}btn_mode_set_t;

typedef struct
	{
		u8 btn_id;
		u8 lock_status;
	}lock_btn_set_t;

typedef struct
	{
		u8 btn_id;
		u8 state;
	}on_power_up_st_set_t;

/*
 *  Update / Response
 */
typedef struct
	{
		u8 op;
		u8 en;
	}en_dis_group_rsp_t;

typedef struct
	{
	    u8 op;
		u8 link;
	}update_link_unlink_t;

typedef struct
	{
	    u8 op;
	    link_unlink_v2_t link[NUMBER_RL];
	}update_link_unlink_v2_t;

typedef struct
	{
		u8 op;
		u8 btn_id;
		u8 mode;
		u8 toggle_type;
		u16 delay_t;
	}update_sw_mode_t;

typedef struct
	{
	    u8  op;
		u8  btn_id;
		u8  lock_mask[2];
	}update_lock_bitmask_t;

typedef struct
	{
	    u8 op;
		u8 btn_id;
		u8 lock_status;
	}update_lock_t;

typedef struct
	{
		u8 mode:4;
		u8 btn_id:4;
	}on_power_up_st_t;


typedef struct
	{
	    u8 op;
	    on_power_up_st_t on_power_up_st;
	}update_on_power_up_st_t;


typedef struct
	{
		u8 op;
		on_power_up_st_t on_power_up_st[NUMBER_RL];
	}update_all_on_power_up_st_t;

typedef struct
	{
		u8 op;
		u8 map_input[NUMBER_RL];
	}update_map_input_t;



#define AUTO_ON_OFF_MAX_TIME_S   10800
#define AUTO_ON_OFF_MIN_TIME_S   1
#define DEFAULT_BLE_MAC_LEN      0x6

#define BTN_ID_ALL               0

#define LOCK_UNLOCK_INVALID      TS_LOCK_UNKNOWN

extern sw_config_t sw_config_st;

#endif

typedef void (*typeConfig_handle_btn_change_mode)(u8 btn_id, u8 mode);

/******************************************************************************/
/*                            EXPORT FUNCTIONS                                */
/******************************************************************************/
#if INTER_PROVISION_EN
void sw_config_get_remaining_and_nw_mode(void);
void sw_config_send_network_mode(void);
void sw_config_get_reamaining_host_info(void);
int  sw_config_ble_mac_acording_to_zb(u8* mac_set);
#endif
void sw_config_auto_send_task(void);
int  sw_config_enable_auto_send(void);
void sw_setup_enable_send_config_delay(bool en, bool join_flag);
u8   sw_config_get_sw_mode(int idx);
void sw_config_init(void);
void sw_config_set_params(u8 type, int val);
int  sw_cfg_handle_set_message(int idx, u8* par, int par_len, u8 cmd);
int  sw_cfg_handle_get_message(int idx, u8* par, int par_len, u8 cmd);
int  sw_config_handle_response_from_mcu(u8* par, int par_len);
void sw_config_reset_all_device_config(void);
bool sw_get_en_disable_group_default(int idx);
void sw_disable_get_nw_mode(void);
void sw_config_get_nw_mode_periodic(void);
void sw_config_callback_init(typeConfig_handle_btn_change_mode func);

#endif /* SW_CONFIG_H_ */
