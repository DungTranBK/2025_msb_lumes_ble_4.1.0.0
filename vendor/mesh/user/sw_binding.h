/*
 * sw_binding.h
 *
 *  Created on: Jan 28, 2021
 *      Author: DungTranBK
 */

#ifndef SW_BINDING_H_
#define SW_BINDING_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../../proj/tl_common.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

typedef struct {
	u8  st;
	u8  cnt_same;
	u16 op;
}control_binding_t;

typedef struct{
	u16 ele_adr;
	u8  en;
	u16 group_adr;
}cfg_binding_format_t;

typedef struct{
	u16 ele_adr;
}get_binding_format_t;

typedef struct {
	u8  en;
	u16 group_dst;
}binding_para_t;

typedef struct {
	u8  op;
	u8  st;
	u16 ele_adr;
	u8  en;
	u16 group_adr;
}binding_msg_response_t;

enum {
	BINDING_SUCCESS      = 0,
	GROUP_NOT_SET        = 1,
	GROUP_INVALID        = 2,
	MODEL_IDX_INVALID    = 3,
	BINDING_ERR_UNKNOWN  = 4,
	BINDING_DISABLE      = 5
};

extern binding_para_t binding_para_st[ELE_CNT];
extern control_binding_t control_binding_st[ELE_CNT];
extern u8 force_control_binding[ELE_CNT];

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/

uint16_t get_group_binding_adr(int idx);
void binding_init(void);
void binding_handle_nw_message(u8 type, u8* par);

#endif /* SW_BINDING_H_ */
