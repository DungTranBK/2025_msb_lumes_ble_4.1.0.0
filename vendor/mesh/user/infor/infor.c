/*
 * infor.c
 *
 *  Created on: Oct 2, 2024
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "tl_common.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/system_time.h"
#include "vendor/mesh/user/utilities.h"
#include "uart_simu.h"
#include "infor.h"

#include "../debug.h"
#ifdef  INFO_DBG_EN
#define DBG_INFO_SEND_STR(x)     Dbg_sendString((s8*)x)
#define DBG_INFO_SEND_INT(x)     Dbg_sendInt(x)
#define DBG_INFO_SEND_HEX(x)     Dbg_sendHex(x)
#define DBG_INFO_SEND_HEX_ONE(x) Dbg_sendHexOneByte(x);
#else
#define DBG_INFO_SEND_STR(x)
#define DBG_INFO_SEND_INT(x)
#define DBG_INFO_SEND_HEX(x)
#define DBG_INFO_SEND_HEX_ONE(x)
#endif


typedef struct {
	u8 protocol;
	u8 device_type;
	u8 endpoint_cnt;
	u8 prov_state;
	u8 MAC[8];
	u8 version[3];
	u16 PID;
}device_infor_t;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

#define SOF_HI    0x4C
#define SOF_LO    0x4D

#define CMD_ID_RSP_HOST_INFO            0xFF

#define PUSH_INFO_TIMEOUT_MS            TIMER_15S
#define PUSH_INFO_INTERVAL_MS           TIMER_1S
#define PUSH_INFO_START_TIME_MS         TIMER_3S

#define DEVICE_TYPE                     DEV_PHYSICAL_SWITCH

static u32 push_infor_st_time_ms = 0;
static bool push_info_enable_flag = true;

/******************************************************************************/
/*                           EXPORT FUNCTIONS                                 */
/******************************************************************************/

/**
 * @func    info_init
 * @brief
 * @param   None
 * @retval  None
 */
void info_init(void)
{
	push_infor_st_time_ms = clock_time_ms();
}

/**
 * @func    info_push_to_information_to_debug_pin
 * @brief
 * @param   None
 * @retval  None
 */
static void info_push_to_information_to_debug_pin(void)
{
	device_infor_t device_info;
	device_info.protocol = PROTO_BLUETOOH;
	device_info.device_type = DEVICE_TYPE;
	device_info.endpoint_cnt = ELE_CNT;
	device_info.prov_state = get_provision_state();
	memset(device_info.MAC, 0, sizeof(device_info.MAC));
	foreach(i, 6) {
		device_info.MAC[2+i] = tbl_mac[5 - i];
	}
	device_info.version[0] = (u8)(FW_VERSION_TELINK_RELEASE) - 0x30;
	device_info.version[1] = (u8)(FW_VERSION_TELINK_RELEASE >> 8) - 0x30;
	device_info.version[2] = LM_SUB_VERSION;
	device_info.PID = MESH_PID_SEL;

    // Push message
	u8 cXor = CMD_ID_RSP_HOST_INFO;
	u8 payload_len = sizeof(device_infor_t);
	u8 total_len = payload_len + 5;
	u8 message[total_len];
	message[0] = SOF_HI;
	message[1] = SOF_LO;
	message[2] = payload_len + 2;
	message[3] = CMD_ID_RSP_HOST_INFO;
	memcpy(&message[4], (u8*)&device_info, payload_len);
	foreach(i, payload_len) {
		cXor ^= message[4+i];
	}
	message[total_len - 1] = cXor;
    uart_info_send_bytes(message, total_len);
}

/**
 * @func    infor_proc
 * @brief
 * @param   None
 * @retval  None
 */
void info_proc(void)
{
	if(push_info_enable_flag == true)
	{
		if(!clock_time_exceed_ms(0, PUSH_INFO_START_TIME_MS)) {
			return;
		}
		if(clock_time_exceed_ms(push_infor_st_time_ms, PUSH_INFO_INTERVAL_MS))
		{
			info_push_to_information_to_debug_pin();
			// Reset time
			push_infor_st_time_ms = clock_time_ms();
		}
		// Timeout
		if(clock_time_exceed_ms(PUSH_INFO_START_TIME_MS, PUSH_INFO_TIMEOUT_MS)) {
			push_info_enable_flag = false;
		}
	}
}
