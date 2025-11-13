/*
 * lock_schedule.h
 *
 *  Created on: Mar 10, 2024
 *      Author: DungTranBK
 */

#ifndef VENDOR_USER_LOCK_SCHEDULE_H_
#define VENDOR_USER_LOCK_SCHEDULE_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj_lib/sig_mesh/app_mesh.h"
#include "config_board.h"
#include "utilities.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/
typedef struct {
	u32 is_ready_flag;
	u16 present_min;
	u8  set_lock_val[NUMBER_BUTTON];
	u8  present_lock[NUMBER_BUTTON];
}lock_com_par_t;

typedef struct {
	u8  lock;
	u8  en_schedule;
	u16 start_t_min;
	u16 stop_t_min;
	u8  active_day;
	u8  rsv;
}lock_schedule_t;

typedef int (*type_lock_schedule_send_config_to_mcu)(int idx, u8 lock_value);

#define ALL_DAY_OF_WEEK        0x7F

#define LM_WEEK_DAY_MAX        6

/******************************************************************************/
/*                            EXPORT FUNCTIONS                                */
/******************************************************************************/
bool button_is_lock(u8 idx);
void lock_schedule_init(void);
void lock_schedule_task(void);
int vd_lock_schedule_get(u8* par, u8 par_len, mesh_cb_fun_par_t *cb_par);
int vd_lock_schedule_set(u8* par, u8 par_len, mesh_cb_fun_par_t *cb_par);
int lock_schedule_update_present_status(u8 idx, lock_unlock_enum status);

int vd_lock_schedule_response(u8 idx, u16 src_adr, u16 dst_adr);
void lock_schedule_reset_to_default(void);
void sw_handle_internal_lock_button(u8 idx, u8 lock_config, u8 en_rsp);

#endif /* VENDOR_USER_LOCK_SCHEDULE_H_ */
