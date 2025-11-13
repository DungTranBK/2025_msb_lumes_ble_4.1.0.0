/*
 * key_report.h
 *
 *  Created on: Feb 27, 2020
 *      Author: DungTran BK
 */

#ifndef KEY_REPORT_H_
#define KEY_REPORT_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "utilities.h"
/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/
enum ButtonKey_enum
	{
		BTN_KEY_PRESS_1_TIME   = 0,
		BTN_KEY_PRESS_2_TIMES  = 1,
		BTN_KEY_HOLD_2_SECONDS = 4,
		EV_SW_ON               = 5,
		EV_SW_OFF              = 6,
		BTN_KEY_HOLD_UP        = 8,
		BTN_KEY_HOLD_DOWN      = 9,
		BTN_KEY_HOLD_UP_DOWN   = 10,
		EV_BTN_MAX             = 8,   // Total
	};
typedef u8 ButtonKey_enum;

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/

void key_report_send(u8 idx, u8 key_code, bool en_execution);
u16 get_ele_addr_base_btn_idx(uint8_t idx);

#endif /* KEY_REPORT_H_ */
