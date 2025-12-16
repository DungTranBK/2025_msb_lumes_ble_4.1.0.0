/*
 * bl0940.c
 *
 *  Created on:  Nov 12, 2025
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj/tl_common.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "drivers/8258/i2c.h"
#include "vendor/mesh/user/standard/math_operation.h"
#include "vendor/common/system_time.h"
#include "vendor/mesh/user/config_board.h"
#include "vendor/mesh/user/utilities.h"
#include "vendor/mesh/user/fifo.h"
#include "vendor/common/mesh_common.h"
#include "../fact/fact.h"
#include "soft_i2c.h"
#include "at24c01.h"
#include "bl0906.h"

#include "../debug.h"
#ifdef  BL0906_DBG_EN
	#define DBG_BL0906_SEND_STR(x)     Dbg_sendString((s8*)x)
	#define DBG_BL0906_SEND_INT(x)     Dbg_sendInt(x)
	#define DBG_BL0906_SEND_HEX(x)     Dbg_sendHex(x)
	#define DBG_BL0906_SEND_BYTE(x)    Dbg_sendOneByteHex(x)
    #define DBG_BL0906_SEND_HEX32(x)   Dbg_sendHex32(x)
    #define DBG_Bl0906_SEND_DWORD(x)   Dbg_sendDword(x)
    #define DBG_Bl0906_SEND_FLOAT(x)   //Dbg_sendFloat(x)
#else
	#define DBG_BL0906_SEND_STR(x)
	#define DBG_BL0906_SEND_INT(x)
	#define DBG_BL0906_SEND_HEX(x)
	#define DBG_BL0906_SEND_BYTE(x)
    #define DBG_BL0906_SEND_HEX32(x)
    #define DBG_Bl0906_SEND_DWORD(x)
    #define DBG_Bl0906_SEND_FLOAT(x)
#endif

typeBl0906_handle_update_energy pvBl0906_handle_update_energy = NULL;
typeBl0906_handle_cf_cnt_scale_overflow pvBl0906_handle_cf_cnt_scale_overflow = NULL;

typeBl0906_force_control_relay pvBl0906_force_control_relay = NULL;


/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

#define K_internal   1

const u16 timeout = 1000;  //Serial timeout[ms]
const float Vref = (1.097*K_internal);  //[V]

const float Rf = 470*4;
const float Rv = 1.1;
const float Gain_v = 1;
const float Gain_i = GAIN_I;
const float Rl = 2;  // mOhm

const float cfdiv = 1;
// const float kp = 1;

#define kp (Gain_v*Gain_i)

static bl0906_read_cmd_t cmd_is_running = { .id_register = REG_UNKNOWN };
static Fifo_t fifo_bl0906_cmd;
static bl0906_read_cmd_t buffer_bl0906_cmds[BL0906_BUF_CMD_SIZE];
static measurement_value_t measurement_value;

typedef struct {
	u32 cf_cnt_offset;
	u32 cf_cnt_present;
}extend_value_t;

static extend_value_t extend_value[NUMBER_RL];

bool manual_rx_flag = false;
u8 manual_rx_data[4] = { 0, 0, 0, 0 };

static current_correction_par_t  current_correction_par;
static gain_par_t gain_par = { .value = 0, .set_gain_st_t_ms = 0 };


#if BL0906_CALIB_EN
/*
 * Active Power Calibration
 */
// const u32 cst_ref_load_active_power_mw[NUMBER_RL] = { 59400, 57000, 59600 };

const u32 cst_ref_load_active_power_mw[NUMBER_RL] = { 40500, 40800, 40800 };

const u32 cst_ref_load_current_ma[NUMBER_RL] = { 184, 185, 186 };
const u32 cst_ref_voltage_mv = 220000;


static BL0906_Operation_Mode_Enum bl0906_operation_mode = BL0906_NORMAL_MODE;


enum {
	CALIB_IDLE,
	CALIB_GET_CURRENT_ACTIVE_POWER,
	CALIB_CALCULATOR_K_INTERNAL,
	CALIP_STEP_UNKNOWN,
};
typedef u8 Calib_Step_Enum;

#define ACTIVE_POWER_SAMPLE_CNT      5
#define SAMPLING_PERIOD_MS           TIMER_1S

typedef struct {
	Calib_Step_Enum step;
	u32 start_t_ms;
	u32 change_step_t_ms;
	u32 get_sample_st_t_ms;

	u8  sample_cnt[NUMBER_RL];
	float average_active_power[NUMBER_RL];
	float total_active_power[NUMBER_RL];

	float voltage;

	u8  sample_current_cnt[NUMBER_RL];
	float average_current[NUMBER_RL];
	float total_current[NUMBER_RL];


}calibration_par_t;

static calibration_par_t  calibration_par;


#define THOUSANDTHS_ERROR_DONT_NEED_CALIP     1    // 0.5%
#define THOUSANDTHS_ERROR_CANT_CALIP          150  // >= 15%

enum {
	CALIP_RUNNING,
	CALIP_SUCCESS,
	CALIP_FAILURE,
};
typedef u8 Bl0906_Calib_Status_Enum;

#endif


#define K_INTERNAL_MIN                        850
#define K_INTERNAL_MAX                        1150

#define K_INTERNAL_DEFAULT                    1000

enum {
	CALIP_VOLTAGE,
	CALIP_CURRENT,
	CALIP_ACTIVE_POWER,
	CALIP_UNKNOWN,
};
typedef u8 CalibType_Enum;

#define MAX_NUMBER_RL      3

typedef struct {
	u16 k_u;
	u16 k_i[MAX_NUMBER_RL];
	u16 k_p[MAX_NUMBER_RL];
}calib_val_t;

static calib_val_t calib_val;

typedef struct {
	bool read_status;
	bool is_valid;
}eeprom_calib_t;


static eeprom_calib_t eeprom_calib = { .read_status = FAILURE, .is_valid = false };

#define EEPROM_CALIB_REG_ADDRESS      0x00

// Flash
#define BLOCK_SIZE_CALIP_VALUE    sizeof(calib_val_t)
#define FLASH_SIZE_CALIP_VALUE    (FLASH_SECTOR_SIZE - BLOCK_SIZE_CALIP_VALUE)

/******************************************************************************/
/*                             PRIVATE FUNCS                                  */
/******************************************************************************/
static void bl0906_push_msg(u8 *par, u8 par_len);
static bool bl0906_write_register(u8 address, u32 data);
static void bl0906_read_register(u8 address);

static void bl0906_send_get_current(u8 idx);
static void bl0906_get_voltage(void);
static void bl0906_get_active_power(u8 idx);
static void bl0906_get_active_energy(u8 idx);
/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

#if BL0906_CALIB_EN
/**
 * @func    bl0906_store_calib_val_to_eeprom
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_store_calib_val_to_eeprom(calib_val_t* p)
{
	DBG_BL0906_SEND_STR("\n ****** STORE CALIB VALUE TO EEPROM");
	at24c01_write_multi_byte(EEPROM_CALIB_REG_ADDRESS, (u8*)p, sizeof(calib_val_t));
}

/**
 * @func    bl0906_get_mode
 * @brief
 * @param
 * @retval  None
 */
BL0906_Operation_Mode_Enum bl0906_get_mode(void)
{
	return bl0906_operation_mode;
}

/**
 * @func    bl0906_calibration_init
 * @brief
 * @param
 * @retval  None
 */
void bl0906_go_to_calip_mode(void)
{
	memset((u8*)&calibration_par, 0, sizeof(calibration_par));
	bl0906_operation_mode = BL0906_CALIBRATION_MODE;
	DBG_BL0906_SEND_STR("\n bl0906_go_to_calip_mode");
}

/**
 * @func    Bl0906_calip_change_step
 * @brief
 * @param
 * @retval  None
 */
static void Bl0906_calip_change_step(Calib_Step_Enum step)
{
	calibration_par.step = step;
	calibration_par.change_step_t_ms = clock_time_ms();
	if(step == CALIB_GET_CURRENT_ACTIVE_POWER) {
		calibration_par.get_sample_st_t_ms = clock_time_ms();
		// TODO
		foreach(i, NUMBER_RL) {
			calibration_par.sample_cnt[i] = 0;
			calibration_par.average_active_power[i] = 0;
			calibration_par.total_active_power[i] = 0;
		}
	}
}

/**
 * @func    bl0906_proc
 * @brief
 * @param
 * @retval  None
 */
static Bl0906_Calib_Status_Enum bl0906_calibration_proc(void)
{
	switch(calibration_par.step) {
		case CALIB_IDLE:
			if(pvBl0906_force_control_relay != NULL) {
				foreach(i, NUMBER_RL) {
					pvBl0906_force_control_relay(i, G_ON);
				}
				Bl0906_calip_change_step(CALIB_GET_CURRENT_ACTIVE_POWER);
			}
			else {
				return CALIP_FAILURE;
			}
			break;

		case CALIB_GET_CURRENT_ACTIVE_POWER:
		{
			bool done_flag = true;
			foreach(i, NUMBER_RL) {
				if(calibration_par.sample_cnt[i] < ACTIVE_POWER_SAMPLE_CNT) {
					done_flag = false;
					break;
				}
			}
			if(done_flag == true) {
				DBG_BL0906_SEND_STR("\n ******************************\n CALIB_GET_CURRENT_ACTIVE_POWER: ");
				foreach(i, NUMBER_RL) {
					calibration_par.average_active_power[i] =  \
							calibration_par.total_active_power[i]/ACTIVE_POWER_SAMPLE_CNT;
					DBG_Bl0906_SEND_DWORD(calibration_par.total_active_power[i]);
					DBG_BL0906_SEND_STR(", ");

					calibration_par.average_current[i] =  \
							calibration_par.total_current[i]/ACTIVE_POWER_SAMPLE_CNT;


				}
				Bl0906_calip_change_step(CALIB_CALCULATOR_K_INTERNAL);
			}
			break;
		}
		case CALIB_CALCULATOR_K_INTERNAL:
		{
			DBG_BL0906_SEND_STR("\n ################################\n CALIB_CALCULATOR_K_INTERNAL ");

			float temp;

			/*
			 *  Active Current
			 */
			foreach(i, NUMBER_RL) {
				temp = abs(calibration_par.average_current[i] - cst_ref_load_current_ma[i]);
				// Check don't need to calibration
				u32 error_in_thousandths =  \
						(u32)((temp/cst_ref_load_current_ma[i])*1000);

				if(error_in_thousandths <= THOUSANDTHS_ERROR_DONT_NEED_CALIP) {
					calib_val.k_i[i] = K_INTERNAL_DEFAULT;
					continue;
				}
				if(error_in_thousandths > THOUSANDTHS_ERROR_CANT_CALIP) {
					calib_val.k_i[i]  = K_INTERNAL_DEFAULT;
					return CALIP_FAILURE;
				}
				// Positive Error
				if(calibration_par.average_current[i] > cst_ref_load_current_ma[i]) {
					calib_val.k_i[i]  =  \
							(K_INTERNAL_DEFAULT - error_in_thousandths);
					continue;
				}
				// Negative Error
				else {
					calib_val.k_i[i]  =  \
							(K_INTERNAL_DEFAULT + error_in_thousandths);
				}
			}
#ifdef  BL0906_DBG_EN
			DBG_BL0906_SEND_STR("\n **********************\n Calib_current_arr: ");
			foreach(i, NUMBER_RL) {
				DBG_BL0906_SEND_INT(i);
				DBG_BL0906_SEND_STR(", ");
				DBG_BL0906_SEND_INT(calibration_par.average_current[i]);
				DBG_BL0906_SEND_STR(", ");
				DBG_BL0906_SEND_INT(calib_val.k_i[i]);
				DBG_BL0906_SEND_STR(" - ");
			}
#endif
			/*
			 * Voltage
			 */
			temp = abs(calibration_par.voltage - cst_ref_voltage_mv);
			u32 error_in_thousandths =  \
					(u32)((temp/cst_ref_voltage_mv)*1000);

			DBG_BL0906_SEND_STR("\n *** VOLTAGE: ");
			DBG_Bl0906_SEND_DWORD((u32)calibration_par.voltage);

			if(error_in_thousandths <= THOUSANDTHS_ERROR_DONT_NEED_CALIP) {
				calib_val.k_u = K_INTERNAL_DEFAULT;
			}
			if(error_in_thousandths > THOUSANDTHS_ERROR_CANT_CALIP) {
				calib_val.k_u  = K_INTERNAL_DEFAULT;
				DBG_BL0906_SEND_STR("\n *** CALIB VOLTAGE FAILURE");
				return CALIP_FAILURE;
			}
			// Positive Error
			if(calibration_par.voltage > cst_ref_voltage_mv) {
				calib_val.k_u  =  \
						(K_INTERNAL_DEFAULT - error_in_thousandths);
			}
			// Negative Error
			else {
				calib_val.k_u  =  \
						(K_INTERNAL_DEFAULT + error_in_thousandths);
			}
#ifdef  BL0906_DBG_EN
			DBG_BL0906_SEND_STR("\n **********************\n Calib_voltage: ");
			DBG_BL0906_SEND_INT(calib_val.k_u);
#endif
			/*
			 *  Active Power
			 */
			u16 calib_arr[NUMBER_RL];
			memset((u8*)&calib_arr, 0, sizeof(calib_arr));

			foreach(i, NUMBER_RL) {
				temp = abs(calibration_par.average_active_power[i] - cst_ref_load_active_power_mw[i]);
				// Check don't need to calibration
				u32 error_in_thousandths =  \
						(u32)((temp/cst_ref_load_active_power_mw[i])*1000);

				if(error_in_thousandths <= THOUSANDTHS_ERROR_DONT_NEED_CALIP) {
					DBG_BL0906_SEND_STR("\n Don't need calib: ");
					DBG_BL0906_SEND_INT(i);
					calib_arr[i] = K_INTERNAL_DEFAULT;
					continue;
				}
				if(error_in_thousandths > THOUSANDTHS_ERROR_CANT_CALIP) {
					DBG_BL0906_SEND_STR("\n Error over 10%, cannot be processed: ");
					DBG_BL0906_SEND_INT(i);
					DBG_BL0906_SEND_STR(", ");
					DBG_BL0906_SEND_INT(calibration_par.average_active_power[i]);

					DBG_BL0906_SEND_STR(", ");
					DBG_BL0906_SEND_INT(temp);

					calib_arr[i] = K_INTERNAL_DEFAULT;
					return CALIP_FAILURE;
				}
				// Positive Error
				if(calibration_par.average_active_power[i] > cst_ref_load_active_power_mw[i]) {
					calib_arr[i] =  \
							(K_INTERNAL_DEFAULT - error_in_thousandths);


					calib_arr[i] = cst_ref_load_active_power_mw[i]/calibration_par.average_active_power[i];



					DBG_BL0906_SEND_STR("\n Positive Error: ");
					DBG_BL0906_SEND_INT(i);
					DBG_BL0906_SEND_STR(" - ");
					DBG_BL0906_SEND_INT(calib_arr[i]);
					continue;
				}
				// Negative Error
				else {
					calib_arr[i] =  \
							(K_INTERNAL_DEFAULT + error_in_thousandths);
					DBG_BL0906_SEND_STR("\n Negative Error: ");
					DBG_BL0906_SEND_INT(i);
					DBG_BL0906_SEND_STR(" - ");
					DBG_BL0906_SEND_INT(calib_arr[i]);
				}
				// Double check
				if(calib_arr[i] < K_INTERNAL_MIN  \
						|| calib_arr[i] > K_INTERNAL_MAX) {
					DBG_BL0906_SEND_STR("\n Error range over: ");
					DBG_BL0906_SEND_INT(i);
					DBG_BL0906_SEND_STR(", ");
					DBG_BL0906_SEND_INT(calib_arr[i]);
					return CALIP_FAILURE;
				}
			}
			/*
			 *  Update calibration value and control relay
			 */
			foreach(i, NUMBER_RL) {
				calib_val.k_p[i] = calib_arr[i];
			}
#ifdef  BL0906_DBG_EN
			DBG_BL0906_SEND_STR("\n **********************\n Calibration success: ");
			foreach(i, NUMBER_RL) {
				DBG_BL0906_SEND_INT(i);
				DBG_BL0906_SEND_STR(", ");
				DBG_BL0906_SEND_INT(calib_val.k_p[i]);
			}
#endif
			// Write to EEPROM
			bl0906_store_calib_val_to_eeprom(&calib_val);
			// Turn off all relay after calibration
			foreach(i, NUMBER_RL) {
				pvBl0906_force_control_relay(i, G_OFF);
			}
			return CALIP_SUCCESS;
		}
		default: return CALIP_FAILURE;
	}
	return CALIP_RUNNING;
}

#endif


/**
 * @func    bl0906_handdle_rx_manual
 * @brief
 * @param
 * @retval  None
 */
void bl0906_handdle_rx_manual(u8* buff, u8 len)
{
	if(len >= sizeof(manual_rx_data)) {
		memcpy(manual_rx_data, buff, sizeof(manual_rx_data));
		manual_rx_flag = true;
	}
}

/**
 * @func    bl_0906_set_gain
 * @brief
 * @param
 * @retval  None
 */
void bl_0906_set_gain(u32 gain)
{
	bl0906_write_register(GAIN_1, gain);
}

/**
 * @func    bl0906_force_control_relay_callback_init
 * @brief
 * @param
 * @retval  None
 */
void bl0906_force_control_relay_callback_init(typeBl0906_force_control_relay func)
{
	if(func != NULL) {
		pvBl0906_force_control_relay = func;
	}
}

/**
 * @func    bl0906_check_valid_and_modify_calib_value
 * @brief
 * @param
 * @retval  None
 */
static bool bl0906_check_valid_and_modify_calib_value(calib_val_t* p_calib)
{
	bool ret = true;
	// Check valid K_u, K_i, K_p
	if(p_calib->k_u < K_INTERNAL_MIN || p_calib->k_u > K_INTERNAL_MAX) {
		p_calib->k_u = K_INTERNAL_DEFAULT;
		ret = false;
	}
	foreach(i, NUMBER_RL) {
		if(p_calib->k_i[i] < K_INTERNAL_MIN || p_calib->k_i[i] > K_INTERNAL_MAX) {
			p_calib->k_i[i] = K_INTERNAL_DEFAULT;
			ret = false;
		}
		if(p_calib->k_p[i] < K_INTERNAL_MIN || p_calib->k_p[i] > K_INTERNAL_MAX) {
			p_calib->k_p[i] = K_INTERNAL_DEFAULT;
			ret = false;
		}
	}
	return ret;
}

/**
 * @func    bl0906_store_calib_value_to_flash
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_store_calib_value_to_flash(calib_val_t* p)
{
	flash_erase_sector(FLASH_ADR_CALIB_POWER_VALUE);
	flash_write_page (
			FLASH_ADR_CALIB_POWER_VALUE, BLOCK_SIZE_CALIP_VALUE, (u8*)p
		);
}

/**
 * @func    bl0906_restore_calib_from_flash
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_restore_calib_from_flash(calib_val_t* p)
{
	flash_read_page(
			FLASH_ADR_CALIB_POWER_VALUE, BLOCK_SIZE_CALIP_VALUE, (u8*)p
		);
}

/**
 * @func    bl0906_restore_calib_val_from_eeprom_and_flash
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_restore_calib_val_from_eeprom_and_flash(void)
{
#if EEPROM_ENABLE
	at24c01_init();
	__delay_ms(5);

	memset((u8*)&calib_val, 0xFF, sizeof(calib_val_t));
	int ret;
	bool eeprom_err = true;
	foreach(i, READ_EEPROM_RETRY_TIME) {
		ret = at24c01_read_multi_byte(EEPROM_CALIB_REG_ADDRESS, (u8*)&calib_val, sizeof(calib_val_t));
		if(ret == 0) {
			eeprom_err = false;
			break;
		}
		__delay_ms(10);
	}
	calib_val_t flash_calib_val;
	// Save to internal flash
	if(eeprom_err == false) {
		i2c_is_pass = true;
		eeprom_calib.read_status = SUCCESS;
		if(bl0906_check_valid_and_modify_calib_value(&calib_val) == true) {
			DBG_BL0906_SEND_STR("\n Read success, EEPROM CALIB VALUE is valid");
			eeprom_calib.is_valid = true;
			calib_is_pass = true;

			// Save to internal flash
			bl0906_restore_calib_from_flash(&flash_calib_val);
			if(memcmp(&flash_calib_val, &calib_val, sizeof(calib_val_t)) != 0) {
				DBG_BL0906_SEND_STR("\n Save calib value from EEPROM to flash");
				bl0906_store_calib_value_to_flash(&calib_val);
			}
		}
		else {
			DBG_BL0906_SEND_STR("\n Read success, But EEPROM CALIB VALUE is invalid");
		}
	}
	else {
		DBG_BL0906_SEND_STR("\n RESTORE CALIB VALUE FROM FLASH");
		// Read from flash
		bl0906_restore_calib_from_flash(&calib_val);
		if(bl0906_check_valid_and_modify_calib_value(&calib_val) == true) {
			eeprom_calib.is_valid = true;
		}
	}

	#ifdef  BL0906_DBG_EN
	DBG_BL0906_SEND_STR("\n ***\n Restore Calib Value: ");
	DBG_BL0906_SEND_STR("\n U: ");
	DBG_BL0906_SEND_INT(calib_val.k_u);
	DBG_BL0906_SEND_STR("\n I: ");
	foreach(i, NUMBER_RL) {
		DBG_BL0906_SEND_INT(calib_val.k_i[i]);
		DBG_BL0906_SEND_STR(", ");
	}
	DBG_BL0906_SEND_STR("\n P: ");
	foreach(i, NUMBER_RL) {
		DBG_BL0906_SEND_INT(calib_val.k_p[i]);
		DBG_BL0906_SEND_STR(", ");
	}
	DBG_BL0906_SEND_STR("\n ***\n");
	#endif

#endif
}

/**
 * @func    bl0940_init
 * @brief
 * @param
 * @retval  None
 */
void bl0906_init(
			typeBl0906_handle_update_energy func,
			typeBl0906_handle_cf_cnt_scale_overflow func_1
		)
{
	FifoInit(&fifo_bl0906_cmd, &buffer_bl0906_cmds, sizeof(bl0906_read_cmd_t), BL0906_BUF_CMD_SIZE);
	if(func != NULL) {
		pvBl0906_handle_update_energy = func;
	}
	if(func_1 != NULL) {
		pvBl0906_handle_cf_cnt_scale_overflow = func_1;
	}
	// For current correction when power on
	current_correction_par.step = STEP_CORRECTION_READ_RMSOS;
	current_correction_par.cmsos_bit_mask = 0;
	current_correction_par.start_time_ms = clock_time_ms();
	current_correction_par.retry_start_time_ms = 0;
	foreach(i, NUMBER_RL) {
		current_correction_par.complete_flag[i] = false;
		extend_value[i].cf_cnt_offset = CF_CNT_UNKNOWN;
		extend_value[i].cf_cnt_present = CF_CNT_UNKNOWN;
	}
    // Restore
	bl0906_restore_calib_val_from_eeprom_and_flash();

/*
	// For test
	calib_val.k_u = K_INTERNAL_DEFAULT;
	foreach(i, MAX_NUMBER_RL) {
		calib_val.k_i[i] = K_INTERNAL_DEFAULT;
		calib_val.k_p[i] = K_INTERNAL_DEFAULT;
	}
	*/
}

/**
 * @func    array_to_u24
 * @brief
 * @param
 * @retval  None
 */
static u32 array_to_u24(u8* in)
{
	return (u32)(in[0] | in[1] << 8 | in[2] << 16);
}

/**
 * @func    bl0940_update_energy
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_update_energy(u8 idx, u8 type, float value)
{
#if CURRENT_CORRECTION_EN
	if(bl0906_is_correction_complete() == true)
#endif
	{
		if(pvBl0906_handle_update_energy != NULL) {
			pvBl0906_handle_update_energy(idx, type, value);
		}
	}
}

/**
 * @func    bl0906_bias_correction
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_bias_correction(u8 addr, float measurements, float correction)
{
	u8 index;
	switch(addr) {
		case RMSOS_1:
			index = 0;
			break;

		case RMSOS_2:
			index = 1;
			break;

		case RMSOS_3:
			index = 2;
			break;

		default:
			return;
	}
	double  ki =  (12875 * Rl * Gain_i)/Vref;
	float i_rms0 = measurements * ki;
	float i_rms = correction * ki;
	int32_t value = (i_rms * i_rms - i_rms0 * i_rms0) / 256;

	u8 data[3];
	data[0] = value;
	data[1] = (value >> 8);
    if (value < 0) {
    	data[2] = (value >> 16) | 0b10000000;
    }
    else {
    	data[2] = (value >> 16);
    }

	bl0906_write_register(addr, value);

	current_correction_par.rmsos[index] = value&0x00FFFFFF;

	DBG_BL0906_SEND_STR("\n ^^^^^^^^^^^^^^^^^^^^^ RMSOS SET: ");
	DBG_BL0906_SEND_INT(index);
	DBG_BL0906_SEND_STR(", ");
	DBG_Bl0906_SEND_DWORD(value);
}

#if CURRENT_CORRECTION_EN
/**
 * @func    bl0906_is_correction_complete
 * @brief
 * @param
 * @retval  None
 */
bool bl0906_is_correction_complete(void)
{
	if(current_correction_par.step == STEP_CORRECTION_IDLE) {
		return true;
	}
	return false;
}
#endif

/**
 * @func    bl0906_handle_current_rsp
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_handle_current_rsp(u8* par, u8 par_len)
{
	u32 data = array_to_u24(par);
	measurement_value.current = (float)data * Vref / (12875 * Rl * Gain_i);
	float ampere_value = measurement_value.current;
	measurement_value.current *= 1000;   // A to mA

	u8 index, correction_reg;
	switch(cmd_is_running.id_register) {
		case I1_RMS:
			index = 0;
			correction_reg = RMSOS_1;
			break;
		case I2_RMS:
			index = 1;
			correction_reg = RMSOS_2;
			break;
		case I3_RMS:
			index = 2;
			correction_reg = RMSOS_3;
			break;
		default:
			return;
	}

	DBG_BL0906_SEND_STR("\n*************************");
	DBG_BL0906_SEND_STR("\n __CURRENT__:");
	DBG_BL0906_SEND_BYTE(index);
	DBG_BL0906_SEND_STR(" - ");
	DBG_BL0906_SEND_INT((u16)measurement_value.current);
	DBG_BL0906_SEND_STR(", ");
	DBG_BL0906_SEND_INT((u16)measurement_value.current*calib_val.k_i[index]/1000);

#if BL0906_CALIB_EN
	if(bl0906_operation_mode == BL0906_CALIBRATION_MODE) {
		if(calibration_par.step == CALIB_GET_CURRENT_ACTIVE_POWER) {
			if(calibration_par.sample_current_cnt[index] < ACTIVE_POWER_SAMPLE_CNT) {
				calibration_par.total_current[index] += measurement_value.current;
				calibration_par.sample_current_cnt[index]++;
			}
		}
	}
#endif
	if(current_correction_par.step == STEP_CORRECTION_PROCESS) {
		if(current_correction_par.complete_flag[index] == false) {
			if(measurement_value.current == 0) {
				current_correction_par.complete_flag[index] = true;
				DBG_BL0906_SEND_STR("\n Don't need calib: ");
				DBG_BL0906_SEND_INT(index);
			}
			else {
				if(measurement_value.current <= CURRENT_OFFSET_MA_MAX) {
					bl0906_bias_correction(correction_reg, ampere_value, 0);
					bl0906_read_register(correction_reg);
				}
			}
		}
	}
	// Calibration Current
	measurement_value.current =  \
			measurement_value.current*calib_val.k_i[index]/1000;
	// Report
	bl0906_update_energy(index, TYPE_CURRENT, measurement_value.current);
}

/**
 * @func    bl0906_handle_rmsos_rsp
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_handle_rmsos_rsp(u8* par, u8 par_len)
{
	u32 rmsos = array_to_u24(par);
	u8 index;
	switch(cmd_is_running.id_register) {
		case RMSOS_1:
			index = 0;
			break;
		case RMSOS_2:
			index = 1;
			break;
		case RMSOS_3:
			index = 2;
			break;

		default:
			return;
	}
	current_correction_par.cmsos_bit_mask |= (1 << index);

	DBG_BL0906_SEND_STR("\n RESPONSE RMSOS: ");
	DBG_BL0906_SEND_INT(index);
	DBG_BL0906_SEND_STR(", ");
	DBG_Bl0906_SEND_DWORD(rmsos);

	if(current_correction_par.step == STEP_CORRECTION_READ_RMSOS) {
		if(rmsos != 0) {
			current_correction_par.complete_flag[index] = true;
			DBG_BL0906_SEND_STR("\n Don't need CALIP: ");
			DBG_BL0906_SEND_INT(index);
		}
		if((current_correction_par.cmsos_bit_mask& \
				BL0906_ALL_CHANNEL_BIT_MASK) == BL0906_ALL_CHANNEL_BIT_MASK) {
			current_correction_par.step = STEP_CORRECTION_PROCESS;
		}
		DBG_BL0906_SEND_STR("\n *** STEP_READ_RMSOS: ");
		DBG_BL0906_SEND_INT(index);
	}
	else {
		DBG_BL0906_SEND_STR("\n STEP_CORRECTION_PROCESS");
		if(current_correction_par.rmsos[index] == rmsos) {
			current_correction_par.complete_flag[index] = true;
			DBG_BL0906_SEND_STR("\n RMSOS set complete: ");
			DBG_BL0906_SEND_INT(index);
			DBG_BL0906_SEND_STR(", ");
			DBG_Bl0906_SEND_DWORD(current_correction_par.rmsos[index]);
		}
	}
}

/**
 * @func    bl0906_handle_voltage_rsp
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_handle_voltage_rsp(u8* par, u8 par_len)
{
	u32 data = array_to_u24(par);
	measurement_value.voltage = \
			(float)data * Vref * (Rf + Rv) / (13162*Rv*Gain_v*1000)*1000;    // mV

#if BL0906_CALIB_EN
	if(bl0906_operation_mode == BL0906_CALIBRATION_MODE) {
		if(calibration_par.step == CALIB_GET_CURRENT_ACTIVE_POWER) {
			if(calibration_par.voltage > 1000) {
				calibration_par.voltage = measurement_value.voltage;
			}
			else {
				calibration_par.voltage = (measurement_value.voltage+calibration_par.voltage)/2;
			}
		}
	}
#endif

	DBG_BL0906_SEND_STR("\n __VOLTAGE__:");
	DBG_BL0906_SEND_INT((u16)(measurement_value.voltage/1000));
	DBG_BL0906_SEND_STR(", ");
	DBG_Bl0906_SEND_FLOAT(measurement_value.voltage);
	DBG_BL0906_SEND_STR(", ");
	DBG_Bl0906_SEND_DWORD((u32)(measurement_value.voltage*calib_val.k_u/1000));

	// Calibration voltage
	measurement_value.voltage =  \
			measurement_value.voltage*calib_val.k_u/1000;
	// Report
	bl0906_update_energy(0, TYPE_VOLTAGE, measurement_value.voltage);

}

/**
 * @func    bl0906_handle_active_power_rsp
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_handle_active_power_rsp(u8* par, u8 par_len)
{
	u32 data = array_to_u24(par);
	u8 index;
	switch(cmd_is_running.id_register) {
		case WATT_1:
			index = 0;
			break;
		case WATT_2:
			index = 1;
			break;
		case WATT_3:
			index = 2;
			break;

		default:
			return;
	}
	bool sign_bit = (par[2]>>7)&0x01;
	if (sign_bit == 1) {
		DBG_BL0906_SEND_STR("\n ***Negative Power");
		measurement_value.active_power = 0;
	}
	else {
		measurement_value.active_power =
			(float)data * Vref * Vref * (Rf + Rv)/(40.4125 * Rl*Rv*Gain_i*1000)*1000;    // Convert from W to mW
	}

#if BL0906_CALIB_EN
	if(bl0906_operation_mode == BL0906_CALIBRATION_MODE) {
		if(calibration_par.step == CALIB_GET_CURRENT_ACTIVE_POWER) {
			if(calibration_par.sample_cnt[index] < ACTIVE_POWER_SAMPLE_CNT) {
				calibration_par.total_active_power[index] += measurement_value.active_power;
				calibration_par.sample_cnt[index]++;
			}
		}
	}
#endif
	DBG_BL0906_SEND_STR("\n __ACTIVE_POWER__: ");
	DBG_BL0906_SEND_INT(index);
	DBG_BL0906_SEND_STR(" - ");
	DBG_Bl0906_SEND_DWORD(measurement_value.active_power);
	DBG_BL0906_SEND_STR(", ");
	DBG_Bl0906_SEND_DWORD(measurement_value.active_power*calib_val.k_p[index]/1000);

	// Calibration active power
	measurement_value.active_power =  \
			measurement_value.active_power*calib_val.k_p[index]/1000;
	// Report
	bl0906_update_energy(index, TYPE_ACTIVE_POWER, measurement_value.active_power);
}

/**
 * @func    bl0906_handle_active_energy_rsp
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_handle_active_energy_rsp(u8* par, u8 par_len)
{
	u32 cf_cnt = array_to_u24(par);
	u8 index;
	switch(cmd_is_running.id_register) {
		case CF1_CNT:
			index = 0;
			break;

		case CF2_CNT:
			index = 1;
			break;

		case CF3_CNT:
			index = 2;
			break;

		default:
			return;
	}
	if(extend_value[index].cf_cnt_offset == CF_CNT_UNKNOWN) {
		extend_value[index].cf_cnt_offset = cf_cnt;
	}
	if(extend_value[index].cf_cnt_present == CF_CNT_UNKNOWN) {
		extend_value[index].cf_cnt_present = cf_cnt;
	}
	else {
		if(cf_cnt < extend_value[index].cf_cnt_present) {
			extend_value[index].cf_cnt_offset = cf_cnt;
			if(pvBl0906_handle_cf_cnt_scale_overflow != NULL) {
				pvBl0906_handle_cf_cnt_scale_overflow(index);
			}
		}
	}
	extend_value[index].cf_cnt_present = cf_cnt;

	u32 delta_cf_cnt = cf_cnt - extend_value[index].cf_cnt_offset;
	float k_p = (40.4125*Rl*Rv*Gain_i)/(Vref * Vref * (Rf + Rv));
	float WH_PER_PULSE = (4194304*0.032768) / (3600000 * cfdiv  * k_p);

	measurement_value.active_energy = delta_cf_cnt * WH_PER_PULSE;
    // Calibration active energy
	measurement_value.active_energy =  \
			measurement_value.active_energy*calib_val.k_p[index]/1000;
    // Report
	bl0906_update_energy(index, TYPE_ACTIVE_ENERGY, measurement_value.active_energy);
}

/**
 * @func    bl0906_handle_temperature_rsp
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_handle_temperature_rsp(u8* par, u8 par_len)
{
	u32 data = array_to_u24(par) & 0x03FF;
	measurement_value.temperature =  \
			(data - 64)*12.5/59-40;
	// bl0906_update_energy(0, TYPE_TEMPERATURE, measurement_value.temperature);
}

/**
 * @func    bl0906_handle_temperature_rsp
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_handle_gain_rsp(u8* par, u8 par_len)
{
	gain_par.value = array_to_u24(par);
}

/**
 * @func   bl0906_push_wait_reg_rsp_to_fifo
 * @brief
 * @param
 * @retval None
 */
static bool bl0906_push_wait_reg_rsp_to_fifo(u8 reg_id)
{
	bl0906_read_cmd_t bl0906_read_cmd;
	bl0906_read_cmd.id_register = reg_id;
	if(FifoPush(&fifo_bl0906_cmd, &bl0906_read_cmd)) {
		return true;
	}
	return false;
}

/**
 * @func    bl0906_init
 * @brief
 * @param
 * @retval  None
 */
void bl0906_measurenment_start(u8 idx, u16 m_mask)
{
	foreach(i, BL0904_MEASUREMENT_MAX)
	{
		u16 tmp = (m_mask & (u16)(1 << i));
		if(!tmp) continue;
		if(tmp == BIT_MASK_CURRENT) {
			bl0906_send_get_current(idx);
		}
		else if(tmp == BIT_MASK_VOLTAGE) {
			if(idx == 0) {
				bl0906_get_voltage();
			}
		}
		else if(tmp == BIT_MASK_ACTIVE_POWER) {
			bl0906_get_active_power(idx);
		}
		else if(tmp == BIT_MASK_ACTIVE_ENERGY) {
			bl0906_get_active_energy(idx);
		}
	}
}

#if CURRENT_CORRECTION_EN
/**
 * @func    bl0906_current_correction_proc
 * @brief   Note, make sure that all relays are off, otherwise this process may cause errors.
 * @param
 * @retval  None
 */
static void bl0906_current_correction_proc(void)
{
	bool complete = true;
	if(current_correction_par.step == STEP_CORRECTION_IDLE) {
		return;
	}
	if(clock_time_exceed_ms(current_correction_par.start_time_ms, CURRENT_CORRECTION_TIMEOUT_MS)) {
		current_correction_par.step = STEP_CORRECTION_IDLE;
		return;
	}
	if(current_correction_par.step == STEP_CORRECTION_READ_RMSOS) {
		if(clock_time_exceed_ms(  \
				current_correction_par.retry_start_time_ms, CURRENT_CORRECTION_RETRY_INTERVAl_MS)) {
			DBG_BL0906_SEND_STR("\n #####################################");
			bl0906_read_register(RMSOS_1);
			bl0906_read_register(RMSOS_2);
			bl0906_read_register(RMSOS_3);
			current_correction_par.retry_start_time_ms = clock_time_ms();
		}
	}
	else if(current_correction_par.step == STEP_CORRECTION_PROCESS) {
		foreach(i, NUMBER_RL) {
			if(current_correction_par.complete_flag[i] == false) {
				if(clock_time_exceed_ms(  \
						current_correction_par.retry_start_time_ms, CURRENT_CORRECTION_RETRY_INTERVAl_MS)  \
							|| (current_correction_par.retry_start_time_ms == 0)) {
					current_correction_par.retry_start_time_ms = clock_time_ms();
					if(current_correction_par.retry_start_time_ms == 0) {
						current_correction_par.retry_start_time_ms = 1;
					}
					foreach(i, NUMBER_RL) {
						bl0906_send_get_current(i);
					}
					DBG_BL0906_SEND_STR("\n &&&&&&&&&&&&&&&&&&&&&&&&&&&");
					return;
				}
				complete = false;
			}
		}
		if(complete == true) {
			current_correction_par.step = STEP_CORRECTION_IDLE;
		}
	}
}
#endif

/**
 * @func    bl0906_set_gain_proc
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_set_gain_proc(void)
{
	if(Gain_i == 16) {
		if(gain_par.value != GAIN_1_DEFAULT_VALUE) {
			if(clock_time_exceed_ms(gain_par.set_gain_st_t_ms, SET_GAIN_INTERVAL_MS)) {
				bl_0906_set_gain(GAIN_1_DEFAULT_VALUE);
				bl0906_read_register(GAIN_1);
				gain_par.set_gain_st_t_ms = clock_time_ms();
			}
		}
	}
}

/**
 * @func    bl0906_fifo_proc
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_fifo_proc(void)
{
	if(cmd_is_running.id_register != REG_UNKNOWN) {
		if(clock_time_exceed_ms(cmd_is_running.active_st_time, BL0906_READ_TIMEOUT)) {
			cmd_is_running.id_register = REG_UNKNOWN;
		}
	}
	else {
		if (FifoIsEmpty(&fifo_bl0906_cmd) == false){
			if(FifoPop(&fifo_bl0906_cmd, &cmd_is_running)){
				cmd_is_running.active_st_time = clock_time_ms();
				switch(cmd_is_running.id_register) {
					case I1_RMS:
					case I2_RMS:
					case I3_RMS:
						cmd_is_running.p_func = bl0906_handle_current_rsp;
						break;

					case RMSOS_1:
					case RMSOS_2:
					case RMSOS_3:
						cmd_is_running.p_func = bl0906_handle_rmsos_rsp;
						break;

					case V_RMS:
						cmd_is_running.p_func = bl0906_handle_voltage_rsp;
						break;

					case WATT_1:
					case WATT_2:
					case WATT_3:
						cmd_is_running.p_func = bl0906_handle_active_power_rsp;
						break;

					case CF1_CNT:
					case CF2_CNT:
					case CF3_CNT:
						cmd_is_running.p_func = bl0906_handle_active_energy_rsp;
						break;

					case TPS:
						cmd_is_running.p_func = bl0906_handle_temperature_rsp;
						break;

					case GAIN_1:
						cmd_is_running.p_func = bl0906_handle_gain_rsp;
						break;

					default:
						cmd_is_running.p_func = NULL;
						cmd_is_running.id_register = REG_UNKNOWN;
						break;
				}
				u8 tx_data[] = { BL0906_READ_COMMAND, cmd_is_running.id_register };
				bl0906_push_msg(tx_data, sizeof(tx_data));
			}
		}
	}
}

/**
 * @func    bl0906_proc
 * @brief
 * @param
 * @retval  None
 */
void bl0906_proc(void)
{
#if BL0906_CALIB_EN
	if(bl0906_operation_mode == BL0906_CALIBRATION_MODE) {
		Bl0906_Calib_Status_Enum status;
		status = bl0906_calibration_proc();
		if(status == CALIP_FAILURE || status == CALIP_SUCCESS) {
			bl0906_operation_mode = BL0906_NORMAL_MODE;
		}
	}
#endif

#if CURRENT_CORRECTION_EN
	bl0906_current_correction_proc();
#endif

	bl0906_set_gain_proc();
	bl0906_fifo_proc();
}

/**
 * @func    bl0906_handle_serial_rx_message
 * @brief
 * @param
 * @retval  None
 */
void bl0906_handle_serial_rx_message(u8* buff, u8 len)
{
	 u8 rx_data[BL0906_RX_LEN];
	 memcpy(rx_data, buff, BL0906_RX_LEN);
	 if(cmd_is_running.id_register != REG_UNKNOWN) {
		 if(cmd_is_running.p_func != NULL) {
			 u8 checksum = cmd_is_running.id_register ;
			 for(u8 i = 0; i < len-1; i++) {
				 checksum += buff[i];
			 }
			 checksum = checksum^0xFF;
			 if(checksum == buff[len-1]) {
				 if(cmd_is_running.p_func != NULL) {
					 cmd_is_running.p_func(rx_data, BL0906_RX_LEN - 1);
				 }
				 cmd_is_running.id_register = REG_UNKNOWN;
				 cmd_is_running.p_func = NULL;
			 }
		 }
	 }
}

/**
 * @func    _culcCheckSum
 * @brief
 * @param
 * @retval  None
 */
u8 _culcCheckSum(u8 *tx_data, int tx_len, u8 *rx_data, int rx_len)
{
	u8 checksum = 0;
	for(u8 i = 0; i < tx_len; i++) {
		checksum += tx_data[i];
	}
	for(u8 i = 0; i < rx_len; i++) {
		checksum += rx_data[i];
	}
	checksum = checksum^0xFF;
	return checksum;
}

/**
 * @func    bl0906_push_msg
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_push_msg(u8 *par, u8 par_len)
{
	my_fifo_push_hci_tx_fifo(par, par_len, 0, 0);
}

/**
 * @func    bl0940_write_register
 * @brief
 * @param
 * @retval  None
 */
static bool bl0906_write_register(u8 address, u32 data)
{
	// Remove write protection
	u8 tx_wrprot[] = { BL0906_WRITE_COMMAND, 0x9e, 0x55, 0x55, 0x00, 0xb7};
	bl0906_push_msg(tx_wrprot, sizeof(tx_wrprot));
	// Write Register
    u8 tx_data[6] = {BL0906_WRITE_COMMAND, address, (u8)(data), (u8)(data >> 8), (u8)(data >> 16)};
    tx_data[5] = _culcCheckSum(&tx_data[1], sizeof(tx_data) - 2, 0, 0);
    bl0906_push_msg(tx_data, sizeof(tx_data));

    // Enable write protection
	u8 tx_read_only[] = { BL0906_WRITE_COMMAND, 0x9e, 0x00, 0x00, 0x00, 0x61};
	bl0906_push_msg(tx_read_only, sizeof(tx_read_only));
    return true;
}

/**
 * @func    bl0906_read_register
 * @brief
 * @param
 * @retval  None
 */
static void bl0906_read_register(u8 address)
{
	bl0906_push_wait_reg_rsp_to_fifo(address);
}

/**
 * @func    bl0940_get_current
 * @brief
 * @param
 * @retval  None
 */
void bl0906_send_get_current(u8 idx)
{
	if(idx < NUMBER_RL) {
		if(idx == 0) {
			bl0906_read_register(I1_RMS);
		}
		else if(idx == 1) {
			bl0906_read_register(I2_RMS);
		}
		else if(idx == 2) {
			bl0906_read_register(I3_RMS);
		}
	}
}

/**
 * @func    bl0906_get_voltage
 * @brief
 * @param
 * @retval  None
 */
void bl0906_get_voltage(void)
{
	bl0906_read_register(V_RMS);
}

/**
 * @func    bl0906_get_active_power
 * @brief
 * @param
 * @retval  None
 */
void bl0906_get_active_power(u8 idx)
{
	if(idx < NUMBER_RL) {
		if(idx == 0) {
			bl0906_read_register(WATT_1);
		}
		else if(idx == 1) {
			bl0906_read_register(WATT_2);
		}
		else if(idx == 2) {
			bl0906_read_register(WATT_3);
		}
	}
}

/**
 * @func    bl0906_get_active_energy
 * @brief
 * @param
 * @retval  None
 */
void bl0906_get_active_energy(u8 idx)
{
	if(idx < NUMBER_RL) {
		if(idx == 0) {
			bl0906_read_register(CF1_CNT);
		}
		else if(idx == 1) {
			bl0906_read_register(CF2_CNT);
		}
		else if(idx == 2) {
			bl0906_read_register(CF3_CNT);
		}
	}
}

#if 0
/**
 * @func    bl0906_get_temperature
 * @brief
 * @param
 * @retval  None
 */
void bl0906_get_temperature(void)
{
	bl0906_read_register(TPS);
}
#endif

/**
 * @func    bl0906_reset
 * @brief
 * @param
 * @retval  None
 */
bool bl0906_reset(void)
{
	if (false == bl0906_write_register(SOFT_RESET, 0x5A5A5A)) {
		DBG_BL0906_SEND_STR("Can not write SOFT_RESET register.");
		return false;
	}
	sleep_ms(500);
	return true;
}
