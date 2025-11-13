/*
 * fact_handle.c
 *
 *  Created on: Sep 30, 2024
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/system_time.h"
#include "vendor/mesh/user/config_board.h"
#include "vendor/mesh/user/utilities.h"
#include "fact_handle.h"

#include "vendor/mesh/user/debug.h"
#ifdef  FACT_DBG_EN
#define DBG_FACT_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_FACT_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_FACT_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_FACT_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_FACT_SEND_STR(x)
#define DBG_FACT_SEND_INT(x)
#define DBG_FACT_SEND_HEX(x)
#define DBG_FACT_SEND_BYTE(x)
#endif

typeFact_handle_update_result  pvFact_handle_update_result = NULL;

/******************************************************************************/
/*                                   DEFINE                                   */
/******************************************************************************/
#define FACT_RETRY_TIME_CNT          2

#define FACT_TOTAL_TIME_LENGTH       TIMER_70S
#define FACT_TEST_MESSAGE_CNT        5

#define FACT_PUBLISH_TIME_LEN        (FACT_TOTAL_TIME_LENGTH /FACT_TEST_MESSAGE_CNT)

#define FACT_POWER_INDEX             RF_POWER_P0p04dBm

// Compare Condition
#define FACT_IS_PASS(x)              ((x >= (FACT_TEST_MESSAGE_CNT - 3))?1:0)

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
typedef struct {
	uint8_t op;
	uint8_t cnt;
	uint8_t MAC[2];
}fact_test_rf_msg_t;

typedef struct {
	u8  enable;
	u32 st_time;
	u16 delay_time;
}fact_push_msg_delay_t;

static fact_push_msg_delay_t   fact_push_msg_delay = { .enable = 0 };

static fact_par_t fact_par = { .enable = 0, .done = 0 };
static fact_test_rf_msg_t  fact_rf_msg;

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/

/**
 * @func    ble_fact_callback_init
 * @brief   None
 * @param
 * @retval  None
 */
void ble_fact_callback_init(typeFact_handle_update_result func)
{
	if(func != NULL) {
		pvFact_handle_update_result = func;
	}
}

/**
 * @func    fact_is_activate
 * @brief   None
 * @param
 * @retval  None
 */
u8 fact_is_activate(void)
{
	return fact_par.enable;
}

/**
 * @func    fact_set
 * @brief   None
 * @param
 * @retval  None
 */
u8 fact_set(u8 act)
{
	if(act) {
		fact_par.enable = 1;
		fact_par.done = 0;
		fact_par.ac_st_time = clock_time_ms();
		fact_par.publish_cnt = fact_par.response_cnt = 0;
		fact_par.active_idx = MAX_U8;
		model_sig_cfg_s.ttl_def = 0;
		rf_set_power_level_index (FACT_POWER_INDEX);
	}
	else {
		fact_par.enable = 0;
		model_sig_cfg_s.ttl_def = TTL_DEFAULT;
		rf_set_power_level_index (my_rf_power_index);
	}
	return fact_par.enable;
}

/**
 * @func    fact_handle_config_response
 * @brief   None
 * @param
 * @retval  None
 */
int fact_handle_config_response(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par)
{

	DBG_FACT_SEND_STR("\n fact_handle_config_response: ");

	if(cb_par->adr_dst == ele_adr_primary) {
        #ifdef  FACT_DBG_EN
		DBG_FACT_SEND_STR("\n fact_handle_config_response: ");
		foreach(i, par_len) {
			DBG_FACT_SEND_STR(" ");
			DBG_FACT_SEND_BYTE(par[i]);
		}
        #endif
		fact_test_rf_msg_t *p_response =
				(fact_test_rf_msg_t*)&par[1];

		DBG_FACT_SEND_STR("\n MAC: ");
		foreach(i, par_len) {
			DBG_FACT_SEND_STR(" ");
			DBG_FACT_SEND_BYTE(p_response->MAC[i]);
		}
		u8 match_flag = 1;
		foreach(i,  sizeof(p_response->MAC)) {
			if(p_response->MAC[i] != tbl_mac[i]) {
				match_flag = 0;
				break;
			}
		}
		// Compare MAC
		if(match_flag) {
			DBG_FACT_SEND_STR("\n MAC match");
			if(p_response->cnt == fact_par.pub_cnt_before) {
				if(fact_par.active_idx != p_response->cnt) {
					fact_par.response_cnt++;
					fact_par.active_idx = p_response->cnt;
					DBG_FACT_SEND_STR("\n Index match, pass all condition: ");
					DBG_FACT_SEND_INT(fact_par.response_cnt);
				}
				else {
					DBG_FACT_SEND_STR("\n RETRY...");
				}
			}
		}
		else {
			DBG_FACT_SEND_STR("\n MAC not match");
		}
	}
	else {
		return -1;
	}
	return 0;
}

/**
 * @func    fact_main_function
 * @brief   None
 * @param
 * @retval  None
 */
void fact_update_result(void)
{
	test_rf_result_t test_rf_result;
	if(FACT_IS_PASS(fact_par.response_cnt)) {
		test_rf_result.result = FACT_PASS;
		DBG_FACT_SEND_STR("\n FACT_PASS");
	}
	else {
		test_rf_result.result = FACT_FAIL;
		DBG_FACT_SEND_STR("\n FACT_FAIL");
	}
	test_rf_result.rssi_value = 0;
	if(pvFact_handle_update_result != NULL) {
		pvFact_handle_update_result(test_rf_result.result);
	}
}

/**
 * @func    fact_push_message_delay
 * @brief   None
 * @param
 * @retval  None
 */
static void fact_push_message_delay(void)
{
	if(fact_push_msg_delay.enable == 1) {
		if(clock_time_exceed_ms(
				fact_push_msg_delay.st_time, fact_push_msg_delay.delay_time)) {
			fact_par.pub_cnt_before = fact_par.publish_cnt;
			fact_rf_msg.op = VD_FACT_TEST_RF;
			fact_rf_msg.cnt = fact_par.publish_cnt;
			memcpy(fact_rf_msg.MAC, tbl_mac, sizeof(fact_rf_msg.MAC));
			foreach(i, FACT_RETRY_TIME_CNT) {
				mesh_tx_cmd_rsp(
						 VD_CONFIG_NODE_STATUS,
						 (u8*)&fact_rf_msg,
						 sizeof(fact_test_rf_msg_t),
						 ele_adr_primary,
						 GATEWAY_UNICAST_ADDR,
						 0,
						 0
					 );
			}
			DBG_FACT_SEND_STR("\n PUBLISH TEST MESSAGE: ");
			DBG_FACT_SEND_INT(fact_par.publish_cnt);
			DBG_FACT_SEND_STR(", ");
			DBG_FACT_SEND_INT(fact_push_msg_delay.delay_time);
			fact_push_msg_delay.enable = 0;
		}
	}
}

/**
 * @func    fact_main_function
 * @brief   None
 * @param
 * @retval  None
 */
u8 fact_main_function(void)
{
	if(fact_par.enable)
	{
		if(fact_par.done) {
			if(clock_time_exceed_ms(fact_par.ac_st_time, TIMER_5Min)) {
				fact_set(0);
			}
			return fact_par.enable;
		}
		// Push message delay
		if(!FACT_IS_PASS(fact_par.response_cnt)) {
			fact_push_message_delay();
		}
		// Timeout
		if(clock_time_exceed_ms(fact_par.ac_st_time, FACT_TOTAL_TIME_LENGTH)) {
			fact_par.done = 1;
			fact_update_result();

			DBG_FACT_SEND_STR("\n fact_par.response_cnt: ");
			DBG_FACT_SEND_INT(fact_par.response_cnt);

			DBG_FACT_SEND_STR("\n Pass_Threshold: ");
			DBG_FACT_SEND_INT((FACT_TEST_MESSAGE_CNT*7)/10);
		}
		else {
			u32 next_step_time =   \
					(fact_par.publish_cnt * FACT_PUBLISH_TIME_LEN);
			if(clock_time_exceed_ms(fact_par.ac_st_time, next_step_time)) {

				if(fact_par.publish_cnt <= FACT_TEST_MESSAGE_CNT) {
					// Publish test message
					fact_push_msg_delay.st_time = clock_time_ms();
					fact_push_msg_delay.delay_time =   \
							rand()%(FACT_PUBLISH_TIME_LEN - TIMER_500MS);
					fact_push_msg_delay.enable = 1;
				}
				fact_par.publish_cnt++;
				DBG_FACT_SEND_STR("\n *** STEP PUB ***");
			}
		}
	}

#ifdef FACT_DBG_EN
	static u32 tmp = 0;
	static u16 cnt;
	if(clock_time_exceed_ms(tmp, TIMER_1S)) {
		tmp = clock_time_ms();
		DBG_FACT_SEND_STR("\n LOOP_CNT: ");
		DBG_FACT_SEND_INT(cnt++);
		DBG_FACT_SEND_STR(", ");
		DBG_FACT_SEND_INT(fact_par.enable);
		DBG_FACT_SEND_STR("\n TTL: ");
		DBG_FACT_SEND_INT(model_sig_cfg_s.ttl_def);
	}
#endif
	return fact_par.enable;
}
