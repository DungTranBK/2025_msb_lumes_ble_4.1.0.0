/*
 * fact.c
 *
 *  Created on: Sep 30, 2024
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "vendor/common/system_time.h"
#include "vendor/common/mesh_config.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/mesh_node.h"
#include "vendor/mesh/user/config_board.h"
#include "vendor/mesh/user/utilities.h"
#include "vendor/mesh/user/led.h"
#include "vendor/mesh/user/relay.h"
#include "vendor/mesh/user/flash_user.h"
#include "fact_handle.h"
#include "fact.h"

#include "vendor/mesh/user/debug.h"
#ifdef FACT_DBG_EN
#define DBG_FACT_SEND_STR(x)     Dbg_sendString((s8*)x)
#define DBG_FACT_SEND_INT(x)     Dbg_sendInt(x)
#define DBG_FACT_SEND_HEX(x)     Dbg_sendHex(x)
#define DBG_FACT_SEND_HEX_ONE(x) Dbg_sendHexOneByte(x);
#else
#define DBG_FACT_SEND_STR(x)
#define DBG_FACT_SEND_INT(x)
#define DBG_FACT_SEND_HEX(x)
#define DBG_FACT_SEND_HEX_ONE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/


/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/

/******************************************************************************/
/*                           EXPORT FUNCTIONS                                 */
/******************************************************************************/
typeFact_handleFactEvent    pvFact_handleFactEvent    = NULL;
typeFact_handleExitfactMode pvFact_handleExitfactMode = NULL;

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/
static int fact_check_match_trigger_val(void);
static void fact_increase_reset_cnt(void);
static void fact_clear_reset_cnt(void);
static int fact_reset_cnt_check (void);
static void fact_calculator_result(void);

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

#define SERIALS_CNT                     5


#define VALID_POWER_ON_TIME_MS          1000
#define RESET_CHECK_TIME_BASE_TIME_MS   500

static u8 reset_cnt = 0;

typedef struct {
    u16 tm_start;
    u16 tm_stop;
}fact_reset_serials_t;

static fact_reset_serials_t fact_rst_serials[] = {
                    {1, 15}, {1, 15}, {1, 15}, {1, 15}, {1, 15}, {1, 15},
                };

#define RESET_CNT_INVALID     0
#define RESET_TRIGGER_VAL    (sizeof(fact_rst_serials)/sizeof(fact_reset_serials_t) - 1)


#define FACT_ACT_TIMEOUT_MS_DEFAULT          TIMER_5Min
#define FACT_SHOW_RESULT_TIME_LEN            TIMER_15S
#define FACT_SHOW_RESULT_ST_TIME             (TIMER_90S - FACT_SHOW_RESULT_TIME_LEN)
// Show result
#define FACT_TOGGLE_INTERVAL                 TIMER_500MS
// Change to FACT mode
#define FACT_WAIT_HOST_CONFIRM_TIMEOUT       TIMER_10S
#define FACT_RETRY_CHANGE_TO_FACT_INTERVAL   ((FACT_WAIT_HOST_CONFIRM_TIMEOUT - TIMER_2S) >> 1)

typedef struct {
    u8  active_flag;
    u32 active_timeout;
    u8  active_show_result_flag;
    bool rf_is_pass;
    u32 active_st_tm;
}fact_mode_par_t;

static fact_mode_par_t fact_mode_par;

typedef struct {
    u8  total_cnt;
    u8  state;
    u32 toggle_st_time;
    u8  fact_result;
    u8  led_st;
}result_par_t;

enum {
    FACT_SUCCESS = 0,
    FACT_ERROR   = 1,
};

static result_par_t  result_par;


static int reset_cnt_flash_idx     = FLASH_INDEX_DEFAULT;
#define FLASH_SIZE_RESET_CNT       4000
#define BLOCK_SIZE_RESET_CNT       sizeof(reset_cnt)

/******************************************************************************/
/*                               FUNCTIONS                                    */
/******************************************************************************/

/**
 * @func    fact_reset_cnt_store
 * @brief
 * @param   None
 * @retval  None
 */
static void fact_reset_cnt_store(void)
{
	flash_user_store((int*)&reset_cnt_flash_idx, \
			FLASH_ADR_RESET_CNT, FLASH_SIZE_RESET_CNT, BLOCK_SIZE_RESET_CNT, (u8*)&reset_cnt);
}

/**
 * @func    fact_reset_cnt_restore
 * @brief
 * @param   None
 * @retval  None
 */
static void fact_reset_cnt_restore(void)
{
	flash_user_get_flash_index(
		        (int*)&reset_cnt_flash_idx,  \
				FLASH_ADR_RESET_CNT, FLASH_SIZE_RESET_CNT, BLOCK_SIZE_RESET_CNT, (u8*)&reset_cnt
			);
	flash_user_restore(reset_cnt_flash_idx,  \
				FLASH_ADR_RESET_CNT, BLOCK_SIZE_RESET_CNT, (u8*)&reset_cnt
			);
}

/**
 * @func   fact_is_active
 * @brief
 * @param  None
 * @retval None
 */
u8 fact_is_active(void)
{
    return fact_mode_par.active_flag;
}

/**
 * @func   fact_handle_result_message
 * @brief
 * @param  None
 * @retval None
 */
static void fact_handle_result(bool result)
{
    if(result == FACT_SUCCESS) {
        fact_mode_par.rf_is_pass = true;
    }
    else {
    	fact_mode_par.rf_is_pass = false;
    }
    DBG_FACT_SEND_STR("\n result: ");
    DBG_FACT_SEND_INT(fact_mode_par.rf_is_pass);
}

/**
 * @func   fact_handle_confirm_sucess
 * @brief
 * @param  None
 * @retval None
 */
static void fact_handle_confirm_sucess(void)
{
    fact_mode_par.active_flag = 1;
    fact_mode_par.active_timeout = FACT_ACT_TIMEOUT_MS_DEFAULT;
    fact_mode_par.active_st_tm = clock_time_ms();
    if(pvFact_handleFactEvent != NULL) {
        pvFact_handleFactEvent(FACT_ACTIVATE);
    }
}

/**
 * @func   fact_init
 * @brief
 * @param  None
 * @retval None
 */
void fact_init(void)
{
    memset(&fact_mode_par, 0, sizeof(fact_mode_par_t));
    result_par.led_st = 0;
    ble_fact_callback_init(fact_handle_result);

    // Power On
	fact_reset_cnt_restore();
	if(reset_cnt > RESET_TRIGGER_VAL) {
		reset_cnt = RESET_CNT_INVALID;
		fact_reset_cnt_store();
	}
}

/**
 * @func   fact_handle_evt_change_callback_init
 * @brief
 * @param  None
 * @retval None
 */
void fact_handle_evt_change_callback_init(
        typeFact_handleFactEvent func_handle_event,
        typeFact_handleExitfactMode func_exit)
{
    if(func_handle_event != NULL) {
        pvFact_handleFactEvent = func_handle_event;
    }
    if(func_exit != NULL) {
        pvFact_handleExitfactMode = func_exit;
    }
}


/**
 * @func   Fact_handleCheckMatchTriggerValue
 * @brief
 * @param  None
 * @retval None
 */
static int fact_check_match_trigger_val(void)
{
    if(reset_cnt == RESET_TRIGGER_VAL){
        fact_clear_reset_cnt();
        bool confirm = false;
        if(is_provision_success() == true) {
        	fact_set(false);
        }
        else {
        	confirm = fact_set(true);
        }
        if(confirm == true) {
        	fact_handle_confirm_sucess();
            DBG_FACT_SEND_STR("\n ___ GO_TO_TEST_MODE ___ : SUCCESS \n");
        }
        return 0;
    }
    return -1;
}

/**
 * @func   fact_increase_reset_cnt
 * @brief
 * @param  None
 * @retval None
 */
static void fact_increase_reset_cnt(void)
{
    if(++reset_cnt > RESET_TRIGGER_VAL) {
        reset_cnt = RESET_CNT_INVALID;
    }
    fact_reset_cnt_store();
}

/**
 * @func   fact_clear_reset_cnt
 * @brief
 * @param  None
 * @retval None
 */
static void fact_clear_reset_cnt(void)
{
    reset_cnt = RESET_CNT_INVALID;
    fact_reset_cnt_store();
}

/**
 * @func   Fact_resetCntCheck
 * @brief
 * @param  None
 * @retval None
 */
static int fact_reset_cnt_check (void)
{
    static u8 clear_st = 2;
    if(0 == clear_st) {
        return 0;
    }
    if((2 == clear_st)  \
            && clock_time_exceed_ms(0, (u32)fact_rst_serials[reset_cnt].tm_start*1000)) {
        clear_st--;
        int trigger = fact_check_match_trigger_val();
        if(trigger != 0) {
            fact_increase_reset_cnt();
        }
        else {
            clear_st = 0;
            fact_clear_reset_cnt();
        }
        DBG_FACT_SEND_STR("\n CNT: ");
        DBG_FACT_SEND_INT(reset_cnt);
    }
    if((1 == clear_st)
            && clock_time_exceed_ms(0, (u32)fact_rst_serials[reset_cnt].tm_stop*1000)) {
        clear_st = 0;
        fact_clear_reset_cnt();
        DBG_FACT_SEND_STR("\n CNT: ");
        DBG_FACT_SEND_INT(reset_cnt);
    }
    return 0;
}

/**
 * @func   fact_handle
 * @brief
 * @param  None
 * @retval None
 */
static void fact_calculator_result(void)
{
    u8 toggle_cnt;
    toggle_cnt = (fact_mode_par.rf_is_pass == true)?4:1;
    // Prepare show result
    result_par.total_cnt = toggle_cnt;
    result_par.toggle_st_time = clock_time_ms();
    result_par.state = G_ON;
    foreach(i, NUMBER_RL) {
        relay_set_target_state(i, G_ON, SRC_DEVICE, false);
    }
    DBG_FACT_SEND_STR("\n FACT_result: ");
    DBG_FACT_SEND_INT(fact_mode_par.rf_is_pass);
}

/**
 * @func   fact_show_led_result
 * @brief
 * @param  None
 * @retval None
 */
static void fact_show_led_result(void)
{
    static u8 cnt_show = 0;
    if(cnt_show++ >= 3) {
        result_par.led_st = (result_par.led_st^1)&1;
        cnt_show = 0;
    }
    else {
        return;
    }
    LedCommand_str led_cmd = COMMAND_LED_DEFAULT;
    led_cmd.ledMask = BACKUP_MASK_RL;
    // Led color
    if(result_par.led_st) {
        if(fact_mode_par.rf_is_pass == true)
        {
        	led_cmd.ledMode  = LED_MODE_ON;
        	led_cmd.ledColor = LED_COLOR_PINK;
        }
        else {
        	led_cmd.ledColor = LED_COLOR_RED;
        	led_cmd.ledMode  = LED_MODE_BLINK;
        	led_cmd.blinkInterval = TIMER_150MS;
        	led_cmd.lastState = LAST_STATE_COLOR_NONE;
        }
    }
    else {
    	led_cmd.ledMode = LED_MODE_OFF;
    }
    (void)led_push_led_command_to_fifo(&led_cmd);
}

/**
 * @func   fact_handle
 * @brief
 * @param  None
 * @retval None
 */
void fact_handle(void)
{
	if(!clock_time_exceed_ms(0, fact_rst_serials[0].tm_start))
	{
		return;
	}
    // calculator reset counter
    fact_reset_cnt_check();
    // active
    if(fact_mode_par.active_flag) {
        if(!fact_mode_par.active_show_result_flag) {
            if(clock_time_exceed_ms(  \
                    fact_mode_par.active_st_tm, FACT_SHOW_RESULT_ST_TIME)) {
                fact_mode_par.active_show_result_flag = 1;
                // Calculator result
                fact_calculator_result();
            }
        }
        else {
            if(clock_time_exceed_ms( \
                    fact_mode_par.active_st_tm, FACT_ACT_TIMEOUT_MS_DEFAULT)) {
                fact_mode_par.active_flag = 0;
                DBG_FACT_SEND_STR("\n *** FACT TIMEOUT ***");
                if(pvFact_handleExitfactMode != NULL) {
                    pvFact_handleExitfactMode();
                }
                led_refresh(0xFFFF);
            }
            if(fact_mode_par.active_flag == 1)
            {
            	// Show result
				if(clock_time_exceed_ms   \
						(result_par.toggle_st_time, FACT_TOGGLE_INTERVAL)) {
					if(result_par.total_cnt != 0) {
						if(result_par.state == G_ON) {
							result_par.state = G_OFF;
							result_par.total_cnt--;
						}
						else {
							result_par.state = G_ON;
						}
						foreach(i, NUMBER_RL) {
							relay_set_target_state(i, result_par.state, SRC_DEVICE, false);
						}
					}
					else {
						fact_show_led_result();
					}
					result_par.toggle_st_time = clock_time_ms();
				}
            }
        }
    }
}
