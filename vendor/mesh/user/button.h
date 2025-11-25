/*
 * button.h
 *
 *  Created on: Sep 21, 2020
 *      Author: DungTran BK
 */

#ifndef BUTTON_H_
#define BUTTON_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define BUTTON_SCENE_0_IDX      0
#define BUTTON_SCENE_1_IDX      1
#define BUTTON_SWITCH_0_IDX     2
#define BUTTON_SWITCH_1_IDX     3
#define BUTTON_SWITCH_2_IDX     4

typedef struct {
	bool  is_active;
	u8    step;
	u32   change_step_last_t;
	u32   is_active_st_time;
}para_btn_reset_t;

enum Step_Reset_Enum
	{
		STEP_IDLE,
		STEP_HOLD_5S,  // wait release
		STEP_HOLD_5S_RELEASE, // wait press one time
		STEP_INVALID,
	};

#define CONFIRM_RESET_TIMEOUT_MS         TIMER_5S
#define BLINK_HOLD_2S_INTERVAL           TIMER_100MS

extern btn_evt_t touch_btn_before_st[NUMBER_BUTTON];

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void button_handle_option_btn_state(u8 idx, u8 evt);
void button_check_clean_reset_param(void);
void button_check_clean_default_param(void);

#endif /* BUTTON_H_ */
