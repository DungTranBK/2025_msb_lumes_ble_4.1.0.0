/*
 * cycle_funcs.c
 *
 *  Created on: May 11, 2021
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "tl_common.h"
#include "vendor/mesh/user/utilities.h"
#include "vendor/common/system_time.h"
#include "cycle_funcs.h"

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static cycle_funcs_info_t cycle_funcs_info[CYCLE_FUNC_LIST_MAX];

/******************************************************************************/
/*                            PRIVATE FUNCTIONS                               */
/******************************************************************************/
static BOOL cycle_func_initialized = false;
int index_func = 0;

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/
/**
* @func   CycleFunc_init 
* @brief  
* @param  
* @retval None
*/
void CycleFunc_init(void)
{
	foreach(index_func, CYCLE_FUNC_LIST_MAX) {
		cycle_funcs_info[index_func].pfunc = NULL;
		cycle_funcs_info[index_func].is_used = false;
	}
	cycle_func_initialized = true;
}

/**
* @func   CycleFunc_add 
* @brief  
* @param  
* @retval None
*/
void CycleFunc_add(cycle_handle_func pfunc, uint32_t time_length)
{
	foreach(index_func, CYCLE_FUNC_LIST_MAX) {
		if(cycle_funcs_info[index_func].is_used == true) {
			if(cycle_funcs_info[index_func].pfunc == pfunc) {
				// UPDATE TIME STEP
				cycle_funcs_info[index_func].time_start = clock_time_ms();
				// UPDATE TIME LENGTH
				cycle_funcs_info[index_func].time_length = time_length;
				return;
			}
		}
	}
	foreach(index_func, CYCLE_FUNC_LIST_MAX) {
		if(cycle_funcs_info[index_func].is_used == false) {
			cycle_funcs_info[index_func].is_used = true;
			cycle_funcs_info[index_func].pfunc = pfunc;
			cycle_funcs_info[index_func].time_start = clock_time_ms();
			cycle_funcs_info[index_func].time_length = time_length;
			break;
		}
	}
}

/**
* @func   CycleFunc_remove 
* @brief  
* @param  
* @retval None
*/
void CycleFunc_remove(cycle_handle_func pfunc)
{
	foreach(index_func, CYCLE_FUNC_LIST_MAX) {
		if(cycle_funcs_info[index_func].is_used == true) {
			if(cycle_funcs_info[index_func].pfunc == pfunc) {
				cycle_funcs_info[index_func].pfunc = NULL;
				cycle_funcs_info[index_func].is_used = false;
			}
		}
	}
}

/**
* @func   CycleFunc_proc 
* @brief  
* @param  
* @retval None
*/
void CycleFunc_proc(void)
{
	if(cycle_func_initialized == false){
		return;
	}
	foreach(index_func, CYCLE_FUNC_LIST_MAX) {
		if(cycle_funcs_info[index_func].is_used == true) {
			if(clock_time_exceed_ms(
					cycle_funcs_info[index_func].time_start, cycle_funcs_info[index_func].time_length)) {
				cycle_funcs_info[index_func].time_start = clock_time_ms();
				if(cycle_funcs_info[index_func].pfunc != NULL) {
					cycle_funcs_info[index_func].pfunc();
				}
				else {
					CycleFunc_remove(cycle_funcs_info[index_func].pfunc);
				}
			}
		}
	}
}

// End
