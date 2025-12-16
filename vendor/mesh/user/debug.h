/*
 * debug.h
 *
 *  Created on: Sep 30, 2019
 *      Author: DungTran BK
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#if MY_PRINTF_DEBUG_EN
//  #define TEST_AUTO_DBG_EN
//  #define CUSTOM_DBG_EN
//  #define TEST_LED_DBG_EN
//  #define AUTO_DBG_EN
//  #define APP_BEACON_DBG_EN
//  #define CMD_INTER_DBG_EN
//	#define FACTORY_RST_DBG_EN
//	#define LIGHT_DBG_EN
//	#define GEN_MODEL_DBG_EN
//	#define MESH_COMMON_DBG_EN
//	#define MODEL_HSL_DBG_EN
//	#define LIGHTING_MODEL_DBG_EN
//	#define MODEL_LC_DBG_EN
//	#define MODEL_XYL_DBG_EN
//	#define MESH_FN_DBG_EN
//	#define MESH_LPN_DBG_EN
//	#define MESH_NODE_DBG_EN
//	#define MESH_OTA_DBG_EN
//	#define REMOTE_PROV_DBG_EN
//	#define SCENE_DBG_EN
//	#define SCHEDULER_DBG_EN
//	#define SENSOR_MODEL_DBG_EN
//	#define SYSTEM_DBG_EN
//	#define TIME_DBG_EN
//	#define USER_ALI_DBG_EN
//	#define USER_PROC_DBG_EN
   #define VENDOR_MODEL_DBG_EN
//	#define BLE_LL_OTA_DBG_EN
//	#define BLT_LED_DBG_EN
//	#define BLT_SOFT_TIMER_DBG_EN
//	#define CMD_INTERFACE_DBG_EN
//	#define DUAL_MODE_DBG_EN
//	#define FAST_PROV_DBG_EN
//	#define SERIAL_DBG_EN
//  #define NET_MSG_DBG_EN
//  #define BUTTON_DBG_EN
//  #define SW_CONFIG_DBG_EN
//  #define EXECUTION_SCENE_DBG_EN
//  #define BIND_CONFIG_DBG_EN
//  #define LED_EV_DBG_EN
//  #define CURTAIN_DBG_EN
//  #define DIMMER_DBG_EN
//  #define FAN_DBG_EN
//  #define GATE_DBG_EN
//  #define FS_USER_DBG_EN
//  #define BIND_CONFIG_DBG_EN
//  #define SW_AUTO_DBG_EN
//#define FAST_PROVISION_DBG_EN
//#define DEFAULT_NETWORK_DBG_EN
//#define DIMMING_DBG_EN
//#define INTER_PROC_DBG_EN
 // #define BIND_DBG_EN
  //#define APP_DBG_EN
//#define APP_ATT_DBG_EN
//#define GEN_MODEL_DBG_EN
//#define LED_DBG_EN
//#define BTN_DBG_EN
//#define BUTTON_DBG_EN
#define RELAY_DBG_EN
//#define NET_MSG_DBG_EN
//#define REPORT_DBG_EN
//#define SW_CONFIG_DBG_EN
//#define LVD_DBG_EN
//#define IRQ_DBG_EN
//#define LOCK_SCH_DBG_EN
// #define TIMESTAMP_DBG_EN
//#define GPIO_DBG_EN
//#define LED_EV_DBG_EN
//#define LED_DBG_EN
//#define FACT_DBG_EN
//#define SW_DBG_EN
//#define INFO_DBG_EN
  #define BL0906_DBG_EN
  #define ENERGY_DBG_EN
#endif

#define DBG_FLOAT_EN   0

void Dbg_sendString(s8 *s);
void Dbg_sendDword(u32 data);
void Dbg_sendInt(u16 data);
#if DBG_FLOAT_EN
void Dbg_sendFloat(float data);
#endif
void Dbg_sendHex(u16 data);
void Dbg_sendByte(u8 data);
void Dbg_sendOneByteHex(s8 data);
void Dbg_sendHexOneByte(u16 data);
void Dbg_sendHex32(u32 data);

#endif /* DEBUG_H_ */
