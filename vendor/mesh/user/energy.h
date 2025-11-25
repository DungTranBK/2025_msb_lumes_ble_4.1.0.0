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
	u8  code;
	u8  status;
	u32 wh;
}act_pw_rsp_t;

typedef struct {
	u8 code;
	u16 i;
	u16 v;
	u8  p[3];
}iup_pf_response_t;

enum energy_report_type_enum{
	ENERGY_I_U_P_PF = 0,
	ENERGY_A = 1,
};
typedef u8 energy_report_type_enum;

#define OVER_CURRENT_DETECT     0


#define VOLTAGE_RMS_MIN                                 200000   // mili_voltage
#define VOLTAGE_RMS_MAX                                 240000

/*
 * Definition
 */
#define IUP_REPORT_DELAY_OFFSET                         (5*1000)   // 5 s
#define IUP_REPORT_RANDOM_OFFSET                        (25*1000)  // 25 s

#define ACTIVE_ENERGY_DELAY_OFFSET                      (5*1000)   // 5 s
#define ACTIVE_ENERGY_RANDOM_OFFSET                     (25*1000)  // 25s

#define ENERGY_FAST_REPORT_MS                           (3*1000)  // 3s + random()%3s

#define IUP_PUBLISH_PERIODIC_DELAY_OFFSET               (115*60)   // 1h55
#define IUP_PUBLISH_PERIODIC_RANDOM_OFFSET              (5*60)     // 5 minutes

#define ACTIVE_ENERGY_PUBLISH_PERIODIC_DELAY_OFFSET     (55*60)     // 55 minutes
#define ACTIVE_ENERGY_PUBLISH_PERIODIC_RANDOM_OFFSET    (5*60)      // 5 minutes

#define MINIMUM_ACTIVE_POWER_MW                         (5*1000)
#define MINIMUM_ACTIVE_CURRENT_MA                       (20)

#define MINIMUM_ACTIVE_ENERY_NEED_REPORT_AFTER_RL_OFF   (5)         // Wh


#define MACRO_IUP_REPORT_TIME           (IUP_REPORT_DELAY_OFFSET + rand()%IUP_REPORT_RANDOM_OFFSET)
#define MACRO_ACTIVE_ENERGY_REPORT_TIME (ACTIVE_ENERGY_DELAY_OFFSET + rand()%ACTIVE_ENERGY_RANDOM_OFFSET)

typedef u8 (*typeEnergy_get_actual_relay_state)(u8 idx);

enum {
	RP_SRC_UNKNOWN,
	RP_SRC_CHANGE_ST,
	RP_SRC_DELTA,
	RP_SRC_PERIODIC,
	RP_SRC_QUERY,
};
typedef u8 SourceReport_Enum;

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void store_active_power(void);
void energy_clean_all_active_power(void);
void energy_callback_init(typeEnergy_get_actual_relay_state func);
void energy_init(void);
void energy_proc(void);
void energy_handle_relay_state_change(u8 idx, u8 st, bool is_fast);

void energy_setup_publish_energy_delay(u8 idx, u32 wait_t_ms);
void energy_setup_publish_iup_pf_delay(u8 idx, u32 wait_t_ms, SourceReport_Enum src);

void energy_handle_delete_power_information(u8 idx);

#endif /* ENERGY_H_ */
