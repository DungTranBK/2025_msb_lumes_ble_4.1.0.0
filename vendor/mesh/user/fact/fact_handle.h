/*
 * fact_handle.h
 *
 *  Created on: Sep 30, 2024
 *      Author: DungTranBK
 */

#ifndef FACT_HANDLE_H_
#define FACT_HANDLE_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

/******************************************************************************/
/*                              EXPORT TYPE                                   */
/******************************************************************************/

typedef struct {
	u8  enable;
	u32 ac_st_time;
	u8  step;
	u8  total_retry_cnt;
	u8  publish_cnt;
	u8  pub_cnt_before;
	u8  response_cnt;
	u8  active_idx;
	u8  done;
}fact_par_t;

typedef struct {
	uint8_t  result;
	uint16_t rssi_value;
}test_rf_result_t;

enum {
	FACT_PASS,
	FACT_FAIL,
};

typedef void  (*typeFact_handle_update_result)(bool result);

/******************************************************************************/
/*                              EXPORT FUNCTION                               */
/******************************************************************************/
void ble_fact_callback_init(typeFact_handle_update_result func);
u8 fact_is_activate(void);
u8 fact_set(u8 act);
u8 fact_main_function(void);
int fact_handle_config_response(u8 *par, int par_len, mesh_cb_fun_par_t *cb_par);

#endif /* FACT_HANDLE_H_ */
