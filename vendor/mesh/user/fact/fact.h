/*
 * fact.h
 *
 *  Created on: Sep 30, 2024
 *      Author: DungTranBK
 */

#ifndef FACT_H_
#define FACT_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/
enum {
    FACT_CONFIRM,
    FACT_ACTIVATE,
    FACT_REFRESH_LED,
};

enum {
    RESULT_FAIL_ALL,
    RESULT_PASS_ZIGBEE,
    RESULT_PASS_BLETOOTH,
    RESULT_PASS_ALL,
    RESULT_UNKNOWN,
};

typedef void (*typeFact_handleControlRelay)(u8 idx, u8 st, u8 src, bool store);

typedef void (*typeFact_handleTestStatus)(bool);

typedef void (*typeFact_handleFactEvent)(uint8_t);
typedef void (*typeFact_handleExitfactMode)(void);
typedef uint8_t (*typeFact_restoreResetCnt)(void);
typedef void (*typeFact_storeResetCnt)(uint8_t);

extern bool i2c_is_pass;
extern bool calib_is_pass;

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/
void fact_set_energy_status(bool is_pass);
void fact_init(void);
void fact_callback_result_status_init(typeFact_handleTestStatus func);
void fact_callback_control_relay_init(typeFact_handleControlRelay func);
void fact_handle_evt_change_callback_init(
        typeFact_handleFactEvent func_handle_event,
        typeFact_handleExitfactMode func_exit);
void fact_callback_init(typeFact_restoreResetCnt funcRestoreResetCnt,
        typeFact_storeResetCnt funcStoreResetCnt);
void fact_handle(void);

u8 fact_is_wait_confirm(void);
u8 fact_is_active(void);
void fact_handle_confirm_message(u8 channel, u8* par);
void fact_handle_result_message(u8 channel, u8* par);

#endif /* FACT_H_ */
