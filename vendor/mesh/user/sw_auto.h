/*
 * sw_auto.h
 *
 *  Created on: Feb 22, 2021
 *      Author: DungTranBK
 */

#ifndef SW_AUTO_H_
#define SW_AUTO_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj/tl_common.h"
/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

typedef void (*type_control_dev_callback_func)(u16, bool, bool);

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/

void auto_reset_time_trans(u16 idx, u8 st);
void auto_loop_task(void);
void auto_callback_init(type_control_dev_callback_func callbackFunc) ;

#endif /* SW_AUTO_H_ */
