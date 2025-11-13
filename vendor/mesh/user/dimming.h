/*
 * dimming.h
 *
 *  Created on: Jan 22, 2022
 *      Author: DungTranBK
 */

#ifndef DIMMING_H_
#define DIMMING_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "config_board.h"
/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

enum dimming_on_type_enum {
	ON_RESTORE,
	ON_SET_UP_VALUE,
	ON_INVALID,
};
typedef u8 dimming_on_type_enum;

enum dimming_en_enum {
	DIMMING_DISABLE,
	DIMMING_ENABLE,
	DIMMING_EN_INVALID
};

#define DIMMING_LIGHTNESS_MIN    0x2628  // s16 is -23000
#define DIMMING_LIGHTNESS_MAX    0xFFFF

#define DIMMING_TRANS_T_MIN      0x0A
#define DIMMING_TRANS_T_DEFAULT  0x32

typedef struct {
	u8  on_type;
	u16 on_value;
	u8  tran_t;
}dim_config_set_t;

extern dim_config_set_t dimming_config[NUMBER_INPUT];
extern u8 cotrol_binding_locally_flag[NUMBER_INPUT];

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/

void dimming_init(void);
void dimming_active(u8 idx, u8 dir, u16 dst_addr);
int  dimming_configuration_set(u8 idx, u8* par, u8 par_len);
int  dimming_configuration_get(u8 idx);
bool dimming_mod_lightness(u8 idx, u16* lightness);
void dimming_handle_btn_st(u8 model_idx, u8 evt);

#endif /* DIMMING_H_ */
