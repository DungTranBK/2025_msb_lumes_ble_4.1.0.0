/*
 * user_irq.h
 *
 *  Created on: Oct 17, 2020
 *      Author: DungTran BK
 */

#ifndef USER_IRQ_H_
#define USER_IRQ_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "config_board.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

typedef void (*typeIrq_HandleTimer1IrqCallbackFunc)(void);
typedef void (*typeIrq_HandleGpioIrqCallbackFunc)(void);

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
#if IRQ_TIMER1_ENABLE
void timer1_update_cmp_value(u16 compare_us);
void timer1_hw_config(void);
void timer1_enable(void);
void timer1_disable(void);
void timer1_irq_callback_init(typeIrq_HandleTimer1IrqCallbackFunc callbackFunc);
_attribute_ram_code_ void irq_timer_handle(void);
#endif

#if	IRQ_GPIO_ENABLE
void irq_gpio_handle();
void gpio_irq_callback_init(typeIrq_HandleGpioIrqCallbackFunc callbackFunc);
#endif

#endif /* USER_IRQ_H_ */
