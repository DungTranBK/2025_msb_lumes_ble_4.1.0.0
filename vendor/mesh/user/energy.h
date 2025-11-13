/*
 * energy.h
 *
 *  Created on: Oct 4, 2023
 *      Author: DungTranBK
 */

#ifndef ENERGY_H_
#define ENERGY_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/
typedef struct {
	u8  type;
	u32 wh;
	u8  pf;
}act_pw_rsp_t;

typedef struct {
	u8 type;
	u16 i;
	u16 v;
	u16 p;
	u8  temp;
}iup_pf_response_t;

enum energy_report_type_enum{
	ENERGY_I_U_P_PF = 0,
	ENERGY_A = 1,
};
typedef u8 energy_report_type_enum;

#define OVER_CURRENT_DETECT     0

/*
 * Definition
 */
#define ENERGY_REPORT_DELAY_OFFSET        (1*1000)   // 1 s
#define ENERGY_REPORT_RANDOM_OFFSET       (30*1000)  // 1 minutes

#define ACTIVE_ENERGY_RANDOM_OFFSET       (15*1000)  // 15s

#define MACRO_ENERGY_REPORT_TIME          (ENERGY_REPORT_DELAY_OFFSET + rand()%ENERGY_REPORT_RANDOM_OFFSET)

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void energy_init(void);
void energy_proc(void);

#endif /* ENERGY_H_ */
