/*
 * timestamp.h
 *
 *  Created on: Feb 7, 2023
 *      Author: DungTranBK
 */

#ifndef VENDOR_USER_TIMESTAMP_H_
#define VENDOR_USER_TIMESTAMP_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "tl_common.h"

/******************************************************************************/

/******************************************************************************/
/*                       EXPORT TYPE AND DEFINITION                           */
/******************************************************************************/

#define INVALID_SEC                    MAX_U32
#define INVALID_MINUTE                 MAX_U16
#define INVALID_WEEK_DAY               8
#define SET_TIMESTAMP_MAX_INTERVAL_S  (7*24*60*60)  // 7 days, external OSC

#define MININUTE_RANGE_IN_DAY          1439

typedef struct {
	u8  cmd;
	u32 value;
}timestamp_rsp_t;

typedef struct
	{
		u8  cmd;
		u16 dst_adr;
	}timestamp_query_t;

/******************************************************************************/
/*                              EXPORT FUNCTION                               */
/******************************************************************************/
bool timestamp_device_is_have_time(void);
void timestamp_send_query_time(u16 src);
void timestamp_get(u16 adr_dst, u16 adr_src);
int timestamp_set(u8 model_idx, u8* par, u8 par_len);
void timestamp_proc();
u16 timestamp_get_minute_in_day(void);
u8 timestamp_get_week_day(void);

#endif /* VENDOR_USER_TIMESTAMP_H_ */
