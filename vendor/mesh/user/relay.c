/*
 * relay.c
 *
 *  Created on: Aug 21, 2024
 *      Author: DungTranBK
 */

#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/system_time.h"
#include "vendor/common/light.h"
#include "periph/bl0906.h"
#include "energy.h"
#include "fact/fact.h"
#include "cycle_funcs.h"
#include "flash_user.h"
#include "utilities.h"
#include "sw_config.h"
#include "led.h"
#include "user_irq.h"
#include "relay.h"

#include "debug.h"
#ifdef  RELAY_DBG_EN
#define DBG_RELAY_SEND_STR(x)    Dbg_sendString((s8*)x)
#define DBG_RELAY_SEND_INT(x)    Dbg_sendInt(x)
#define DBG_RELAY_SEND_HEX(x)    Dbg_sendHex(x)
#define DBG_RELAY_SEND_DWORD(x)  Dbg_sendDword(x)
#else
#define DBG_RELAY_SEND_STR(x)
#define DBG_RELAY_SEND_INT(x)
#define DBG_RELAY_SEND_HEX(x)
#define DBG_RELAY_SEND_DWORD(x)
#endif

typeRL_handle_relay_state_change pvRL_handle_relay_state_change = NULL;

#define P_ST_TRANS(idx, type)		(&light_res_sw[idx].trans[type])

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
typedef struct {
	u16  hold_t_on[NUMBER_RL];
	u16  hold_t_off[NUMBER_RL];
	u16  RLS_TARGET;
	u16  RLS_PRESENT;
	u16  RLS_SET;
	u16  RLS_ACTUAL;
	u8   relay_module_st;
	bool done_on_off_control_flag;
	u16  half_period_of_grid;
	u32  md_busy_st_time;
	u32  control_relay_st_time;
	u32  check_power_type_st_time;
	u32  wait_zero_point_st_time;
	bool control_when_timeout;
	u16  isr_cnt;
	src_control_enum src_control[NUMBER_RL];
}relay_para_t;

static relay_para_t relay_para;

typedef struct {
	u8 chn;
	u8 state;
	u8 step;
}relay_active_t;

typedef struct
{
	u16 pin_on;
	u16 pin_off;
}control_pin_t;

typedef struct
{
	bool enable;
	u32  start_wait_time_t;
	u32  time_length;
}store_relay_st_delay_t;

#if CACULATOR_FREQ_EN
typedef struct
	{
		u32 last_irq_t_ms;
		u32 start_t_ms;
		u8  freq;
		u8  cnt_50HZ;
		u8  cnt_60HZ;
		bool is_calculating;
		u8  cnt;
	}caculator_grid_freq_t;

static caculator_grid_freq_t caculator_grid_freq =
	{
		.last_irq_t_ms = 0,
		.start_t_ms = 0,
		.freq = 50,
		.cnt = 0,
		.is_calculating = true,
		.cnt_50HZ = 0,
		.cnt_60HZ = 0,
	};
#endif

static relay_active_t  relay_active_st = { \
		.chn = NO_RL_CHN_ACTIVE, .state = 0xFF, .step = 0};

static uint32_t onRelayStartTime[NUMBER_RL] = {0};
static bool     onRelayFlag[NUMBER_RL]      = {false};

static u16 control_pin[NUMBER_RL] = CONTROL_PIN_ARR;
static store_relay_st_delay_t store_relay_st_delay = {.enable = false};

typedef struct {
	u32  total_seconds[NUMBER_RL];
	u32  total_seconds_stored[NUMBER_RL];
	u32  check_start_t_ms[NUMBER_RL];
	bool enable_calculator[NUMBER_RL];

	u32  response_last_t_s[NUMBER_RL];
	u32  interval_t_s[NUMBER_RL];

}relay_on_time_par_t;

static relay_on_time_par_t relay_on_time_par;

static int flash_relay_state_idx = FLASH_INDEX_DEFAULT;

#define BLOCK_SIZE_RELAY_STATE    sizeof(relay_para.RLS_TARGET)
#define FLASH_SIZE_RELAY_STATE    (FLASH_SECTOR_SIZE - BLOCK_SIZE_RELAY_STATE)


static int flash_relay_on_time_idx = FLASH_INDEX_DEFAULT;

#define BLOCK_SIZE_RELAY_ON_TIME    (sizeof(u32)*NUMBER_RL)
#define FLASH_SIZE_RELAY_ON_TIME    (FLASH_SECTOR_SIZE - BLOCK_SIZE_RELAY_ON_TIME)


#define CHECK_RELAY_ON_TIME_INTERVAL_MS         TIMER_1Min
#define STORE_TOTAL_RELAY_ON_TIME_INTERVAL_MS   TIMER_30Min

static bool relay_power_on_control[NUMBER_RL];

/******************************************************************************/
/*                        PRIVATE FUNCTIONS DECLERATION                       */
/******************************************************************************/
static void relay_map_set_and_target_status(void);
static void relay_handle_switch_change_mode(u8 btn_id, u8 mode);
static u8 relay_get_actual_state(u8 idx);
static void relay_clean_relay_on_time(void);

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

/**
 * @func    relay_handle_fact_status
 * @brief
 * @param
 * @retval  None
 */
void relay_handle_fact_status(bool is_pass)
{
	if(is_pass == true) {
		relay_clean_relay_on_time();
		energy_clean_all_active_power();
	}
}

/**
 * @func    relay_store_total_relay_on_time
 * @brief
 * @param   None
 * @retval  None
 */
void relay_store_total_relay_on_time(void)
{
	bool need_store = false;
	foreach(i, NUMBER_RL) {
		if(relay_on_time_par.total_seconds[i] != relay_on_time_par.total_seconds_stored[i]) {
			need_store = true;
			break;
		}
	}

	DBG_RELAY_SEND_STR("\n ...Store total relay on time: ");


	if(need_store == true) {
		flash_user_store(  \
				(int*)&flash_relay_on_time_idx,
				FLASH_ADR_RELAY_ON_TIME, FLASH_SIZE_RELAY_ON_TIME,
				BLOCK_SIZE_RELAY_ON_TIME,(u8*)&relay_on_time_par.total_seconds \
			);
		foreach(i, NUMBER_RL) {
			relay_on_time_par.total_seconds_stored[i] = relay_on_time_par.total_seconds[i];
		}
		DBG_RELAY_SEND_STR("1");
	}
}


/**
 * @func    relay_store_total_relay_on_time_delay
 * @brief
 * @param   None
 * @retval  None
 */
static void relay_store_total_relay_on_time_delay(void)
{
	relay_store_total_relay_on_time();
	CycleFunc_remove(relay_store_total_relay_on_time_delay);
}

/**
 * @func    relay_response_total_relay_on_time
 * @brief
 * @param
 * @retval  None
 */
void relay_response_total_relay_on_time(u8 idx)
{
	if(idx < ELE_CNT) {
		relay_on_time_rsp_t relay_on_time_rsp;
		relay_on_time_rsp.code = VD_TOTAL_RELAY_ON_TIME_INFORMATION;
		relay_on_time_rsp.on_time = (u32)relay_on_time_par.total_seconds[idx];
		mesh_tx_cmd_rsp(
				VD_CONFIG_NODE_STATUS,
				(u8 *)&relay_on_time_rsp,
				sizeof(relay_on_time_rsp_t),
				ele_adr_primary + idx + ELE_RELAY_OFFSET,
				GATEWAY_UNICAST_ADDR,
				0,
				0
			);
		// Save after updating results
		relay_store_total_relay_on_time();
	}
}


/**
 * @func    relay_handle_delete_relay_on_time
 * @brief
 * @param   None
 * @retval  None
 */
void relay_handle_delete_relay_on_time(u8 idx)
{
	if(idx < NUMBER_RL) {
		relay_on_time_par.total_seconds[idx] = 0;
		relay_store_total_relay_on_time();
		relay_response_total_relay_on_time(idx);
	}
}

/**
 * @func    relay_restore_relay_on_time
 * @brief
 * @param   None
 * @retval  None
 */
static void relay_restore_total_relay_on_time(void)
{
	flash_user_get_flash_index(  \
			(int*)&flash_relay_on_time_idx, FLASH_ADR_RELAY_ON_TIME, \
			FLASH_SIZE_RELAY_ON_TIME, BLOCK_SIZE_RELAY_ON_TIME, (u8*)&relay_on_time_par.total_seconds \
		);
	flash_user_restore(
			flash_relay_on_time_idx, FLASH_ADR_RELAY_ON_TIME, BLOCK_SIZE_RELAY_ON_TIME,(u8*)&relay_on_time_par.total_seconds
		);
	foreach(i, NUMBER_RL) {
		if(relay_on_time_par.total_seconds[i] == MAX_U32) {
			relay_on_time_par.total_seconds[i] = 0;
		}
		relay_on_time_par.total_seconds_stored[i] = relay_on_time_par.total_seconds[i];
	}


	DBG_RELAY_SEND_STR("\n ...Restore total relay on time: ");
#ifdef  RELAY_DBG_EN
	foreach(i, NUMBER_RL) {
		DBG_RELAY_SEND_INT(i);
		DBG_RELAY_SEND_STR(", ");
		DBG_RELAY_SEND_DWORD(relay_on_time_par.total_seconds[i]);
		if(i < NUMBER_RL-1) {
			DBG_RELAY_SEND_STR(" - ");
		}
	}
#endif
}

/**
 * @func    relay_clean_relay_on_time
 * @brief
 * @param   None
 * @retval  None
 */
static void relay_clean_relay_on_time(void)
{
	foreach(i, NUMBER_RL) {
		relay_on_time_par.total_seconds[i] = 0;
	}
	relay_store_total_relay_on_time();
}

/**
 * @func    relay_store_target_state
 * @brief
 * @param   None
 * @retval  None
 */
static void relay_store_target_state(void)
{
	flash_user_store(  \
			(int*)&flash_relay_state_idx,
			FLASH_ADR_RELAY_STATE, FLASH_SIZE_RELAY_STATE,
			BLOCK_SIZE_RELAY_STATE,(u8*)&relay_para.RLS_TARGET  \
		);
}

/**
 * @func    relay_restore_target_state
 * @brief
 * @param   None
 * @retval  None
 */
void relay_restore_target_state(void)
{
	flash_user_get_flash_index(  \
			(int*)&flash_relay_state_idx, FLASH_ADR_RELAY_STATE, \
			FLASH_SIZE_RELAY_STATE, BLOCK_SIZE_RELAY_STATE, (u8*)&relay_para.RLS_TARGET \
		);
	flash_user_restore(
			flash_relay_state_idx, FLASH_ADR_RELAY_STATE, BLOCK_SIZE_RELAY_STATE,(u8*)&relay_para.RLS_TARGET
		);
	if(relay_para.RLS_TARGET == MAX_U16) {
		relay_para.RLS_TARGET = G_OFF;
	}
	foreach(i, NUMBER_RL) {
		if(sw_config_st.switch_mode[i] == MOMENTORY_SWITCH_TYPE) {
			relay_para.RLS_TARGET &= (~(1 << i));
		}
		else {
			if(sw_config_st.power_on_state[i] == POWER_UP_ON) {
				relay_para.RLS_TARGET |= (1 << i);
			}
			else if(sw_config_st.power_on_state[i] == POWER_UP_OFF) {
				relay_para.RLS_TARGET &= (~(1 << i));
			}
		}
	}
    // Check Valid
	relay_para.RLS_TARGET &= BACKUP_MASK_RL;
	relay_para.RLS_PRESENT = (~relay_para.RLS_TARGET)&BACKUP_MASK_RL;
	relay_para.RLS_SET = relay_para.RLS_TARGET;
	relay_para.RLS_ACTUAL = 0;
	// Common
	foreach(i, NUMBER_RL) {
		u16 model_idx = i + ELE_RELAY_OFFSET;
		st_transition_t *p_trans =  P_ST_TRANS(model_idx, ST_TRANS_LIGHTNESS);
		if(((relay_para.RLS_TARGET >> i)&0x01) == G_ON){
			p_trans->present = p_trans->target = LEVEL_MAX;
		}
		else {
			p_trans->present = p_trans->target = LEVEL_OFF;
		}
	}
	DBG_RELAY_SEND_STR("\n relay_restore_target_state");
}

/**
 * @func    relay_handle_store_delay
 * @brief
 * @param   None
 * @retval  None
 */
static void relay_handle_store_delay(void)
{
	if(store_relay_st_delay.enable == true) {
		if(clock_time_exceed_ms(  \
				store_relay_st_delay.start_wait_time_t, store_relay_st_delay.time_length)) {
			relay_store_target_state();
			store_relay_st_delay.enable = false;
			DBG_RELAY_SEND_STR("\n ...STORE RELAY STATE");
		}
	}
}

/**
 * @func    relay_setup_store_state_delay
 * @brief
 * @param   None
 * @retval  None
 */
static void relay_setup_store_state_delay(void)
{
	store_relay_st_delay.enable = true;
	store_relay_st_delay.start_wait_time_t = clock_time_ms();
	store_relay_st_delay.time_length = STORE_RELAY_STATE_TIME_LEN_MS;
	DBG_RELAY_SEND_STR("\n ...SETUP->STORE RELAY STATE");
}

/**
 * @func   get_power_type_when_detect_timeout
 * @brief
 * @param  None
 * @retval None
 */
static void relay_normal_on(u8 idx)
{
	gpio_write(control_pin[idx], RL_ON);
}

/**
 * @func   relay_normal_off
 * @brief
 * @param  None
 * @retval None
 */
static void relay_normal_off(u8 idx)
{
	gpio_write(control_pin[idx], RL_OFF);
}

/**
 * @func   RL_ChangeRelayStateWhenTimerOV (For RTC Timer)
 * @brief  Interrupt server of RTC module, used to turn on/off
 *         the relay at zero point
 * @param  None
 * @retval None
 */
void relay_handle_control_when_timer_match(void)
{
	u8 idx = relay_active_st.chn;
#if MAP_INPUT_OUTPUT_EN
	if(sw_config_st.map_input[relay_active_st.chn] != RL_ID_UNMAP) {
		idx = sw_config_st.map_input[relay_active_st.chn] - 1;
	}
#endif
	timer1_disable();
	if(relay_para.relay_module_st == RELAY_BUSY) {
		if(idx < NUMBER_RL) {
			if(sw_config_st.switch_mode[relay_active_st.chn] == LIGHTING_SWITCH_TYPE) {
				relay_normal_on(idx);
			}
			else {
				(relay_active_st.state == G_ON)? (relay_normal_on(idx)):(relay_normal_off(idx));
			}
		}
		relay_para.done_on_off_control_flag = CONTROL_SUCCESS;
		relay_para.relay_module_st = RELAY_IDLE;
	}
}


/**
 * @func   RL_handleZeroSync
 * @brief
 * @param  None
 * @retval None
 */
void RL_handleZeroSync(void)
{
	if(relay_para.control_when_timeout == false) {
		relay_para.control_when_timeout = true;
		timer1_enable();
	}
#if CACULATOR_FREQ_EN
	if(caculator_grid_freq.is_calculating == true)
	{
		if(clock_time_exceed_ms(caculator_grid_freq.last_irq_t_ms, TIMER_5MS))
		{
			caculator_grid_freq.cnt++;
			caculator_grid_freq.last_irq_t_ms = clock_time_ms();
		}
		if((clock_time_get_elapsed_time   \
				(caculator_grid_freq.start_t_ms) >= TIMER_1S)
				&& (caculator_grid_freq.cnt != 0)) {

			caculator_grid_freq.freq = caculator_grid_freq.cnt;

			DBG_RELAY_SEND_STR("\n CNT: ");
			DBG_RELAY_SEND_INT(caculator_grid_freq.cnt);

			if(caculator_grid_freq.freq < 55) {
				relay_para.half_period_of_grid = 10000;
				caculator_grid_freq.cnt_50HZ++;
				caculator_grid_freq.cnt_60HZ = 0;
			}
			else {
				relay_para.half_period_of_grid = 8000;
				caculator_grid_freq.cnt_60HZ++;
				caculator_grid_freq.cnt_50HZ = 0;
			}

//			DBG_RELAY_SEND_STR("\n CNT_50: ");
//			DBG_RELAY_SEND_INT(caculator_grid_freq.cnt_50HZ);
//
//			DBG_RELAY_SEND_STR("\n CNT_60: ");
//			DBG_RELAY_SEND_INT(caculator_grid_freq.cnt_60HZ);

			if(caculator_grid_freq.cnt_50HZ >= DETECT_FREQ_CNT_DEF   \
					|| caculator_grid_freq.cnt_60HZ >= DETECT_FREQ_CNT_DEF) {
				caculator_grid_freq.is_calculating = false;
			}
			caculator_grid_freq.start_t_ms = clock_time_ms();
			caculator_grid_freq.cnt = 0;
		}
	}
#endif
}

/**
 * @func    relay_callback_init
 * @brief
 * @param
 * @retval  None
 */
void relay_callback_init(typeRL_handle_relay_state_change func)
{
	if(func != NULL) {
		pvRL_handle_relay_state_change = func;
	}
}

/**
 * @func    relay_deinit_after_fact
 * @brief
 * @param
 * @retval  None
 */
void relay_deinit_after_fact(void)
{
	relay_restore_target_state();
}

/**
 * @func    relay_report_relay_on_time_init
 * @brief
 * @param
 * @retval  None
 */
void relay_report_relay_on_time_init(void)
{
	foreach(i, NUMBER_RL) {
		relay_on_time_par.response_last_t_s[i] = 0;
		relay_on_time_par.interval_t_s[i] =   \
				RELAY_ON_TIME_PUBLISH_TIME_PW_ON + rand()%RELAY_ON_TIME_PUBLISH_RANDOM_TIME_PW_ON;
	}
}

/**
 * @func    relay_force_control
 * @brief
 * @param
 * @retval  None
 */
void relay_force_control(u8 idx, u8 st)
{
	if(idx < NUMBER_RL) {
		relay_set_target_state(idx, st, SRC_DEVICE, false);
	}
}

/**
 * @func    relay_init
 * @brief
 * @param
 * @retval  None
 */
void relay_init(void)
{
	foreach(i, NUMBER_RL) {
		// Pin on
		gpio_set_func(control_pin[i], AS_GPIO);
		gpio_set_output_en(control_pin[i], 1);
		gpio_set_input_en(control_pin[i], 0);
		gpio_write(control_pin[i], RL_OFF);
		// Variable
		relay_para.src_control[i] = SRC_DEVICE;
		relay_power_on_control[i] = true;
	}
	relay_restore_target_state();
#if !CACULATOR_FREQ_EN
	relay_para.half_period_of_grid = 10000;
#endif
	relay_para.relay_module_st = RELAY_IDLE;
	relay_para.check_power_type_st_time =  \
			relay_para.control_relay_st_time = relay_para.md_busy_st_time = clock_time_ms();
	relay_para.isr_cnt = 0;

	// ZERO
#if ZERO_CROSSING_ENABLE
	gpio_set_interrupt_init(
				PIN_ZERO_DETECT, PM_PIN_PULLUP_10K, 0, FLD_IRQ_GPIO_EN
			);

	gpio_set_interrupt_pol(PIN_ZERO_DETECT, POL_FALLING);

	gpio_en_interrupt(PIN_ZERO_DETECT, IRQ_ENABLE);
	gpio_irq_callback_init(RL_handleZeroSync);
    #if CACULATOR_FREQ_EN
	caculator_grid_freq.start_t_ms = clock_time_ms();
    #endif
#endif

	// Relay type
    #if DETECT_RELAY_TYPE_EN
	gpio_set_func(PIN_DETECT_RELAY_TYPE, AS_GPIO);
	gpio_set_output_en(PIN_DETECT_RELAY_TYPE, 0);
	gpio_set_input_en(PIN_DETECT_RELAY_TYPE, 1);
	gpio_setup_up_down_resistor(PIN_DETECT_RELAY_TYPE, PM_PIN_PULLUP_10K);
    #endif
	// TIMER
	timer1_hw_config();
	timer1_irq_callback_init(relay_handle_control_when_timer_match);
	// Call-back sw_config
	sw_config_callback_init(relay_handle_switch_change_mode);

	// Energy
	energy_init();
	energy_callback_init(relay_get_actual_state);
	bl0906_force_control_relay_callback_init(relay_force_control);

	// Relay On Time
	relay_restore_total_relay_on_time();
	relay_report_relay_on_time_init();

	// Fact call_back
	fact_callback_result_status_init(relay_handle_fact_status);
	fact_callback_control_relay_init(relay_set_target_state);
}

/**
 * @func    relay_set_up_time_to_change_state
 * @brief
 * @param
 * @retval  None
 */
static void relay_set_up_time_to_change_state(u8 st, u8 idx)
{
	u16 cmp_us = (u16)((st == G_ON)
			?(relay_para.half_period_of_grid - NORMAL_RL_ON_DELAY)
					:(relay_para.half_period_of_grid - NORMAL_RL_OFF_DELAY));
	// 60HZ
	if(relay_para.half_period_of_grid == 8000) {
		cmp_us += ADJUST_60HZ_US;  //us
	}
    timer1_update_cmp_value(cmp_us);
}

/**
 * @func    relay_check_auto_off
 * @brief
 * @param
 * @retval  None
 */
void relay_check_auto_off(void)
{
	foreach(i, NUMBER_RL) {
		if(sw_config_st.switch_mode[i] == MOMENTORY_SWITCH_TYPE) {
			if(onRelayFlag[i] == true) {
				if(clock_time_exceed_ms(onRelayStartTime[i], TIMER_500MS)) {
					onRelayFlag[i] = false;
					relay_set_target_state(i, G_OFF, SRC_DEVICE, true);
				}
			}
		}
	}
}

/**
 * @func    relay_check_publish_relay_on_time
 * @brief
 * @param
 * @retval  None
 */
static void relay_check_publish_relay_on_time(u8 idx)
{
	if(idx < NUMBER_RL) {
		if(clock_time_exceed_s(
				relay_on_time_par.response_last_t_s[idx], relay_on_time_par.interval_t_s[idx])) {
			relay_on_time_par.interval_t_s[idx] =  \
					RELAY_ON_TIME_PUBLISH_TIME_S + rand()%RELAY_ON_TIME_PUBLISH_RANDOM_TIME_S;
			relay_on_time_par.response_last_t_s[idx] = clock_time_s();
			relay_response_total_relay_on_time(idx);
		}
	}
}

/**
 * @func    relay_calculator_on_time_proc
 * @brief
 * @param
 * @retval  None
 */
static void relay_on_time_proc(void)
{
	static u32 tick_ms = 0;
	// Check relay on time
	if(clock_time_exceed_ms(tick_ms, TIMER_5S)) {
		u32 tick_ms = clock_time_ms();
		foreach(i, NUMBER_RL) {
			// Check update relay on time
			if(((relay_para.RLS_PRESENT>>i)&0x01) == G_ON) {
				if(relay_on_time_par.check_start_t_ms[i] > tick_ms) {
					relay_on_time_par.check_start_t_ms[i]  = tick_ms;
				}
				u32 expire_time_ms = tick_ms - relay_on_time_par.check_start_t_ms[i];
				if(expire_time_ms >= CHECK_RELAY_ON_TIME_INTERVAL_MS) {
					relay_on_time_par.total_seconds[i] += expire_time_ms/1000;
					relay_on_time_par.check_start_t_ms[i] = tick_ms;
					DBG_RELAY_SEND_STR("\n 0_Update relay ON time: ");
					DBG_RELAY_SEND_INT(i);
					DBG_RELAY_SEND_STR(", ");
					DBG_RELAY_SEND_DWORD(relay_on_time_par.total_seconds[i]);
					// Check publish
					relay_check_publish_relay_on_time(i);
				}
			}
			else {
				// Check publish
				relay_check_publish_relay_on_time(i);
			}
		}
	}
}

/**
 * @func    relay_proc
 * @brief
 * @param
 * @retval  None
 */
u8 relay_proc(void)
{
	relay_on_time_proc();

#if CURRENT_CORRECTION_EN
	// Check bl0906 busy
	if(bl0906_is_correction_complete() == false) {
		return RELAY_IDLE;
	}
#endif

	// map state control
	relay_map_set_and_target_status();
	// Store delay
	relay_handle_store_delay();

	// Auto off momentary switch
	relay_check_auto_off();

	if(relay_para.relay_module_st == RELAY_IDLE)
	{
		if(relay_para.done_on_off_control_flag == CONTROL_SUCCESS)
		{
			if(relay_active_st.chn != NO_RL_CHN_ACTIVE)
			{
				// Update Present
				(relay_active_st.state == G_ON)
						?(relay_para.RLS_PRESENT |= (u16)(1 << relay_active_st.chn))
							:(relay_para.RLS_PRESENT &= (u16)(~(1 << relay_active_st.chn)));
				relay_para.RLS_PRESENT &= BACKUP_MASK_RL;

				// Update Actual
				if(sw_config_st.switch_mode[relay_active_st.chn] == LIGHTING_SWITCH_TYPE) {
					relay_para.RLS_ACTUAL |= (u16)(1 << relay_active_st.chn);
				}
				else {
					(relay_active_st.state == G_ON)
							?(relay_para.RLS_ACTUAL |= (u16)(1 << relay_active_st.chn))
								:(relay_para.RLS_ACTUAL &= (u16)(~(1 << relay_active_st.chn)));
				}
				// Only for MSB
				u8 idx = relay_active_st.chn;
				if(idx < NUMBER_RL) {
					// Relay On time
					u32 tick_ms = clock_time_ms();
					if(relay_on_time_par.check_start_t_ms[idx] > tick_ms) {
						relay_on_time_par.check_start_t_ms[idx]  = tick_ms;
					}
					if(relay_active_st.state == G_OFF) {
						if(relay_power_on_control[idx] == false) {
							relay_on_time_par.total_seconds[idx] += (tick_ms - relay_on_time_par.check_start_t_ms[idx])/1000;
							CycleFunc_add(relay_store_total_relay_on_time_delay, TIMER_5S);
							DBG_RELAY_SEND_STR("\n 1_Update relay ON time: ");
							DBG_RELAY_SEND_INT(idx);
							DBG_RELAY_SEND_STR(", ");
							DBG_RELAY_SEND_DWORD(relay_on_time_par.total_seconds[idx]);
						}
					}
					else {
						relay_on_time_par.check_start_t_ms[idx] = clock_time_ms();
					}
					relay_power_on_control[idx] = false;
					relay_para.RLS_ACTUAL &= BACKUP_MASK_RL;
				}
			}
			relay_para.done_on_off_control_flag = CONTROL_IN_PROCESSING;
		}
		if(relay_para.RLS_TARGET != relay_para.RLS_PRESENT)
		{
			for(uint8_t i = 0; i < NUMBER_RL; i++) {
				if(((relay_para.RLS_TARGET >> i) & 0x01) \
						!= ((relay_para.RLS_PRESENT >> i) & 0x01)) {
					// Prepare all parameters
					relay_active_st.chn = i;
					relay_active_st.step = 0;
					relay_active_st.state = (relay_para.RLS_TARGET >> i) & 0x01;
					// Auto OFF
					if(relay_active_st.state == G_ON) {
						if(sw_config_st.switch_mode[i] ==  \
								MOMENTORY_SWITCH_TYPE && relay_para.src_control[i] == SRC_APP)
						{
							onRelayStartTime[i] = clock_time_ms();
							onRelayFlag[i] = true;
							relay_para.src_control[i] = SRC_DEVICE;
						}
					}
					// Setup Parameter
					relay_para.relay_module_st = RELAY_BUSY;
					relay_para.wait_zero_point_st_time = clock_time_ms();
					relay_set_up_time_to_change_state(relay_active_st.state, relay_active_st.chn );
					//gpio_en_interrupt(PIN_ZERO_DETECT, IRQ_ENABLE);
					relay_para.control_when_timeout = false;

					break;
				}
			}
		}
	}
	else {
		if(relay_active_st.chn != NO_RL_CHN_ACTIVE) {
			if(relay_para.control_when_timeout == false){
				if(clock_time_exceed_ms(relay_para.wait_zero_point_st_time, WAIT_ZERO_POINT_TIMEOUT_MS)) {
					DBG_RELAY_SEND_STR("\r\n Wait Zero Point Time out");
					relay_para.control_when_timeout = true;
					timer1_enable();
				}
			}
		}
	}
	if(relay_para.relay_module_st != RELAY_BUSY) {
		relay_para.md_busy_st_time = clock_time_ms();
	}
	// Exception handle
	if(clock_time_exceed_ms(relay_para.md_busy_st_time, MODULE_BYSY_TIME_OUT_MS)) {
		start_reboot();;
	}
	return RELAY_IDLE;
}

/**
 * @func    led_handle_refresh
 * @brief
 * @param
 * @retval  None
 */
void relay_handle_refresh(u16 mask)
{
	DBG_RELAY_SEND_STR("\n relay_handle_refresh: ");
	if(fact_is_active() == 0)
	{
		DBG_RELAY_SEND_STR("OK!");
		foreach(i, NUMBER_LED_RL) {
			if(((mask >> i)&0x01) == 0x01) {
				if(((relay_para.RLS_TARGET >> i)&0x01) == G_ON) {
					led_set_color(i, LED_COLOR_RED);
				}
				else {
					led_set_color(i, LED_COLOR_BLUE);
				}
			}
		}
	}
}

/**
 * @func    relay_map_set_and_target_status
 * @brief
 * @param
 * @retval  None
 */
static void relay_map_set_and_target_status(void)
{
	if(relay_para.RLS_SET != relay_para.RLS_TARGET) {
		foreach(i, NUMBER_RL) {
			if(((relay_para.RLS_SET >> i)&0x01) != ((relay_para.RLS_TARGET >> i)&0x01)) {
				u16 model_idx = i + ELE_RELAY_OFFSET;
				st_transition_t *p_trans = P_ST_TRANS(model_idx, ST_TRANS_LIGHTNESS);
				u8 st = (relay_para.RLS_SET >> i)&0x01;
				if(st == G_ON) {
					relay_para.RLS_TARGET  |= (uint16_t)(1 << i);
					relay_para.RLS_PRESENT &= (uint16_t)(~(1 << i));
					p_trans->present = p_trans->target = LEVEL_MAX;
				}
				else {
					relay_para.RLS_TARGET  &= (uint16_t)(~(1 << i));
					relay_para.RLS_PRESENT |= (uint16_t)(1 << i);
					p_trans->present = p_trans->target = LEVEL_OFF;
				}
				// Refresh led
				// Led control
				LedCommand_str ledCmd = COMMAND_LED_DEFAULT;
				ledCmd.ledColor =  \
						(((relay_para.RLS_TARGET >> i)&1) == G_ON)?LED_COLOR_RED:LED_COLOR_BLUE;
				ledCmd.ledMode  = LED_MODE_ON;
				ledCmd.ledMask = (uint16_t)(1 << i);
				(void)led_push_led_command_to_fifo(&ledCmd);
#ifdef RELAY_DBG_EN
				if(ledCmd.ledColor == LED_COLOR_RED)
				{
					DBG_RELAY_SEND_STR("\n RL - RED");
				}
				else
				{
					DBG_RELAY_SEND_STR("\n RL - BLUE");
				}
#endif
				if(fact_is_active() == 0)
				{
					// Handle state change
					if(pvRL_handle_relay_state_change != NULL) {
						pvRL_handle_relay_state_change(i, st);
					}
				}
			}
		}
		relay_para.RLS_PRESENT &= BACKUP_MASK_RL;
		relay_para.RLS_TARGET &= BACKUP_MASK_RL;
		relay_para.RLS_SET &= BACKUP_MASK_RL;
	}
}

/**
 * @func    relay_control_refresh_all
 * @brief
 * @param
 * @retval  None
 */
void relay_control_refresh_all(void)
{
	DBG_RELAY_SEND_STR("\n relay_control_refresh_all");
	foreach(i, NUMBER_RL) {
		if(((relay_para.RLS_TARGET >> i) & 1) == G_ON) {
			relay_para.RLS_PRESENT &= (uint16_t)(~(1 << i));
		}
		else {
			relay_para.RLS_PRESENT |= (uint16_t)(1 << i);
		}
	}
}

/**
 * @func    relay_change_state
 * @brief
 * @param
 * @retval  None
 */
void relay_set_target_state(u8 idx, u8 st, src_control_enum src, bool store)
{
	DBG_RELAY_SEND_STR("\n Relay Set: ");
	DBG_RELAY_SEND_INT(idx);
	DBG_RELAY_SEND_STR(", ");
	DBG_RELAY_SEND_INT(st);

	if(idx < NUMBER_RL)
	{
		if(st == G_ON)
		{
			relay_para.RLS_SET |= (uint16_t)(1 << idx);
		}
		else
		{
			relay_para.RLS_SET &= (uint16_t)(~(1 << idx));
		}
		relay_para.RLS_SET &= BACKUP_MASK_RL;
		// Compare
		if(st != ((relay_para.RLS_TARGET >> idx)&0x01))
		{
			if(store == true)
			{
				relay_setup_store_state_delay();
			}
		}
		// Update source control RL
		relay_para.src_control[idx] = src;

#if MODIFY_SRC_CONTROL_EN
		if(src == SRC_DEVICE) {
			update_control_message_params(
							idx + ELE_RELAY_OFFSET,
							idx+ele_adr_primary,
							idx+ele_adr_primary,
							G_ONOFF_SET
						);
		}
#endif

	}
}

/**
 * @func    relay_control_same_state
 * @brief
 * @param
 * @retval  None
 */
static void relay_control_same_state(u8 idx)
{
	if(idx >= NUMBER_RL) {
		return;
	}
	if(((relay_para.RLS_TARGET >> idx)&0x01) == G_ON) {
		relay_para.RLS_PRESENT &= (uint16_t)(~(1 << idx));
	}
	else {
		relay_para.RLS_PRESENT |= (uint16_t)(1 << idx);
	}
}

/**
 * @func    relay_toggle_state
 * @brief
 * @param
 * @retval  None
 */
void relay_toggle_state(u8 idx, src_control_enum src)
{
	u8 st = (((relay_para.RLS_TARGET >> idx) & 0x01) == G_ON)?G_OFF:G_ON;
	relay_set_target_state(idx, st, src, true);
}

/**
 * @func    relay_get_present_state
 * @brief
 * @param
 * @retval  None
 */
u8 relay_get_present_state(u8 idx)
{
	return ((relay_para.RLS_PRESENT >> idx)&0x01);
}

/**
 * @func    relay_get_actual_state
 * @brief
 * @param
 * @retval  None
 */
static u8 relay_get_actual_state(u8 idx)
{
	return ((relay_para.RLS_ACTUAL >> idx)&0x01);
}

/**
 * @func    relay_get_target_state
 * @brief
 * @param
 * @retval  None
 */
u16 relay_get_target_state(void)
{
	return relay_para.RLS_TARGET;
}

/**
 * @func    relay_get_target_state_and_response
 * @brief
 * @param
 * @retval  None
 */
u8 relay_get_target_state_and_response_by_index(u8 idx)
{
	u8 st = G_ONOFF_RSV;
	if(idx < NUMBER_RL) {
		st = (relay_para.RLS_TARGET >> idx)&0x01;
		if(pvRL_handle_relay_state_change != NULL) {
			pvRL_handle_relay_state_change(idx, st);
		}
	}
	return st;
}

/**
 * @func    relay_handle_switch_change_mode
 * @brief
 * @param
 * @retval  None
 */
static void relay_handle_switch_change_mode(u8 id, u8 mode)
{
	if(id >= NUMBER_RL) {
		return;
	}
	if(sw_config_st.switch_mode[id] == LIGHTING_SWITCH_TYPE
			|| sw_config_st.switch_mode[id] == TOGGLE_SWITCH_TYPE)
	{
		relay_control_same_state(id);
	}
	else if(sw_config_st.switch_mode[id] == MOMENTORY_SWITCH_TYPE)
	{
		u16 target_st = relay_get_target_state();
		if(((target_st >> id)&0x01) == G_OFF) {
			relay_control_same_state(id);
		}
		else {
			relay_set_target_state(id, G_OFF, SRC_DEVICE, true);
		}
	}
}

// End
