/*
 * lock_schedule.c
 *
 *  Created on: Mar 10, 2024
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/system_time.h"
#include "utilities.h"
#include "flash_user.h"
#include "led_ev.h"
#include "led_scene.h"
#include "timestamp.h"
#include "sw_config.h"
#include "lock_schedule.h"

#include "debug.h"
#ifdef  LOCK_SCH_DBG_EN
#define DBG_LOCK_SCH_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_LOCK_SCH_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_LOCK_SCH_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_LOCK_SCH_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
#else
#define DBG_LOCK_SCH_SEND_STR(x)
#define DBG_LOCK_SCH_SEND_INT(x)
#define DBG_LOCK_SCH_SEND_HEX(x)
#define DBG_LOCK_SCH_SEND_BYTE(x)
#endif

/*
 * wday standard: Sunday - 0, Monday - 1.....
 */

type_lock_schedule_send_config_to_mcu pv_lock_schedule_send_config_to_mcu = NULL;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static lock_com_par_t lock_com_par = {
		.is_ready_flag = false, .present_min = MAX_U8 };

lock_schedule_t lock_schedule_st[ELE_CNT];

static int lock_schedule_flash_idx = FLASH_INDEX_DEFAULT;
#define FLASH_SIZE_LOCK_SCHEDULE     4000
#define BLOCK_SIZE_LOCK_SCHEDULE     sizeof(lock_schedule_st)

static int lock_status_flash_idx     = FLASH_INDEX_DEFAULT;
#define FLASH_SIZE_LOCK_STATUS       4000
#define BLOCK_SIZE_LOCK_STATUS       sizeof(lock_com_par.present_lock)

/******************************************************************************/
/*                       PRIVATE FUNCTION DECLERATION                         */
/******************************************************************************/


/******************************************************************************/
/*                            EXPORT FUNCTION                                 */
/******************************************************************************/

/**
 * @func    button_is_lock
 * @brief
 * @param   None
 * @retval  None
 */
bool button_is_lock(u8 idx)
{
	if(idx < NUMBER_BUTTON)
	{
		if(lock_com_par.present_lock[idx] == TS_LOCK)
		{
			return true;
		}
	}
	return false;
}

/**
 * @func    current_lock_status_store
 * @brief
 * @param   None
 * @retval  None
 */
static void current_lock_status_store(void)
{
	flash_user_store(
			(int*)&lock_status_flash_idx,
			FLASH_ADR_LOCK_STATUS, FLASH_SIZE_LOCK_STATUS, BLOCK_SIZE_LOCK_STATUS, (u8*)lock_com_par.present_lock
		);
#ifdef LOCK_SCH_DBG_EN
	DBG_LOCK_SCH_SEND_STR("\n current_lock_status_store: ");
	DBG_LOCK_SCH_SEND_INT(lock_status_flash_idx);
	DBG_LOCK_SCH_SEND_STR("\n");
	foreach(i, NUMBER_BUTTON) {
		DBG_LOCK_SCH_SEND_STR("\nmodel_index: ");
		DBG_LOCK_SCH_SEND_INT(i);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_INT(lock_com_par.present_lock[i]);
	}
#endif
}

/**
 * @func    lock_schedule_restore
 * @brief
 * @param   None
 * @retval  None
 */
static void current_lock_status_restore(void)
{
	flash_user_get_flash_index(
		        (int*)&lock_status_flash_idx,
				FLASH_ADR_LOCK_STATUS, FLASH_SIZE_LOCK_STATUS, BLOCK_SIZE_LOCK_STATUS, (u8*)lock_com_par.present_lock
			);
	flash_user_restore(lock_status_flash_idx,
				FLASH_ADR_LOCK_STATUS, BLOCK_SIZE_LOCK_STATUS, (u8*)lock_com_par.present_lock
			);
	// Default
	bool is_default = false;
	foreach(i, NUMBER_BUTTON) {
		if(lock_com_par.present_lock[i] >= TS_LOCK_UNKNOWN) {
			lock_com_par.present_lock[i] = TS_UNLOCK;
			is_default = true;
			DBG_LOCK_SCH_SEND_STR("\n ...DF");
		}
	}
	if(is_default) {
		current_lock_status_store();
	}
#ifdef LOCK_SCH_DBG_EN
	DBG_LOCK_SCH_SEND_STR("\n current_lock_status_restore: ");
	DBG_LOCK_SCH_SEND_INT(lock_status_flash_idx);
	DBG_LOCK_SCH_SEND_STR("\n ");

	foreach(i, NUMBER_BUTTON) {
		DBG_LOCK_SCH_SEND_STR("\nmodel_index: ");
		DBG_LOCK_SCH_SEND_INT(i);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_INT(lock_com_par.present_lock[i]);
	}
#endif
}


/**
 * @func    lock_schedule_store
 * @brief
 * @param   None
 * @retval  None
 */
static void lock_schedule_store(void)
{
	flash_user_store(
			(int*)&lock_schedule_flash_idx,
			FLASH_ADR_LOCK_SCHEDULE, FLASH_SIZE_LOCK_SCHEDULE, BLOCK_SIZE_LOCK_SCHEDULE, (u8*)&lock_schedule_st
		);
}

/**
 * @func    lock_schedule_reset_to_default
 * @brief
 * @param   None
 * @retval  None
 */
void lock_schedule_reset_to_default(void)
{
	memset(lock_schedule_st, 0, sizeof(lock_schedule_st));
	foreach(i, NUMBER_BUTTON) {
		lock_schedule_st[i].active_day = ALL_DAY_OF_WEEK;
	}
	lock_schedule_store();
	DBG_LOCK_SCH_SEND_STR("\n lock_schedule_reset_to_default");
}

/**
 * @func    lock_schedule_restore
 * @brief
 * @param   None
 * @retval  None
 */
static void lock_schedule_restore(void)
{
	flash_user_get_flash_index(
		        (int*)&lock_schedule_flash_idx,
				FLASH_ADR_LOCK_SCHEDULE, FLASH_SIZE_LOCK_SCHEDULE, BLOCK_SIZE_LOCK_SCHEDULE, (u8*)&lock_schedule_st
			);
	flash_user_restore(lock_schedule_flash_idx,
				FLASH_ADR_LOCK_SCHEDULE, BLOCK_SIZE_LOCK_SCHEDULE, (u8*)&lock_schedule_st
			);
#ifdef LOCK_SCH_DBG_EN
	DBG_LOCK_SCH_SEND_STR("\n LOCK_SCH_INIT: \n");
	DBG_LOCK_SCH_SEND_INT(lock_schedule_flash_idx);
	DBG_LOCK_SCH_SEND_STR("\n");

	foreach(i, NUMBER_BUTTON) {
		DBG_LOCK_SCH_SEND_INT(lock_schedule_st[i].lock);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_INT(lock_schedule_st[i].en_schedule);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_HEX(lock_schedule_st[i].start_t_min);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_HEX(lock_schedule_st[i].stop_t_min);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_INT(lock_schedule_flash_idx);
		DBG_LOCK_SCH_SEND_STR("\n");
	}
#endif
	// Default
	foreach(i, NUMBER_BUTTON) {
		if(lock_schedule_st[i].lock > max2(true, false)) {
			lock_schedule_reset_to_default();
			break;
		}
	}
}

/**
 * @func    lock_schedule_init
 * @brief
 * @param   None
 * @retval  None
 */
void lock_schedule_init(void)
{
	lock_schedule_restore();
	current_lock_status_restore();
	DBG_LOCK_SCH_SEND_STR("\n *** \nLOCK restore and modify: ");
	foreach(i, NUMBER_BUTTON) {
		if(lock_schedule_st[i].lock == true) {
			if(lock_schedule_st[i].en_schedule == false) {
				lock_com_par.present_lock[i] = TS_LOCK;
			}
		}
		else {
			lock_com_par.present_lock[i] = TS_UNLOCK;
		}
		DBG_LOCK_SCH_SEND_STR("\nmodel_index: ");
		DBG_LOCK_SCH_SEND_INT(i);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_INT(lock_com_par.present_lock[i]);
	}
}

/**
 * @func    lock_schedule_update_present_status
 * @brief
 * @param   None
 * @retval  None
 */
int lock_schedule_update_present_status(u8 idx, lock_unlock_enum status)
{
	if(idx < NUMBER_BUTTON    \
			&& status < TS_LOCK_UNKNOWN) {
		lock_com_par.present_lock[idx] = status;
		return 0;
	}
	return -1;
}

/**
 * @func    lock_schedule_check_active_time_point
 * @brief
 * @param   None
 * @retval  None
 */
static void lock_schedule_check_active_time_point(u8 idx, u8 wday)
{
	if(wday >= INVALID_WEEK_DAY)
	{
		DBG_LOCK_SCH_SEND_STR("\n Week day error, can't handle");
		return;
	}
	u8 bit_pos = (wday == 0)?6:(wday - 1);
	if(idx < NUMBER_BUTTON) {
		u8 day_passed = false;
		if(((lock_schedule_st[idx].active_day >>  \
					(LM_WEEK_DAY_MAX - bit_pos))&1) == 1) {
			day_passed = true;
		}
		else {
			DBG_LOCK_SCH_SEND_STR("\n *** SCHEDULE NOT ACTIVE AT THIS DAY *** ");
		}

		DBG_LOCK_SCH_SEND_STR("\n mActive day: ");
		DBG_LOCK_SCH_SEND_HEX(lock_schedule_st[idx].active_day);

		DBG_LOCK_SCH_SEND_STR("\n current day: ");
		DBG_LOCK_SCH_SEND_INT(wday);

		if(day_passed == true) {
			if(lock_schedule_st[idx].start_t_min > lock_schedule_st[idx].stop_t_min) {
				if((lock_com_par.present_min >= lock_schedule_st[idx].start_t_min)  \
						|| (lock_com_par.present_min < lock_schedule_st[idx].stop_t_min)) {
					lock_com_par.set_lock_val[idx] = TS_LOCK;
					DBG_LOCK_SCH_SEND_STR("\n S1");
				} else {
					lock_com_par.set_lock_val[idx] = TS_UNLOCK;
					DBG_LOCK_SCH_SEND_STR("\n S2");
				}
			} else if(lock_schedule_st[idx].start_t_min < lock_schedule_st[idx].stop_t_min) {
				if((lock_com_par.present_min >= lock_schedule_st[idx].start_t_min)
						&& (lock_com_par.present_min < lock_schedule_st[idx].stop_t_min)) {
					lock_com_par.set_lock_val[idx] = TS_LOCK;
					DBG_LOCK_SCH_SEND_STR("\n S3");
				} else {
					lock_com_par.set_lock_val[idx] = TS_UNLOCK;
					DBG_LOCK_SCH_SEND_STR("\n S4");
				}

				DBG_LOCK_SCH_SEND_STR("\n Current: ");
				DBG_LOCK_SCH_SEND_INT(lock_com_par.present_min);

				DBG_LOCK_SCH_SEND_STR(", STA: ");
				DBG_LOCK_SCH_SEND_INT(lock_schedule_st[idx].start_t_min);

				DBG_LOCK_SCH_SEND_STR(", STO: ");
				DBG_LOCK_SCH_SEND_INT(lock_schedule_st[idx].stop_t_min);

			} else {
				lock_com_par.set_lock_val[idx] = TS_LOCK;
				DBG_LOCK_SCH_SEND_STR("\n S5");
			}
		}
		else
		{
			DBG_LOCK_SCH_SEND_STR("\n Always unlock in this day: ");
			DBG_LOCK_SCH_SEND_INT(wday);
			lock_com_par.set_lock_val[idx] = TS_UNLOCK;
		}
		// Active
		if((lock_com_par.present_lock[idx] != lock_com_par.set_lock_val[idx]) \
				|| (lock_com_par.present_min == lock_schedule_st[idx].start_t_min)  \
					|| (lock_com_par.present_min == lock_schedule_st[idx].stop_t_min))
		{
			lock_com_par.present_lock[idx] = lock_com_par.set_lock_val[idx];
			current_lock_status_store();
			DBG_LOCK_SCH_SEND_STR("\n CONTROL LOCK: ");
			DBG_LOCK_SCH_SEND_INT(idx);
			DBG_LOCK_SCH_SEND_STR(", ");
			DBG_LOCK_SCH_SEND_INT(lock_com_par.set_lock_val[idx]);
		}
	}
}

/**
 * @func    lock_schedule_check_lock_unlock
 * @brief
 * @param   None
 * @retval  None
 */
void lock_schedule_check_lock_unlock(void)
{
	u8 wday = timestamp_get_week_day();
	DBG_LOCK_SCH_SEND_STR("\n Wday: ");
	DBG_LOCK_SCH_SEND_INT(wday);

	foreach(i, NUMBER_BUTTON) {
		if(lock_schedule_st[i].lock == false)
		{
			lock_com_par.present_lock[i] = TS_UNLOCK;
		}
		else
		{
			if(lock_schedule_st[i].en_schedule == false) {
				lock_com_par.present_lock[i] = TS_LOCK;
				DBG_LOCK_SCH_SEND_STR("\n LOCK all time");
			}
			else {
				lock_schedule_check_active_time_point(i, wday);
			}
		}
	}
}

/**
 * @func    lock_schedule_task
 * @brief
 * @param   None
 * @retval  None
 */
void lock_schedule_task(void)
{
	u16 tmp = timestamp_get_minute_in_day();
	if(tmp != INVALID_MINUTE) {
		if(lock_com_par.is_ready_flag == false) {
			if(clock_time_exceed_s(0, 10)) {
				lock_com_par.is_ready_flag = true;
			}
			return;
		}
		if(tmp != lock_com_par.present_min)
		{
			DBG_LOCK_SCH_SEND_STR("\n ___TM: ");
			DBG_LOCK_SCH_SEND_INT(tmp);
			DBG_LOCK_SCH_SEND_STR(", ");
			DBG_LOCK_SCH_SEND_INT(lock_com_par.present_min);
			lock_com_par.present_min = tmp;
			lock_schedule_check_lock_unlock();
		}
	}
}

/**
 * @func    vd_lock_schedule_response
 * @brief
 * @param   None
 * @retval  None
 */
int vd_lock_schedule_response(u8 model_idx, u16 src_adr, u16 dst_adr)
{
	#ifdef  MD_RADAR_ENABLE
	u8 idx = cb_par->model_idx - 1;
	#else
	u8 idx = model_idx;
	#endif
	if(idx < NUMBER_BUTTON) {
		u8 size = sizeof(lock_schedule_t);
		u8 resp[size+1];
		resp[0] = VD_TS_LOCK_SCHEDULE;
		memcpy(&resp[1], (u8*)&lock_schedule_st[idx], size);
		mesh_tx_cmd_rsp(
				 VD_CONFIG_NODE_STATUS,
				 resp,
				 size+1,
				 ele_adr_primary + model_idx,
				 GATEWAY_UNICAST_ADDR,
				 0,
				 0
			 );
		DBG_LOCK_SCH_SEND_STR("\n vd_lock_schedule_response: ");
		DBG_LOCK_SCH_SEND_INT(lock_schedule_st[idx].en_schedule);
		DBG_LOCK_SCH_SEND_STR(", START - ");
		DBG_LOCK_SCH_SEND_INT(lock_schedule_st[idx].start_t_min);
		DBG_LOCK_SCH_SEND_STR(", STOP - ");
		DBG_LOCK_SCH_SEND_INT(lock_schedule_st[idx].stop_t_min);
	}
	return -1;
}


/**
 * @func    vd_lock_schedule_get
 * @brief
 * @param   None
 * @retval  None
 */
int vd_lock_schedule_get(u8* par, u8 par_len, mesh_cb_fun_par_t *cb_par)
{
	#ifdef  MD_RADAR_ENABLE
	u8 idx = cb_par->model_idx - 1;
	#else
	u8 idx = cb_par->model_idx;
	#endif
	if(idx < NUMBER_BUTTON) {
		return vd_lock_schedule_response(  \
					cb_par->model_idx, \
					ele_adr_primary+cb_par->model_idx, GATEWAY_UNICAST_ADDR
				 );
	}
	return -1;
}

/**
 * @func    vd_lock_schedule_set
 * @brief
 * @param   None
 * @retval  None
 */
void vd_lock_schedule_notify_led(u8 idx, u8 evt)
{
	switch(evt)
	{
		case LED_CONFIG_LOCK_SCHEDULE_ENABLE:
			if(idx < NUMBER_SCENE) {
				led_scene_push_normal_blink_led_cmd_to_fifo(1 << idx, 2);
			}
			else {
				led_blink_color(1 << (idx - NUMBER_SCENE),  \
						LED_COLOR_BLUE, 2, LAST_STATE_REFRESH_LED, 200);
			}
			break;

		case LED_CONFIG_LOCK_SCHEDULE_DISABLE:
			if(idx < NUMBER_SCENE) {
				led_scene_push_normal_blink_led_cmd_to_fifo(1 << idx, 1);
			}
			else {
				led_blink_color(1 << (idx - NUMBER_SCENE),  \
						LED_COLOR_BLUE, 1, LAST_STATE_REFRESH_LED, 200);
			}
			break;

		case LED_CONFIG_LOCK_SCHEDULE_FAIL:
			if(idx < NUMBER_SCENE) {
				led_scene_push_normal_blink_led_cmd_to_fifo(BACKUP_MASK_SCENE, 2);
			}
			else {
				led_blink_color(BACKUP_MASK_RL,  \
						LED_COLOR_RED, 2, LAST_STATE_REFRESH_LED, 200);
			}
			break;
	}
}

/**
 * @func    vd_lock_schedule_set
 * @brief
 * @param   None
 * @retval  None
 */
int vd_lock_schedule_set(u8* par, u8 par_len, mesh_cb_fun_par_t *cb_par)
{
#ifdef LOCK_SCH_DBG_EN
	DBG_LOCK_SCH_SEND_STR("\n vd_lock_schedule_set: ");
	DBG_LOCK_SCH_SEND_INT(cb_par->model_idx);
	DBG_LOCK_SCH_SEND_STR(", ");
	foreach(i, par_len)
	{
		DBG_LOCK_SCH_SEND_STR(" ");
		DBG_LOCK_SCH_SEND_BYTE(par[i]);
	}
#endif

	int err = -1;
	#ifdef  MD_RADAR_ENABLE
	u8 idx = cb_par->model_idx - 1;
	#else
	u8 idx = cb_par->model_idx;
	#endif
	u8 changed_flag = false;
	if(idx < NUMBER_BUTTON) {
		lock_schedule_t* p_lock_schedule = (lock_schedule_t*)par;
		p_lock_schedule->en_schedule &= 0x01;
		if(p_lock_schedule->lock == true) {
			if(p_lock_schedule->en_schedule == true) {
				if(p_lock_schedule->start_t_min <= MININUTE_RANGE_IN_DAY
						&& p_lock_schedule->stop_t_min <= MININUTE_RANGE_IN_DAY) {
					// Lock with schedule
					changed_flag = true;
					lock_schedule_st[idx].lock = true;
					lock_schedule_st[idx].en_schedule = true;
					lock_schedule_st[idx].start_t_min = p_lock_schedule->start_t_min;
					lock_schedule_st[idx].stop_t_min = p_lock_schedule->stop_t_min;
					lock_schedule_st[idx].active_day = p_lock_schedule->active_day;
					vd_lock_schedule_notify_led(idx, LED_CONFIG_LOCK_SCHEDULE_ENABLE);
					err = 0;
					DBG_LOCK_SCH_SEND_STR("\n Enable Lock Success");
					lock_schedule_check_lock_unlock();
				}
				else {
					vd_lock_schedule_notify_led(idx, LED_CONFIG_LOCK_SCHEDULE_FAIL);
					DBG_LOCK_SCH_SEND_STR("\n Lock time invalid");
				}
			}
			else
			{
				// Always lock
				changed_flag = true;
				lock_schedule_st[idx].lock = true;
				lock_schedule_st[idx].en_schedule = false;
				vd_lock_schedule_notify_led(idx, LED_CONFIG_LOCK_SCHEDULE_ENABLE);
				lock_com_par.present_lock[idx] = TS_LOCK;
				err = 0;
				DBG_LOCK_SCH_SEND_STR("\n Lock All time Schedule");
			}
		}
		else
		{
			// Unlock
			changed_flag = true;
			lock_schedule_st[idx].lock = false;
			lock_schedule_st[idx].en_schedule = false;
			vd_lock_schedule_notify_led(idx, LED_CONFIG_LOCK_SCHEDULE_DISABLE);
			lock_com_par.present_lock[idx] = TS_UNLOCK;
			err = 0;
			DBG_LOCK_SCH_SEND_STR("\n Disable Lock: ");
			DBG_LOCK_SCH_SEND_INT(idx);
		}
		if(changed_flag == true) {
			lock_schedule_store();
			DBG_LOCK_SCH_SEND_STR("\n --- LOCK SCH Store");
		}
#ifdef LOCK_SCH_DBG_EN
	DBG_LOCK_SCH_SEND_STR("\n LOCK_SCH: \n");
	foreach(i, NUMBER_BUTTON) {
		DBG_LOCK_SCH_SEND_INT(lock_schedule_st[i].lock);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_INT(lock_schedule_st[i].en_schedule);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_HEX(lock_schedule_st[i].start_t_min);
		DBG_LOCK_SCH_SEND_STR(", ");
		DBG_LOCK_SCH_SEND_HEX(lock_schedule_st[i].stop_t_min);
		DBG_LOCK_SCH_SEND_STR("\n");
	}
#endif
		vd_lock_schedule_response(idx, ele_adr_primary +  cb_par->model_idx, GATEWAY_UNICAST_ADDR);
	}
	return err;
}
