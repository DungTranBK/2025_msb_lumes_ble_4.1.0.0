/*
 * factory_reset.h
 *
 *  Created on: Dec 25, 2019
 *      Author: DungTran BK
 */

#ifndef FACTORY_RESET_H_
#define FACTORY_RESET_H_

#include "../../../proj/tl_common.h"


typedef struct{
	u8  flag;
	u32 startTimeDelay;
	u32 delayTime;
}FactoryReset_str;

#define MAX_FACTORY_RESET_DELAY_MS    10000

u8 get_reset_cnt ();
void reset_cnt_get_idx ();
void start_factory_reset(void);
void setup_factory_reset_with_delay(u8 enable);
void handle_factory_reset_with_delay(void);

#endif /* FACTORY_RESET_H_ */
