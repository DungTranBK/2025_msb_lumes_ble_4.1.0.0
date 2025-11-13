/*
 * net_message.h
 *
 *  Created on: Sep 19, 2020
 *      Author: DungTran BK
 */

#ifndef NET_MESSAGE_H_
#define NET_MESSAGE_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "../../../proj/tl_common.h"
/******************************************************************************/
/*                       EXPORT TYPE AND DEFINITION                           */
/******************************************************************************/

#if MD_LEVEL_EN
typedef struct {
	u8 type;
	u8 level;
	u8 tid;
	u8 transit_t;
	u8 delay;
}vd_cmd_g_level_set_t;
#endif

typedef struct {
	u8  st_present;
	u8  st_from_nwk;
	u8  binding_st;
	u32 binding_active_last_t;
}sw_control_t;

#define CONTROL_BDG_SAME_ST_CNT_MAX       0x3

extern u8 scene_active_arr[ELE_CNT];
extern u8 incomming_st[ELE_CNT];

typedef struct {
	u8  flag;
	u8  level;
	u16 lightness;
	u8  level_non_zz;
	u16 lightness_non_zz;
}app_control_state_t;

#if DEV_TYPE_SEL == G_TYPE_LM_DIMMER
extern app_control_state_t  app_control_state_st[ELE_CNT];
#endif

/******************************************************************************/
/*                              EXPORT FUNCTION                               */
/******************************************************************************/
u16 net_message_get_target_state(void);
int my_handle_mesh_cmd_sig_g_on_off_set(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par);
int my_handle_mesh_cmd_sig_g_on_off_get(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par);
int send_on_off_status(u8 idx, u8 status);
void send_control_dev_manual_to_mcu(u16 model_idx, bool state, bool update_en);
int func_handle_control_message(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par, u16 op);
u8 get_incomming_st(u16 idx);
void set_incomming_st(u16 idx, bool st);
int net_message_handle_state_change(u8 idx, u8 status);

#endif /* NET_MESSAGE_H_ */
