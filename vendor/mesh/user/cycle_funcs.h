/*
 * cycle_funcs.h
 *
 *  Created on: May 11, 2021
 *      Author: DungTranBK
 */

#ifndef CYCLE_FUNCS_H_
#define CYCLE_FUNCS_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define CYCLE_FUNC_LIST_MAX      16

typedef void (*cycle_handle_func)(void);

typedef struct {
	uint32_t time_start;
	uint32_t time_length;
	cycle_handle_func pfunc;
	uint16_t is_used;
}cycle_funcs_info_t;

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/
void CycleFunc_init(void);
void CycleFunc_add(cycle_handle_func pfunc, uint32_t time_length);
void CycleFunc_remove(cycle_handle_func pfunc);
void CycleFunc_proc(void);
#endif /* CYCLE_FUNCS_H_ */
