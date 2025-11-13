/*
 * timestamp.c
 *
 *  Created on: Feb 7, 2023
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/system_time.h"
#include "utilities.h"
#include "timestamp.h"
#include "standard/time.h"

#include "debug.h"
#ifdef  TIMESTAMP_DBG_EN
	#define DBG_TIMESTAMP_SEND_STR(x)     Dbg_sendString((s8*)x)
	#define DBG_TIMESTAMP_SEND_INT(x)     Dbg_sendInt(x)
	#define DBG_TIMESTAMP_SEND_HEX(x)     Dbg_sendHex(x)
	#define DBG_TIMESTAMP_SEND_BYTE(x)    Dbg_sendOneByteHex(x)
    #define DBG_TIMESTAMP_SEND_HEX32(x)   Dbg_sendHex32(x)
    #define DBG_TIMESTAMP_SEND_DWORD(x)   Dbg_sendDword(x)
#else
	#define DBG_TIMESTAMP_SEND_STR(x)
	#define DBG_TIMESTAMP_SEND_INT(x)
	#define DBG_TIMESTAMP_SEND_HEX(x)
	#define DBG_TIMESTAMP_SEND_BYTE(x)
    #define DBG_TIMESTAMP_SEND_HEX32(x)
    #define DBG_TIMESTAMP_SEND_DWORD(x)
#endif


/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

#define PAR_LEN_SET_TIMESTAMP       4

#define TIMESTAMP_CHECK_INTERVAL    (CLOCK_SYS_CLOCK_1S)


typedef struct {
	time_t current_time;
	time_t previous_time;
	u32    time_tick;
	BOOL   is_set_flag;
	u32    set_st_time;
}time_par_t;

time_par_t   time_par;

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/

#ifdef TIMESTAMP_DBG_EN
/**
 * @func   timestamp_debug
 * @brief
 * @param  None
 * @retval None
 */
static void timestamp_debug(time_t time)
{
	struct tm *tm_st;
	tm_st = std_gmtime(&time);

	DBG_TIMESTAMP_SEND_STR("\n ________\n SET_VALUE: ");
	DBG_TIMESTAMP_SEND_HEX32((u32)time);

	DBG_TIMESTAMP_SEND_STR("\n DATE: ");
	DBG_TIMESTAMP_SEND_INT(tm_st->tm_wday);
	DBG_TIMESTAMP_SEND_STR(" ");
	DBG_TIMESTAMP_SEND_INT(tm_st->tm_mday);
	DBG_TIMESTAMP_SEND_STR(" ");
	DBG_TIMESTAMP_SEND_INT(tm_st->tm_mon + 1);
	DBG_TIMESTAMP_SEND_STR(" ");
	DBG_TIMESTAMP_SEND_INT(tm_st->tm_year + 1900);

	DBG_TIMESTAMP_SEND_STR("\n TIME: ");
	DBG_TIMESTAMP_SEND_INT(tm_st->tm_hour);
	DBG_TIMESTAMP_SEND_STR(" ");
	DBG_TIMESTAMP_SEND_INT(tm_st->tm_min);
	DBG_TIMESTAMP_SEND_STR(" ");
	DBG_TIMESTAMP_SEND_INT(tm_st->tm_sec);
}
#endif

/**
 * @func   timestamp_get_second_in_day
 * @brief
 * @param  None
 * @retval None
 */
u32 timestamp_get_second_in_day(void)
{
	if(!time_par.is_set_flag) {
		if(!clock_time_exceed_s(
				time_par.set_st_time, SET_TIMESTAMP_MAX_INTERVAL_S)) {
			struct tm *tm_st;
			tm_st = std_gmtime(&time_par.current_time);
			return (u32)((tm_st->tm_hour*60 + tm_st->tm_min)*60 + tm_st->tm_sec);
		}
	}
	return INVALID_SEC;
}

/**
 * @func   timestamp_get_second_in_day
 * @brief
 * @param  None
 * @retval None
 */
u16 timestamp_get_minute_in_day(void)
{
	if(time_par.is_set_flag) {
		if(!clock_time_exceed_s(
				time_par.set_st_time, SET_TIMESTAMP_MAX_INTERVAL_S)) {
			struct tm *tm_st;
			tm_st = std_gmtime(&time_par.current_time);
			return (u16)(tm_st->tm_hour*60 + tm_st->tm_min);
		}
	}
	return INVALID_MINUTE;
}

/**
 * @func   timestamp_get_week_day
 * @brief
 * @param  None
 * @retval None
 */
u8 timestamp_get_week_day(void)
{
	if(time_par.is_set_flag) {
		if(!clock_time_exceed_s(
				time_par.set_st_time, SET_TIMESTAMP_MAX_INTERVAL_S)) {
			struct tm *tm_st;
			tm_st = std_gmtime(&time_par.current_time);
			return (u8)(tm_st->tm_wday);
		}
	}
	return INVALID_WEEK_DAY;
}

/**
 * @func   timestamp_device_is_have_time
 * @brief
 * @param  None
 * @retval None
 */
bool timestamp_device_is_have_time(void)
{
	return time_par.is_set_flag;
}

/**
 * @func   timestamp_send_query_time
 * @brief
 * @param  None
 * @retval None
 */
void timestamp_send_query_time(u16 src)
{
	// Send response
	timestamp_query_t  timestamp_query;
	timestamp_query.cmd   = VD_QUERY_TIMESTAMP;
	timestamp_query.dst_adr = ADR_ALL_NODES;
	mesh_tx_cmd_rsp(
			VD_CONFIG_NODE_STATUS, (u8*)&timestamp_query,
			sizeof(timestamp_query_t), src, GATEWAY_UNICAST_ADDR, 0, 0
		);
}

/**
 * @func   timestamp_proc
 * @brief
 * @param  None
 * @retval None
 */
void timestamp_proc()
{
	if(!time_par.is_set_flag)
		return;
    u32 clock_tmp = clock_time();
    u32 t_delta = (u32)(clock_tmp - time_par.time_tick);    // should be different from system_time_tick_
    if(t_delta >= TIMESTAMP_CHECK_INTERVAL){
        u32 interval_cnt = t_delta / TIMESTAMP_CHECK_INTERVAL;
        foreach(i, interval_cnt){
        	time_par.current_time++;
        }
        time_par.time_tick += interval_cnt * TIMESTAMP_CHECK_INTERVAL;
    }
    if(time_par.previous_time != time_par.current_time){
    	time_par.previous_time = time_par.current_time;
		#ifdef TIMESTAMP_DBG_EN
    	timestamp_debug(time_par.current_time);
		#endif
    }
}

/**
 * @func   timestamp_get
 * @brief
 * @param  None
 * @retval None
 */
void timestamp_get(u16 adr_dst, u16 adr_src)
{
	// Send response
	timestamp_rsp_t  timestamp_rsp;
	timestamp_rsp.cmd   = VD_CONFIG_TIMESTAMP;
	timestamp_rsp.value = time_par.current_time;
	mesh_tx_cmd_rsp(
			VD_CONFIG_NODE_STATUS, (u8*)&timestamp_rsp,
			sizeof(timestamp_rsp_t), adr_dst, adr_src, 0, 0
		);
}

/**
 * @func   timestamp_init
 * @brief
 * @param  None
 * @retval None
 */
int timestamp_set(u8 model_idx, u8* par, u8 par_len)
{
	if(par_len == PAR_LEN_SET_TIMESTAMP) {
		if(model_idx == 0) {
			time_par.is_set_flag = TRUE;
			time_par.current_time = 0;
			foreach(i, sizeof(time_t)) {
				time_par.current_time |= par[i] << (8*i);
			}
			time_par.time_tick = clock_time();
			time_par.set_st_time = clock_time_s();
			return 0;
		}
	}
	return -1;
}
