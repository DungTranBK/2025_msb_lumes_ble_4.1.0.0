/*
 * relay.h
 *
 *  Created on: Aug 21, 2024
 *      Author: DungTranBK
 */

#ifndef RELAY_H_
#define RELAY_H_
/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "config_board.h"
#include "utilities.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

enum
{
	NORMAL_RELAY,
	LATCHING_RELAY,
	RELAY_TYPE_UNKNOWN,
};
typedef u8 Relay_Type_Enum;

#define	NORMAL_RL_ON_DELAY	     5200
#define	NORMAL_RL_OFF_DELAY      5200

#define	LATCHING_RL_ON_DELAY	 4000
#define	LATCHING_RL_OFF_DELAY    4000

#define ADJUST_60HZ_US           300


#if   NUMBER_RL == 1
    // RL0
	#define PIN_ON_RL0     PIN_CONTROL_RL0_ON
	#define PIN_OFF_RL0    PIN_CONTROL_RL0_OFF
#elif NUMBER_RL == 2
    // RL0
	#define PIN_ON_RL0     PIN_CONTROL_RL0_ON
	#define PIN_OFF_RL0    PIN_CONTROL_RL0_OFF
    // RL1
	#define PIN_ON_RL1     PIN_CONTROL_RL1_ON
	#define PIN_OFF_RL1    PIN_CONTROL_RL1_OFF
#elif NUMBER_RL == 3
    // RL0
	#define PIN_ON_RL0     PIN_CONTROL_RL0_ON
	#define PIN_OFF_RL0    PIN_CONTROL_RL0_OFF
    // RL1
	#define PIN_ON_RL1     PIN_CONTROL_RL1_ON
	#define PIN_OFF_RL1    PIN_CONTROL_RL1_OFF
    // RL2
	#define PIN_ON_RL2     PIN_CONTROL_RL2_ON
	#define PIN_OFF_RL2    PIN_CONTROL_RL2_OFF
#endif


#if   NUMBER_RL == 1
	#define CONTROL_PIN_ARR {  \
			    { PIN_ON_RL0, PIN_OFF_RL0 }, \
		    };
#elif NUMBER_RL == 2
	#define CONTROL_PIN_ARR {  \
			    { PIN_ON_RL0, PIN_OFF_RL0 }, \
			    { PIN_ON_RL1, PIN_OFF_RL1 }, \
		    };
#elif NUMBER_RL == 3
	#define CONTROL_PIN_ARR {  \
			    { PIN_ON_RL0, PIN_OFF_RL0 }, \
			    { PIN_ON_RL1, PIN_OFF_RL1 }, \
			    { PIN_ON_RL2, PIN_OFF_RL2 }, \
		    };
#endif


enum {
	RELAY_IDLE = 0,
	RELAY_BUSY = 1,
};
typedef uint8_t RelayModuleStatus_Enum;

enum RelayControlSuccess_enum{
	CONTROL_IN_PROCESSING = 0,
	CONTROL_SUCCESS = 1,
};
typedef uint8_t RelayControlSuccess_enum;

enum {
	POWER_ONE_WIRE,
	POWER_TWO_WIRE,
	POWER_TYPE_UNKNOWN,
};
typedef uint8_t Power_Type_Enum;

typedef struct {
	u8  code;
	u32 on_time;
}relay_on_time_rsp_t;

#define NO_RL_CHN_ACTIVE      0xFF

#define RL_ON          1
#define RL_OFF         0

#define CONTROL_RL_INTERVAL_MS                  TIMER_500MS
#define WAIT_ZERO_POINT_TIMEOUT_MS              TIMER_70MS
#define STORE_RELAY_STATE_TIME_LEN_MS           TIMER_3S
#define DETECT_POWER_TYPE_TIMEOUT               TIMER_5S
#define MODULE_BYSY_TIME_OUT_MS                 TIMER_10S

#define DETECT_FREQ_CNT_DEF                     10

#define CONTROL_RL_TIME_LEN_US                  10000

// Relay On Time Report
#define RELAY_ON_TIME_PUBLISH_RANDOM_TIME_S	    (12*60*60)  // 12h
#define RELAY_ON_TIME_PUBLISH_TIME_S	        (12*60*60)  // 12h

#define RELAY_ON_TIME_PUBLISH_RANDOM_TIME_PW_ON	(5*60)      // 5 minutes
#define RELAY_ON_TIME_PUBLISH_TIME_PW_ON	    (5*60)      // 5 minutes


#define MINIMUM_CNT_ISR_TO_DETECT_PW_TYPE        70

typedef int (*typeRL_handle_relay_state_change)(u8 idx, u8 st);

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/

void relay_callback_init(typeRL_handle_relay_state_change func);
void relay_init(void);
u8 relay_proc(void);
void relay_handle_refresh(u16 mask);
u8 relay_get_present_state(u8 idx);
void relay_set_target_state(u8 idx, u8 st, src_control_enum src, bool store);
void relay_toggle_state(u8 idx, src_control_enum src);

u16 relay_get_target_state(void);
u8 relay_get_target_state_and_response_by_index(u8 idx);
void relay_deinit_after_fact(void);
void relay_store_total_relay_on_time(void);

void relay_handle_delete_relay_on_time(u8 idx);

#endif /* RELAY_H_ */
