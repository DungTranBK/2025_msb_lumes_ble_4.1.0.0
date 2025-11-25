/*
 * energy.c
 *
 *  Created on: Oct 4, 2023
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "vendor/common/lighting_model.h"
#include "proj_lib/ble/blt_config.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/system_time.h"
#include "vendor/common/vendor_model.h"
#include "vendor/common/mesh_node.h"
#include "fact/fact.h"
#include "utilities.h"
#include "config_board.h"
#include "periph/bl0906.h"
#include "flash_user.h"
#include "cycle_funcs.h"
#include "energy.h"

#include "debug.h"
#ifdef ENERGY_DBG_EN
	#define DBG_ENERGY_SEND_STR(x)   Dbg_sendString((s8*)x)
	#define DBG_ENERGY_SEND_INT(x)   Dbg_sendInt(x)
	#define DBG_ENERGY_SEND_HEX(x)   Dbg_sendHex(x)
	#define DBG_ENERGY_SEND_BYTE(x)  Dbg_sendOneByteHex(x)
    #define DBG_ENERGY_SEND_FLOAT(x) //Dbg_sendFloat(x)
    #define DBG_ENERGY_SEND_DWORD(x) Dbg_sendDword(x)
#else
	#define DBG_ENERGY_SEND_STR(x)
	#define DBG_ENERGY_SEND_INT(x)
	#define DBG_ENERGY_SEND_HEX(x)
	#define DBG_ENERGY_SEND_BYTE(x)
    #define DBG_ENERGY_SEND_FLOAT(x)
    #define DBG_ENERGY_SEND_DWORD(x)
#endif

#define P_ST_TRANS(idx, type)		(&light_res_sw[idx].trans[type])

typeEnergy_get_actual_relay_state pvEnergy_get_actual_relay_state = NULL;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
#define PERCENT_CURRENT_NEED_REPORT      (20)
#define PERCENT_VOLTAGE_NEED_REPORT      (10)
#define PERCENT_POWER_NEED_REPORT        (20)

#define TIME_SCAN_MEASURE                (2*1000)    // 5s
#define MIN_TIME_UPDATE_WH               (30*1000)   // 30s

#define DELTA_CURRENT_NEED_REPORT        50          // mA
#define DELTA_VOLTAGE_NEED_REPORT        50          // V
#define DELTA_ACTIVE_POWER_NEED_REPORT   10          // W
#define DELTA_ACTIVE_ENERGY_NEED_REPORT  50          // wh

#define OVER_CURRENT_THRESHOLD           5000        // mA


static float active_power_max[NUMBER_RL];

typedef struct {
	bool i_rms;
	bool v_rms;
	bool active_power;
	bool active_energy;
}energy_updated_flag_t;

typedef struct {
	float i_rms;
	float v_rms;
	float active_power;
	float active_energy;
	float power_factor;
}energy_value_t;

typedef struct {
	u32 query_t_start;
	u32 update_t_start;
	u32 update_wh_t_start;
}energy_time_t;

typedef struct {
	u8  enable;
	u32 start_t_ms;
	u32 wait_t_ms;
	u32 update_last_t_s;
}publish_energy_delay_par_t;


typedef struct {
	u8  enable;
	u32 start_t_ms;
	u32 wait_t_ms;
	u32 update_last_t_s;
	u8  src;
}publish_iup_pf_delay_par_t;

static energy_updated_flag_t energy_updated_flag[NUMBER_RL];
static energy_value_t energy_value[NUMBER_RL];
static energy_value_t energy_reported[NUMBER_RL];
static energy_time_t energy_time[NUMBER_RL];

static float total_active_energy[NUMBER_RL];
static float active_energy_power_on[NUMBER_RL];
static float active_energy_offset[NUMBER_RL];

static bool force_report_iup_when_st_change_flag[NUMBER_RL];
static bool fast_report_iup_flag[NUMBER_RL];

static publish_energy_delay_par_t publish_energy_delay_par[NUMBER_RL];
static publish_iup_pf_delay_par_t publish_iup_pf_delay_par[NUMBER_RL];

static bool report_power_on_flag[NUMBER_RL];

typedef struct {
	u32 iup_t_len_s;
	u32 power_t_len_s;
}publish_periodic_par_t;


static publish_periodic_par_t publish_periodic_par[NUMBER_RL];

static int flash_active_energy_idx = 0;
#define BLOCK_SIZE_ENERGY         (NUMBER_RL*sizeof(u32))
#define FLASH_SIZE_ENERGY        (FLASH_SECTOR_SIZE - BLOCK_SIZE_ENERGY)

/******************************************************************************/
/*                        PRIVATE FUNCTIONS DECLERATION                       */
/******************************************************************************/
static void update_input_to_report(u8 idx);
static bool energy_iup_is_need_report_by_delta(u8 idx);

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

/**
 * @func    restore_active_power
 * @brief
 * @param   None
 * @retval  None
 */
static void restore_active_power(void)
{
	u32 tmp[NUMBER_RL];
	flash_user_get_flash_index(
			(int*)&flash_active_energy_idx,
			FLASH_ADR_ENERGY, FLASH_SIZE_ENERGY, BLOCK_SIZE_ENERGY, (u8*)&active_energy_power_on
		);
	flash_user_restore(
			flash_active_energy_idx,
			FLASH_ADR_ENERGY, BLOCK_SIZE_ENERGY, (u8*)&tmp
		);
	foreach(i, NUMBER_RL) {
		if(tmp[i] == MAX_U32) {
			active_energy_power_on[i] = 0;
		}
		else {
			active_energy_power_on[i] = tmp[i];
		}
		DBG_ENERGY_SEND_STR("\n ...RESTORE ACT_PW: ");
		DBG_ENERGY_SEND_INT(i);
		DBG_ENERGY_SEND_STR(", ");
	    DBG_ENERGY_SEND_DWORD(active_energy_power_on[i]);
	}
}

/**
 * @func    store_active_power
 * @brief
 * @param   None
 * @retval  None
 */
void store_active_power(void)
{
	u32 tmp[NUMBER_RL];
	foreach(i, NUMBER_RL) {
		tmp[i] = (u32)total_active_energy[i];
	}
	flash_user_store(
			(int*)&flash_active_energy_idx,
			FLASH_ADR_ENERGY,
			FLASH_SIZE_ENERGY,
			BLOCK_SIZE_ENERGY,
			(u8*)&tmp
		);
}

/**
 * @func    energy_clean_all_active_power
 * @brief
 * @param   None
 * @retval  None
 */
void energy_clean_all_active_power(void)
{
	u32 tmp[NUMBER_RL];
	foreach(i, NUMBER_RL) {
		tmp[i] = 0;
	}
	flash_user_store(
				(int*)&flash_active_energy_idx,
				FLASH_ADR_ENERGY,
				FLASH_SIZE_ENERGY,
				BLOCK_SIZE_ENERGY,
				(u8*)&tmp
			);
}

/**
 * @func    Energy_init
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_handle_measurement_complete(u8 idx, m_type_enum m_type, float value)
{
//	DBG_ENERGY_SEND_STR("\n");
//	DBG_ENERGY_SEND_INT(idx);
//	DBG_ENERGY_SEND_STR(" - ");

	if(idx >= NUMBER_RL) {
		return;
	}
	u8 actual_st = G_ON;
	if(pvEnergy_get_actual_relay_state != NULL) {
		actual_st = pvEnergy_get_actual_relay_state(idx);
	}

	switch(m_type)
	{
		case TYPE_CURRENT:
		{
			energy_updated_flag[idx].i_rms = true;
			energy_value[idx].i_rms = value;

			if(actual_st == G_OFF && value < BL0906_CURRENT_OFFSET_MAX) {
				energy_value[idx].i_rms = 0;
			}

//			DBG_ENERGY_SEND_STR("\n Irms: ");
//			//DBG_ENERGY_SEND_FLOAT(energy_value[idx].i_rms);
//			DBG_ENERGY_SEND_INT((u16)energy_value[idx].i_rms);
			break;
		}
		case TYPE_VOLTAGE:
		{
			if(idx != 0) {
				break;
			}
			foreach(i, NUMBER_RL) {
				energy_updated_flag[i].v_rms = true;
				energy_value[i].v_rms = value;
			}

			if(fact_is_active() == true) {
				if(energy_value[0].v_rms >=  \
						VOLTAGE_RMS_MIN && energy_value[0].v_rms <= VOLTAGE_RMS_MAX) {
					fact_set_energy_status(true);
				}
			}

//			DBG_ENERGY_SEND_STR("\n Vrms: ");
//			// DBG_ENERGY_SEND_FLOAT(energy_value[idx].v_rms);
//			DBG_ENERGY_SEND_INT(energy_value[idx].v_rms);
			break;
		}
		case TYPE_ACTIVE_POWER:
		{
			energy_updated_flag[idx].active_power = true;
			energy_value[idx].active_power = value;
//			DBG_ENERGY_SEND_STR("\n active_power: ");
//			// DBG_ENERGY_SEND_FLOAT(energy_value[idx].active_power);
//			DBG_ENERGY_SEND_INT(energy_value[idx].active_power);

			if(value > active_power_max[idx]) {
				active_power_max[idx] = value;
			}


			break;
		}
		case TYPE_ACTIVE_ENERGY:
		{
			energy_updated_flag[idx].active_energy = true;
			energy_value[idx].active_energy = value;
//			DBG_ENERGY_SEND_STR("\n active_energy: ");
//			// DBG_ENERGY_SEND_FLOAT(energy_value[idx].active_energy);
//			DBG_ENERGY_SEND_INT(energy_value[idx].active_energy);
			break;
		}
	}
}

/**
 * @func    energy_publish_active_energy_delay_proc
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_publish_active_energy_delay_proc(void)
{
	foreach(i, NUMBER_RL) {
		if(publish_energy_delay_par[i].enable == true) {
			if(clock_time_exceed_ms(   \
					publish_energy_delay_par[i].start_t_ms, publish_energy_delay_par[i].wait_t_ms)) {
				bool need_save = false;
				if(total_active_energy[i] - energy_reported[i].active_energy > 1) {
					need_save = true;
				}
				else {
					DBG_ENERGY_SEND_STR("\n DON'T NEED STORE ACT_PW: ");
				}
				energy_reported[i].active_energy = total_active_energy[i];
				energy_time[i].update_wh_t_start = clock_time_ms();
				if(need_save == true) {
					store_active_power();
					DBG_ENERGY_SEND_STR("\n ...STORE ACT_PW: ");
					DBG_ENERGY_SEND_INT(i);
					DBG_ENERGY_SEND_STR(", ");
					DBG_ENERGY_SEND_DWORD((u32)energy_reported[i].active_energy);
				}
				st_transition_t *p_trans =  P_ST_TRANS(i + ELE_RELAY_OFFSET, ST_TRANS_LIGHTNESS);

				act_pw_rsp_t act_pw_rsp;
				act_pw_rsp.code = VD_ACTIVE_ENERGY_INFORMATION;
				act_pw_rsp.status = ((p_trans->present == LEVEL_OFF)?G_OFF:G_ON);
				act_pw_rsp.wh = (u32)energy_reported[i].active_energy;
				mesh_tx_cmd_rsp(
						VD_CONFIG_NODE_STATUS,(u8 *)&act_pw_rsp,sizeof(act_pw_rsp_t),
						ele_adr_primary + i + ELE_RELAY_OFFSET, GATEWAY_UNICAST_ADDR, 0, 0
					);
				// Clear flag
				publish_energy_delay_par[i].enable = false;
				publish_energy_delay_par[i].update_last_t_s = clock_time_s();

				DBG_ENERGY_SEND_STR("\n ************Report ENERGY: ");
				DBG_ENERGY_SEND_INT(i);
			}
		}
	}
}

/**
 * @func    energy_setup_publish_energy_delay
 * @brief
 * @param   None
 * @retval  None
 */
void energy_setup_publish_energy_delay(u8 idx, u32 wait_t_ms)
{
	if(publish_energy_delay_par[idx].enable == true) {
		return;
	}
	if(idx < NUMBER_RL) {
		publish_energy_delay_par[idx].enable = true;
		publish_energy_delay_par[idx].start_t_ms = clock_time_ms();
		publish_energy_delay_par[idx].wait_t_ms = wait_t_ms;
	}
}

/**
 * @func    energy_handle_delete_power_information
 * @brief
 * @param   None
 * @retval  None
 */
void energy_handle_delete_power_information(u8 idx)
{
	if(idx < NUMBER_RL) {
		active_energy_power_on[idx] = 0;
		total_active_energy[idx] = 0;
		active_energy_offset[idx] = 0;
		store_active_power();
		energy_setup_publish_energy_delay(idx, 0);
	}
}

/**
 * @func    energy_get_active_power_threshold
 * @brief
 * @param   None
 * @retval  None
 */
static float energy_get_active_power_threshold(u8 idx)
{
	float theshold = DELTA_ACTIVE_ENERGY_NEED_REPORT;
	if(idx < NUMBER_RL) {
		if(active_power_max[idx] < 100) {
			theshold = 50;
		}
		else if(active_power_max[idx] >= 100 && active_power_max[idx] < 300) {
			theshold = 150;
		}
		else if(active_power_max[idx] >= 300 && active_power_max[idx] < 500) {
			theshold = 400;
		}
		else if(active_power_max[idx] >= 500 && active_power_max[idx] < 1000) {
			theshold = 750;
		}
		else if(active_power_max[idx] >= 1000) {
			theshold = 1000;
		}
	}
	return theshold;
}

/**
 * @func    energy_calculator_kwh
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_calculator_kwh(u8 idx)
{
	if(idx >= NUMBER_RL) {
		return;
	}
	if(active_energy_offset[idx] != 0) {
		total_active_energy[idx] = active_energy_power_on[idx] +  \
				(energy_value[idx].active_energy - active_energy_offset[idx]);
		float threshold = energy_get_active_power_threshold(idx);

		DBG_ENERGY_SEND_STR("\n TOTAL - ACT_PW: ");
		DBG_ENERGY_SEND_INT(idx);
		DBG_ENERGY_SEND_STR(", ");
		DBG_ENERGY_SEND_DWORD(total_active_energy[idx]);


		if(abs(total_active_energy[idx] -  \
				energy_reported[idx].active_energy) >= threshold) {

			DBG_ENERGY_SEND_STR("\n >>> Over threshold Report Energy: ");

			if(clock_time_exceed_ms(energy_time[idx].update_wh_t_start, MIN_TIME_UPDATE_WH)) {
				energy_setup_publish_energy_delay(idx, MACRO_ACTIVE_ENERGY_REPORT_TIME);

				DBG_ENERGY_SEND_STR("1");
			}
		}
	}
	else {
		if(energy_value[idx].active_energy > 0) {
			active_energy_offset[idx] = energy_value[idx].active_energy;
			DBG_ENERGY_SEND_STR("\n ACT OFFSET: ");
			DBG_ENERGY_SEND_INT(idx);
			DBG_ENERGY_SEND_STR(", ");
			DBG_ENERGY_SEND_FLOAT(active_energy_offset[idx]);
		}
	}
}

/**
 * @func    update_input_to_report
 * @brief
 * @param   None
 * @retval  None
 */
static void update_input_to_report(u8 idx)
{
	if(idx < NUMBER_RL) {
		energy_reported[idx].i_rms =  energy_value[idx].i_rms;
		energy_reported[idx].v_rms = energy_value[idx].v_rms;
		energy_reported[idx].active_power = energy_value[idx].active_power;
	}
}

/**
 * @func    energy_publish_iup_pf_delay_proc
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_publish_iup_pf_delay_proc(void)
{
	foreach(i, NUMBER_RL) {
		if(publish_iup_pf_delay_par[i].enable == true) {
			if(clock_time_exceed_ms(   \
					publish_iup_pf_delay_par[i].start_t_ms, publish_iup_pf_delay_par[i].wait_t_ms)) {
				if(publish_iup_pf_delay_par[i].src == RP_SRC_DELTA) {
					if(energy_iup_is_need_report_by_delta(i) == false) {
						// Clear flag
						publish_iup_pf_delay_par[i].enable = false;
						publish_energy_delay_par[i].update_last_t_s = clock_time_s();
						continue;
					}
				}
				iup_pf_response_t iup_pf_response;
				iup_pf_response.code = VD_IUP_INFORMATION;
				// Update newest value
				update_input_to_report(i);
				iup_pf_response.i = (u16)energy_reported[i].i_rms;
				iup_pf_response.v = (u16)(energy_reported[i].v_rms/100); // mV to Vx10
				u32 tmp_power = (u32)(energy_reported[i].active_power);  // mW
				iup_pf_response.p[0] = (u8)tmp_power;
				iup_pf_response.p[1] = (u8)(tmp_power >> 8);
				iup_pf_response.p[2] = (u8)(tmp_power >> 16);
				mesh_tx_cmd_rsp(
						VD_CONFIG_NODE_STATUS, (u8 *)&iup_pf_response, sizeof(iup_pf_response_t),
						ele_adr_primary + i + ELE_RELAY_OFFSET, GATEWAY_UNICAST_ADDR, 0,0
					);
				DBG_ENERGY_SEND_STR("\n ************Report IUP: ");
				DBG_ENERGY_SEND_INT(i);

				DBG_ENERGY_SEND_STR("\n p - ");
				DBG_ENERGY_SEND_DWORD((u32)(energy_reported[i].active_power));


				// Clear flag
				publish_iup_pf_delay_par[i].enable = false;
				publish_energy_delay_par[i].update_last_t_s = clock_time_s();
			}
		}
	}
}

/**
 * @func    energy_setup_publish_iup_pf_delay
 * @brief
 * @param   None
 * @retval  None
 */
void energy_setup_publish_iup_pf_delay(u8 idx, u32 wait_t_ms, SourceReport_Enum src)
{
	if(publish_iup_pf_delay_par[idx].enable == true && wait_t_ms > (ENERGY_FAST_REPORT_MS*2)) {
		return;
	}
	if(idx < NUMBER_RL) {
		publish_iup_pf_delay_par[idx].enable = true;
		publish_iup_pf_delay_par[idx].start_t_ms = clock_time_ms();
		publish_iup_pf_delay_par[idx].wait_t_ms = wait_t_ms;
		publish_iup_pf_delay_par[idx].src = src;
		DBG_ENERGY_SEND_STR("\n---OK");
	}
}

/**
 * @func    is_satisfy_percent_change_condition
 * @brief
 * @param   None
 * @retval  None
 */
static bool is_satisfy_percent_change_condition(u8 idx, float delta, float value, u16 percent, m_type_enum type)
{
	u32 tmp = (u32)value;
	u32 tmp_delta = (u32)delta;
	if(tmp == 0) {
		if(pvEnergy_get_actual_relay_state != NULL) {
			if(pvEnergy_get_actual_relay_state(idx) == G_ON) {
				if(type == TYPE_ACTIVE_POWER) {
					if(tmp_delta >= MINIMUM_ACTIVE_POWER_MW) {
						return true;
					}
				}
				if(type == TYPE_CURRENT) {
					if(tmp_delta >= MINIMUM_ACTIVE_CURRENT_MA) {
						return true;
					}
				}
			}
		}
		return false;
	}
	u32 change_percent = (u32)((delta/value)*100);

	DBG_ENERGY_SEND_STR("\n is_satisfy_percent_change_condition: ");
	DBG_ENERGY_SEND_INT(change_percent);

	DBG_ENERGY_SEND_STR(", ");
	DBG_ENERGY_SEND_INT(delta);

	DBG_ENERGY_SEND_STR(", ");
	DBG_ENERGY_SEND_INT(value);

	if(change_percent >= percent) {
		DBG_ENERGY_SEND_STR("------------------------------ ");
		return true;
	}
	return false;
}

/**
 * @func    energy_iup_is_need_report_by_delta
 * @brief
 * @param   None
 * @retval  None
 */
static bool energy_iup_is_need_report_by_delta(u8 idx)
{
	if(is_satisfy_percent_change_condition(
			    idx,
				abs(energy_reported[idx].i_rms - energy_value[idx].i_rms),
					energy_reported[idx].i_rms,
						PERCENT_CURRENT_NEED_REPORT, TYPE_CURRENT)) {
		DBG_ENERGY_SEND_STR("\n Report current: ");
		DBG_ENERGY_SEND_INT(energy_reported[idx].i_rms);

		DBG_ENERGY_SEND_STR(", ");
		DBG_ENERGY_SEND_INT(energy_value[idx].i_rms);
		return true;
	}
	if(is_satisfy_percent_change_condition(
			        idx,
					abs(energy_reported[idx].v_rms - energy_value[idx].v_rms),
						energy_reported[idx].v_rms,
						PERCENT_VOLTAGE_NEED_REPORT, TYPE_VOLTAGE)) {
		DBG_ENERGY_SEND_STR("\n Report Voltage: ");
		DBG_ENERGY_SEND_INT(energy_reported[idx].v_rms);

		DBG_ENERGY_SEND_STR(", ");
		DBG_ENERGY_SEND_INT(energy_value[idx].v_rms);
		return true;
	}
	if(is_satisfy_percent_change_condition(
			    idx,
				abs(energy_reported[idx].active_power - energy_value[idx].active_power),
					energy_reported[idx].active_power,
						PERCENT_POWER_NEED_REPORT, TYPE_ACTIVE_POWER)) {

		DBG_ENERGY_SEND_STR("\n Report Active Power: ");
		DBG_ENERGY_SEND_INT(energy_reported[idx].active_power);

		DBG_ENERGY_SEND_STR(", ");
		DBG_ENERGY_SEND_INT(energy_value[idx].active_power);

		return true;
	}
	return false;
}

/**
 * @func    energy_check_publish_all_parameter
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_check_publish_all_parameter(u8 idx)
{
	if(idx >= NUMBER_RL) {
		return;
	}
	// Force report
	if(force_report_iup_when_st_change_flag[idx] == true) {
		if(fast_report_iup_flag[idx] == true) {
			energy_setup_publish_iup_pf_delay(idx, TIMER_4S + rand()%ENERGY_FAST_REPORT_MS, RP_SRC_CHANGE_ST);
		}
		else {
			energy_setup_publish_iup_pf_delay(idx, MACRO_IUP_REPORT_TIME, RP_SRC_CHANGE_ST);
		}
		update_input_to_report(idx);
		DBG_ENERGY_SEND_STR("\n FORCE Report IUP: ");
		DBG_ENERGY_SEND_INT(idx);
		force_report_iup_when_st_change_flag[idx] = false;
	}
	if(publish_iup_pf_delay_par[idx].enable == true) {
		return;
	}
	if(energy_iup_is_need_report_by_delta(idx) == true) {
		u32 delay_ms = MACRO_IUP_REPORT_TIME;
		if(report_power_on_flag[idx] == true) {
			report_power_on_flag[idx] = false;
			delay_ms = TIMER_1Min + rand()%TIMER_1Min;
		}
		energy_setup_publish_iup_pf_delay(idx, delay_ms, RP_SRC_DELTA);
	}
#if 0
	// Current
	if(is_satisfy_percent_change_condition(
			abs(energy_reported[idx].i_rms - energy_value[idx].i_rms),
				energy_reported[idx].i_rms,
					PERCENT_CURRENT_NEED_REPORT)) {
		energy_setup_publish_iup_pf_delay(idx, MACRO_IUP_REPORT_TIME, RP_SRC_DELTA);
		update_input_to_report(idx);
		DBG_ENERGY_SEND_STR("\n Update current: ");
		DBG_ENERGY_SEND_INT(idx);
		return;
	}
	// Voltage
	if(is_satisfy_percent_change_condition(
				abs(energy_reported[idx].v_rms - energy_value[idx].v_rms),
					energy_reported[idx].v_rms,
					PERCENT_VOLTAGE_NEED_REPORT)) {
		energy_setup_publish_iup_pf_delay(idx, MACRO_IUP_REPORT_TIME, RP_SRC_DELTA);
		update_input_to_report(idx);
		DBG_ENERGY_SEND_STR("\n Update voltage: ");
		DBG_ENERGY_SEND_INT(idx);
		return;
	}
	// Active power
	if(is_satisfy_percent_change_condition(
			abs(energy_reported[idx].active_power - energy_value[idx].active_power),
				energy_reported[idx].active_power,
					PERCENT_POWER_NEED_REPORT)) {
		DBG_ENERGY_SEND_STR("\n Update Active Power: ");
		DBG_ENERGY_SEND_INT(idx);
		DBG_ENERGY_SEND_STR(" - ");
		DBG_ENERGY_SEND_INT(energy_reported[idx].active_power);
		DBG_ENERGY_SEND_STR(", ");
		DBG_ENERGY_SEND_INT(energy_value[idx].active_power);
		energy_setup_publish_iup_pf_delay(idx, MACRO_IUP_REPORT_TIME, RP_SRC_DELTA);
		update_input_to_report(idx);
		return;
	}
#endif

}

/**
 * @func    energy_measurement_proc
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_measurement_proc(void)
{
    foreach(i, NUMBER_RL) {
		if(clock_time_exceed_ms(
				energy_time[i].query_t_start, TIME_SCAN_MEASURE)) {
			memset(&energy_updated_flag[i],  \
					false, sizeof(energy_updated_flag_t));
			u16 mask = BIT_MASK_CURRENT      | BIT_MASK_VOLTAGE       |
					   BIT_MASK_ACTIVE_POWER | BIT_MASK_ACTIVE_ENERGY;
			bl0906_measurenment_start(i, mask);
			energy_time[i].query_t_start = clock_time_ms();

			DBG_ENERGY_SEND_STR("\n ___QUERY: ");
			DBG_ENERGY_SEND_INT(i);
			DBG_ENERGY_SEND_STR(", ");
			DBG_ENERGY_SEND_HEX(mask);
		}
		// Get energy OK or not?
		u8* p_energy_updated_flag = (u8*)&energy_updated_flag[i];
		u8 cnt = 0;
		foreach(j, sizeof(energy_updated_flag_t)) {
			if(*(p_energy_updated_flag+j) == true)
				cnt++;
			else break;

		}
		if(cnt == sizeof(energy_updated_flag_t)) {
			memset(&energy_updated_flag[i],
					false, sizeof(energy_updated_flag_t));
			energy_check_publish_all_parameter(i);
			energy_calculator_kwh(i);
		}
    }
}

/**
 * @func    energy_handle_relay_state_change
 * @brief
 * @param   None
 * @retval  None
 */
void energy_handle_relay_state_change(u8 idx, u8 st, bool is_fast)
{
	if(idx < NUMBER_RL) {
		force_report_iup_when_st_change_flag[idx] = true;
		fast_report_iup_flag[idx] = is_fast;
		if(st == G_OFF) {
			active_power_max[idx] = 0;
			// Check report active power
			u32 tmp = total_active_energy[idx] -  \
					energy_reported[idx].active_energy;
			if(tmp >= MINIMUM_ACTIVE_ENERY_NEED_REPORT_AFTER_RL_OFF) {
				energy_setup_publish_energy_delay(idx, TIMER_30S + rand()%TIMER_30S);
			}
		}
	}
}

/**
 * @func    energy_publish_periodic_proc
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_publish_periodic_proc(void)
{
	foreach(i, NUMBER_RL) {
		if(clock_time_exceed_s(   \
				publish_iup_pf_delay_par[i].update_last_t_s, publish_periodic_par[i].iup_t_len_s)) {
			energy_setup_publish_iup_pf_delay(i, 0, RP_SRC_PERIODIC);
			publish_periodic_par[i].iup_t_len_s =  \
					IUP_PUBLISH_PERIODIC_DELAY_OFFSET + rand()%IUP_PUBLISH_PERIODIC_RANDOM_OFFSET;
			DBG_ENERGY_SEND_STR("\n IUP PUBLISH PERIODIC");
			publish_iup_pf_delay_par[i].update_last_t_s = clock_time_s();

		}
		if(clock_time_exceed_s(   \
				publish_energy_delay_par[i].update_last_t_s, publish_periodic_par[i].iup_t_len_s)) {
			energy_setup_publish_energy_delay(i, 0);
			publish_periodic_par[i].power_t_len_s =  \
					ACTIVE_ENERGY_PUBLISH_PERIODIC_DELAY_OFFSET + rand()%ACTIVE_ENERGY_PUBLISH_PERIODIC_RANDOM_OFFSET;
			DBG_ENERGY_SEND_STR("\n ACTIVE POWER PUBLISH PERIODIC");
			publish_energy_delay_par[i].update_last_t_s = clock_time_s();
		}
	}
}

/**
 * @func    energy_proc
 * @brief
 * @param   None
 * @retval  None
 */
void energy_proc(void)
{
    bl0906_proc();
	if(bl0906_is_correction_complete() == false) {
		return;
	}
    energy_publish_periodic_proc();
    energy_publish_active_energy_delay_proc();
    energy_publish_iup_pf_delay_proc();
    energy_measurement_proc();
}


/**
 * @func    energy_handle_cf_cnt_scale_overflow
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_handle_cf_cnt_scale_overflow(u8 idx)
{
	if(idx < NUMBER_RL) {
		active_energy_power_on[idx] = total_active_energy[idx];
		total_active_energy[idx] = 0;
		active_energy_offset[idx] = 0;
	}
}

/**
 * @func    energy_callback_init
 * @brief
 * @param   None
 * @retval  None
 */
void energy_callback_init(typeEnergy_get_actual_relay_state func)
{
	if(func != NULL) {
		pvEnergy_get_actual_relay_state = func;
	}
}

/**
 * @func    Energy_init
 * @brief
 * @param   None
 * @retval  None
 */
void energy_init(void)
{
	memset(&energy_time, 0, sizeof(energy_time));
	memset(&energy_updated_flag, false, sizeof(energy_updated_flag));
	memset(&energy_reported, 0, sizeof(energy_reported));
	memset(&energy_value, 0, sizeof(energy_value));
	bl0906_init(energy_handle_measurement_complete, energy_handle_cf_cnt_scale_overflow);
	// Active energy
	restore_active_power();
	foreach(i, NUMBER_RL) {
		energy_reported[i].active_energy = active_energy_power_on[i];
		total_active_energy[i] = 0;
		active_energy_offset[i] = 0;
		publish_energy_delay_par[i].enable = false;
		publish_iup_pf_delay_par[i].enable = false;
		publish_iup_pf_delay_par[i].src = RP_SRC_UNKNOWN;
		force_report_iup_when_st_change_flag[i] = false;

		publish_iup_pf_delay_par[i].update_last_t_s = 0;
		publish_energy_delay_par[i].update_last_t_s = 0;
		publish_periodic_par[i].iup_t_len_s =  \
				IUP_PUBLISH_PERIODIC_DELAY_OFFSET + rand()%IUP_PUBLISH_PERIODIC_RANDOM_OFFSET;
		publish_periodic_par[i].power_t_len_s =  \
				ACTIVE_ENERGY_PUBLISH_PERIODIC_DELAY_OFFSET + rand()%ACTIVE_ENERGY_PUBLISH_PERIODIC_RANDOM_OFFSET;
		active_power_max[i] = 0;
		report_power_on_flag[i] = true;
	}
}
// End file
