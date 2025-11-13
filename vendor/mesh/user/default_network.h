/*
 * default_network.h
 *
 *  Created on: 2021
 *      Author: Thuan Nguyen
 */



#define OPEN_DF_NW_TIMEOUT         1800 // 30 minutes
#define OPEN_DF_NW_DEFAULT_TIME    60   // s


u8 default_network_is_present(void);
void set_tmp_keys(u8 tmp);
void del_tmp_keys();
void proc_default_network();
void set_default_network_manual(bool en, u16 time_s);
void check_and_del_default_power_on(void);
