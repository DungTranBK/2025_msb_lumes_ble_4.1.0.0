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
    #define DBG_ENERGY_SEND_FLOAT(x) Dbg_sendFloat(x)
    #define DBG_ENERGY_SEND_DWORD(x) Dbg_sendDword(x)
#else
	#define DBG_ENERGY_SEND_STR(x)
	#define DBG_ENERGY_SEND_INT(x)
	#define DBG_ENERGY_SEND_HEX(x)
	#define DBG_ENERGY_SEND_BYTE(x)
    #define DBG_ENERGY_SEND_FLOAT(x)
    #define DBG_ENERGY_SEND_DWORD(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
#define TIME_SCAN_MEASURE                (5*1000)    // 5s
#define MIN_TIME_UPDATE_WH               (30*1000)   // 30s


#define DELTA_CURRENT_NEED_REPORT        50          // mA
#define DELTA_VOLTAGE_NEED_REPORT        50          // V
#define DELTA_ACTIVE_POWER_NEED_REPORT   10          // W
#define DELTA_ACTIVE_ENERGY_NEED_REPORT  50          // wh
#define DELTA_TEMPERATURE_NEED_REPORT    5           // celsius

#define OVER_CURRENT_THRESHOLD           5000        // mA

typedef struct {
	bool i_rms;
	bool v_rms;
	bool active_power;
	bool active_energy;
	bool power_factor;
	bool temperature;
}energy_updated_flag_t;

typedef struct {
	float i_rms;
	float v_rms;
	float active_power;
	float active_energy;
	float power_factor;
	float temperature;
}energy_value_t;

typedef struct {
	u32 query_t_start;
	u32 update_t_start;
	u32 update_wh_t_start;
}energy_time_t;

static energy_updated_flag_t energy_updated_flag;
static energy_value_t energy_value;
static energy_value_t energy_reported;
static energy_time_t energy_time;

static u8* p_energy_updated_flag = (u8*)&energy_updated_flag;

static float total_active_energy = 0;

static float active_energy_power_on = 0;
static int flash_active_energy_idx = 0;
#define FLASH_SIZE_ENERGY        (FLASH_SECTOR_SIZE - sizeof(u32))
#define BLOCK_SIZE_ENERGY         sizeof(u32)

/******************************************************************************/
/*                        PRIVATE FUNCTIONS DECLERATION                       */
/******************************************************************************/
static void update_input_to_report(void);

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
	u32 tmp;
	flash_user_get_flash_index(
			(int*)&flash_active_energy_idx,
			FLASH_ADR_ENERGY, FLASH_SIZE_ENERGY, BLOCK_SIZE_ENERGY, (u8*)&tmp
		);
	flash_user_restore(
			flash_active_energy_idx,
			FLASH_ADR_ENERGY, BLOCK_SIZE_ENERGY, (u8*)&tmp
		);
	if(tmp == MAX_U32) {
		active_energy_power_on = 0;
	}
	else {
		active_energy_power_on = tmp;
	}
	//active_energy_power_on = 0;
	DBG_ENERGY_SEND_STR("\n ...RESTORE ACT_PW: ");
    DBG_ENERGY_SEND_DWORD(active_energy_power_on);
}

/**
 * @func    store_active_power
 * @brief
 * @param   None
 * @retval  None
 */
static void store_active_power(void)
{
	u32 tmp = (u32)total_active_energy;
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
void energy_handle_measurement_complete(m_type_enum m_type, float value)
{
	switch(m_type)
	{
		case TYPE_CURRENT:
		{
			energy_updated_flag.i_rms = true;
			energy_value.i_rms = value;
			DBG_ENERGY_SEND_STR("\n Irms: ");
			DBG_ENERGY_SEND_FLOAT(energy_value.i_rms);
			break;
		}
		case TYPE_VOLTAGE:
		{
			energy_updated_flag.v_rms = true;
			energy_value.v_rms = value;
			DBG_ENERGY_SEND_STR("\n Vrms: ");
			DBG_ENERGY_SEND_FLOAT(energy_value.v_rms);
			break;
		}
		case TYPE_ACTIVE_POWER:
		{
			energy_updated_flag.active_power = true;
			energy_value.active_power = value;
			DBG_ENERGY_SEND_STR("\n active_power: ");
			DBG_ENERGY_SEND_FLOAT(energy_value.active_power);
			break;
		}
		case TYPE_ACTIVE_ENERGY:
		{
			energy_updated_flag.active_energy = true;
			energy_value.active_energy = value;
			DBG_ENERGY_SEND_STR("\n active_energy: ");
			DBG_ENERGY_SEND_FLOAT(energy_value.active_energy);
			break;
		}
		case TYPE_POWER_FACTOR:
		{
			energy_updated_flag.power_factor = true;
			energy_value.power_factor = value;
			DBG_ENERGY_SEND_STR("\n power_factor: ");
			DBG_ENERGY_SEND_FLOAT(energy_value.power_factor);
			break;
		}
		case TYPE_TEMPERATURE:
		{
			energy_updated_flag.temperature = true;
			energy_value.temperature = value;
			DBG_ENERGY_SEND_STR("\n temperature: ");
			DBG_ENERGY_SEND_FLOAT(energy_value.temperature);
			break;
		}
	}
}

/**
 * @func    energy_publish_active_energy_delay
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_publish_active_energy_delay(void)
{
	act_pw_rsp_t act_pw_rsp;
	act_pw_rsp.type = ENERGY_A;
	act_pw_rsp.wh = (u32)energy_reported.active_energy;
	act_pw_rsp.pf = (u8)energy_value.power_factor;

	// TODO
	mesh_tx_cmd_rsp(
			VD_NEMA_ENERGY_REPORT,
			(u8 *)&act_pw_rsp,  \
			sizeof(act_pw_rsp_t),
			ele_adr_primary,
			GATEWAY_UNICAST_ADDR,
			0,
			0
		);
	CycleFunc_remove(energy_publish_active_energy_delay);
	DBG_ENERGY_SEND_STR("\n ++++++++++++++ energy_publish_active_energy_delay");
}

/**
 * @func    energy_calculator_kwh
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_calculator_kwh(void)
{
	static float active_energy_offset = 0;
	if(active_energy_offset != 0) {
		total_active_energy = active_energy_power_on +  \
				(energy_value.active_energy - active_energy_offset);
		if(abs(total_active_energy -  \
				energy_reported.active_energy) >= DELTA_ACTIVE_ENERGY_NEED_REPORT) {
			if(clock_time_exceed_ms(energy_time.update_wh_t_start, MIN_TIME_UPDATE_WH))
			{
				CycleFunc_add(energy_publish_active_energy_delay,
						rand()%ACTIVE_ENERGY_RANDOM_OFFSET);
				energy_reported.active_energy = total_active_energy;
				energy_time.update_wh_t_start = clock_time_ms();
				store_active_power();
				DBG_ENERGY_SEND_STR("\n ...STORE ACT_PW: ");
				DBG_ENERGY_SEND_DWORD((u32)energy_reported.active_energy);
			}
		}
	}
	else {
		if(energy_value.active_energy > 0) {
			active_energy_offset = energy_value.active_energy;
			DBG_ENERGY_SEND_STR("\n ACT OFFSET: ");
			DBG_ENERGY_SEND_FLOAT(active_energy_offset);
		}
	}
}

/**
 * @func    energy_publish_iup_pf_delay
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_publish_iup_pf_delay(void)
{
	iup_pf_response_t iup_pf_response;
	iup_pf_response.type = ENERGY_I_U_P_PF;
	// Update newest value
	update_input_to_report();
	iup_pf_response.i = (u16)energy_reported.i_rms;
	iup_pf_response.v = (u16)energy_reported.v_rms;
	iup_pf_response.p = (u16)energy_reported.active_power;
	iup_pf_response.temp = (u8)energy_reported.temperature;

	// TODO
	mesh_tx_cmd_rsp(
			VD_NEMA_ENERGY_REPORT,
			(u8 *)&iup_pf_response,  \
			sizeof(iup_pf_response_t),
			ele_adr_primary,
			GATEWAY_UNICAST_ADDR,
			0,
			0
		);
	CycleFunc_remove(energy_publish_iup_pf_delay);
	DBG_ENERGY_SEND_STR("\n --------------- energy_publish_iup_pf_delay");
}

/**
 * @func    update_input_to_report
 * @brief
 * @param   None
 * @retval  None
 */
static void update_input_to_report(void)
{
	energy_reported.i_rms =  energy_value.i_rms;
	energy_reported.v_rms = energy_value.v_rms;
	energy_reported.active_power = energy_value.active_power;
	energy_reported.temperature = energy_value.temperature;
}

/**
 * @func    energy_check_publish_all_parameter
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_check_publish_all_parameter(void)
{
	// Current
	if(abs(energy_reported.i_rms -    \
			energy_value.i_rms) >= DELTA_CURRENT_NEED_REPORT) {
		CycleFunc_add(energy_publish_iup_pf_delay, MACRO_ENERGY_REPORT_TIME);
		update_input_to_report();
		DBG_ENERGY_SEND_STR("\n Update current");
		return;
	}
	// Voltage
	if(abs(energy_reported.v_rms -   \
			energy_value.v_rms) >= DELTA_VOLTAGE_NEED_REPORT) {
		CycleFunc_add(energy_publish_iup_pf_delay, MACRO_ENERGY_REPORT_TIME);
		update_input_to_report();
		DBG_ENERGY_SEND_STR("\n Update voltage");
		return;
	}
	// Active power
	if(abs(energy_reported.active_power -  \
			energy_value.active_power) >= DELTA_ACTIVE_POWER_NEED_REPORT) {
		CycleFunc_add(energy_publish_iup_pf_delay, MACRO_ENERGY_REPORT_TIME);
		update_input_to_report();
		DBG_ENERGY_SEND_STR("\n Update Active Power");
		return;
	}
	// Temperature
	if(abs(energy_reported.temperature -  \
			energy_value.temperature) >= DELTA_TEMPERATURE_NEED_REPORT) {
		CycleFunc_add(energy_publish_iup_pf_delay, MACRO_ENERGY_REPORT_TIME);
		update_input_to_report();
		DBG_ENERGY_SEND_STR("\n Update temperature");
		return;
	}
}

#if OVER_CURRENT_DETECT
/**
 * @func    energy_force_send_current_when_over_detected
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_force_send_current_when_over_detected(void)
{
	static u32 last_send_over_time = 0;
	if(clock_time_exceed_ms(last_send_over_time, TIMER_30S)
			|| (last_send_over_time == 0)) {
		energy_publish_iup_pf_delay();
		last_send_over_time = clock_time_ms();
	}
}

/**
 * @func    energy_check_over_current
 * @brief
 * @param   None
 * @retval  None
 */
static void energy_check_over_current(void)
{
	if(energy_value.i_rms >= OVER_CURRENT_THRESHOLD)
	{
		energy_force_send_current_when_over_detected();
		/*
		 * Force Turn off the power supply
		 */
		mesh_cmd_lightness_set_t lightness_set_st;
		lightness_set_st.lightness = LUM_OFF;
		lightness_set_st.tid = 0xFF;
		lightness_set_st.delay = 50;
		lightness_set_st.transit_t = 0x10;
		mesh_cmd_sig_lightness_set_manual(
				(u8*)&lightness_set_st, sizeof(mesh_cmd_lightness_set_t), 0
			);
		DBG_ENERGY_SEND_STR("\n OVER CURRENT DETECTION");
	}
}
#endif

/**
 * @func    energy_proc
 * @brief
 * @param   None
 * @retval  None
 */
void energy_proc(void)
{
    bl0906_proc();
	if(clock_time_exceed_ms(
			energy_time.query_t_start, TIME_SCAN_MEASURE)) {
		memset(&energy_updated_flag,  \
				false, sizeof(energy_updated_flag_t));
		u16 mask = BIT_MASK_CURRENT      | BIT_MASK_VOLTAGE       |
				   BIT_MASK_ACTIVE_POWER | BIT_MASK_ACTIVE_ENERGY |
				   BIT_MASK_POWER_FACTOR | BIT_MASK_TEMPERATURE;
		bl0906_measurenment_start(mask);
		energy_time.query_t_start = clock_time_ms();
		DBG_ENERGY_SEND_STR("\n ___QUERY: ");
		DBG_ENERGY_SEND_HEX(mask);
	}
    // Get energy OK or not?
	u8 cnt = 0;
	foreach(i, sizeof(energy_updated_flag_t)) {
		if(*(p_energy_updated_flag+i) == true)
			cnt++;
		else break;

	}
	if(cnt == sizeof(energy_updated_flag_t)) {
		DBG_ENERGY_SEND_STR("\n ENERY_GET_SS_OK");
		memset(&energy_updated_flag,
				false, sizeof(energy_updated_flag_t));
		energy_check_publish_all_parameter();
		energy_calculator_kwh();
#if OVER_CURRENT_DETECT
		energy_check_over_current();
#endif
		DBG_ENERGY_SEND_STR("\n TOTAL - ACT_PW: ");
	    DBG_ENERGY_SEND_DWORD(total_active_energy);
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
	memset(&energy_time, 0, sizeof(energy_time_t));
	memset(&energy_updated_flag, false, sizeof(energy_updated_flag_t));
	memset(&energy_reported, 0, sizeof(energy_value_t));
	memset(&energy_value, 0, sizeof(energy_value_t));
	bl0906_init(energy_handle_measurement_complete);
	// Active energy
	restore_active_power();
	energy_reported.active_energy = active_energy_power_on;
}
// End file
