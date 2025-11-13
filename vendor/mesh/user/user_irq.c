/*
 * user_irq.c
 *
 *  Created on: Oct 17, 2020
 *      Author: DungTran BK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../../proj/tl_common.h"
#include "user_irq.h"

//#include "debug.h"
#ifdef IRQ_DBG_EN
#define DBG_IRQ_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_IRQ_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_IRQ_SEND_HEX(x)   Dbg_sendHex(x)
#else
#define DBG_IRQ_SEND_STR(x)
#define DBG_IRQ_SEND_INT(x)
#define DBG_IRQ_SEND_HEX(x)
#endif

typeIrq_HandleTimer1IrqCallbackFunc  pv_HandleTimer1IrqCallbackFunc = NULL;
typeIrq_HandleGpioIrqCallbackFunc pv_HandleGpioIrqCallbackFunc = NULL;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/


/******************************************************************************/
/*                            PRIVATE FUNCTION                                */
/******************************************************************************/


/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/


#if IRQ_TIMER1_ENABLE

/**
 * @func   timer1_update_cmp_value
 * @brief
 * @param  None
 * @retval None
 */
void timer1_update_cmp_value(u16 compare_us)
{
	timer1_disable();
	reg_tmr1_tick = 0;
	reg_tmr1_capt = CLOCK_MCU_RUN_CODE_1US * compare_us;
}

/**
 * @func   hw_timer1_config
 * @brief
 * @param  None
 * @retval None
 */
void timer1_hw_config(void)
{
    reg_irq_mask |= FLD_IRQ_TMR1_EN;
    reg_tmr1_tick = 0;
    reg_tmr1_capt = CLOCK_MCU_RUN_CODE_1US * IRQ_TIME1_INTERVAL ;
    reg_tmr_ctrl |= FLD_TMR1_EN;
}

/**
 * @func   timer1_enable
 * @brief
 * @param  None
 * @retval None
 */
void timer1_enable(void)
{
	reg_irq_mask |= FLD_IRQ_TMR1_EN;
	reg_tmr_ctrl |= FLD_TMR1_EN;
}

/**
 * @func   timer1_disable
 * @brief
 * @param  None
 * @retval None
 */
void timer1_disable(void)
{
	reg_irq_mask &= ~FLD_IRQ_TMR1_EN;
	reg_tmr_ctrl &= ~FLD_TMR1_EN;
}

/**
 * @func   irq_timer_handle
 * @brief  ISR for TIMER1
 * @param  None
 * @retval None
 */
_attribute_ram_code_ void irq_timer_handle(void)
{
    u32 src = reg_irq_src;
    if(src & FLD_IRQ_TMR1_EN){
       reg_tmr_sta = FLD_TMR_STA_TMR1;
       if(pv_HandleTimer1IrqCallbackFunc != NULL){
    	   pv_HandleTimer1IrqCallbackFunc();
       }
    }
}

/**
 * @func   timer1_irq_callback_init
 * @brief
 * @param  CB
 * @retval None
 */
void timer1_irq_callback_init(typeIrq_HandleTimer1IrqCallbackFunc callbackFunc)
{
	if(callbackFunc != NULL){
		pv_HandleTimer1IrqCallbackFunc = callbackFunc;
	}
}

#endif


#if	IRQ_GPIO_ENABLE

/**
 * @func   gpio_irq_user_handle
 * @brief
 * @param  CB
 * @retval None
 */
void gpio_irq_user_handle()
{
	if(pv_HandleGpioIrqCallbackFunc != NULL){
		pv_HandleGpioIrqCallbackFunc();
	}
	return;
}
/**
 * @func   gpio_risc0_user_handle
 * @brief
 * @param  CB
 * @retval None
 */
void gpio_risc0_user_handle()
{
	return;
}
/**
 * @func   gpio_risc1_user_handle
 * @brief
 * @param  CB
 * @retval None
 */
void gpio_risc1_user_handle()
{
	return;
}
/**
 * @func   gpio_risc2_user_handle
 * @brief
 * @param  CB
 * @retval None
 */
void gpio_risc2_user_handle()
{
	return;
}
/**
 * @func   irq_gpio_handle
 * @brief
 * @param  CB
 * @retval None
 */
void irq_gpio_handle()
{
	u32 src = reg_irq_src;
	if(src & FLD_IRQ_GPIO_EN){
		gpio_irq_user_handle();
		reg_irq_src = FLD_IRQ_GPIO_EN;              // clear irq_gpio irq flag
	}

	/************* gpio irq risc0 *************/
	if(src & FLD_IRQ_GPIO_RISC0_EN){
		gpio_risc0_user_handle();
		reg_irq_src = FLD_IRQ_GPIO_RISC0_EN;        // clear irq_gpio irq flag
	}

	/************* gpio irq risc1 *************/
	if(src & FLD_IRQ_GPIO_RISC1_EN){
		gpio_risc1_user_handle();
		reg_irq_src = FLD_IRQ_GPIO_RISC1_EN;        // clear irq_gpio irq flag
	}
	#if (!(__TL_LIB_8258__ || (MCU_CORE_TYPE && MCU_CORE_TYPE == MCU_CORE_8258) || (MCU_CORE_TYPE == MCU_CORE_8278)))
	if(src & FLD_IRQ_GPIO_RISC2_EN){
		gpio_risc2_user_handle();
		reg_irq_src = FLD_IRQ_GPIO_RISC2_EN;
	}
	#endif
}

/**
 * @func   gpio_irq_callback_init
 * @brief
 * @param  CB
 * @retval None
 */
void gpio_irq_callback_init(typeIrq_HandleGpioIrqCallbackFunc callbackFunc)
{
	if(callbackFunc != NULL){
		pv_HandleGpioIrqCallbackFunc = callbackFunc;
	}
}


#endif


// End File
