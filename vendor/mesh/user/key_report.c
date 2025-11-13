/*
 * key_report.c
 *
 *  Created on: Feb 27, 2020
 *      Author: DungTran BK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "proj/tl_common.h"
#include "vendor/common/mesh_node.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/vendor_model.h"
#include "config_board.h"
#include "execution_scene.h"
#include "led.h"
#include "key_report.h"

#include "debug.h"
#ifdef  REPORT_DBG_EN
#define DBG_REPORT_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_REPORT_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_REPORT_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_REPORT_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_REPORT_SEND_STR(x)
#define DBG_REPORT_SEND_INT(x)
#define DBG_REPORT_SEND_HEX(x)
#define DBG_REPORT_SEND_BYTE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/


/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/
/**
 * @func   get_ele_addr_base_btn_idx
 * @brief
 * @param  Button Index
 * @retval Element Address
 */
u16 get_ele_addr_base_btn_idx(uint8_t idx)
{
	if(idx < NUMBER_BUTTON) {
		return (ele_adr_primary + idx);
	}
	return ADR_UNASSIGNED;
}
/**
 * @func   key_report_send
 * @brief
 * @param  None
 * @retval None
 */
void key_report_send(u8 idx, u8 key_code, bool en_execution)
{
	u16 ele_addr;
	vd_rc_key_report_t key_report = {0};
	key_report.code = key_code;
	ele_addr = get_ele_addr_base_btn_idx(idx);
	if(ele_addr != ADR_UNASSIGNED) {
		if(en_execution == true) {
			foreach(i, EXECUTION_SCENE_RETRY_TIME)
			{
				execution_scene_active(idx, key_code);
				DBG_REPORT_SEND_STR("\n *** active scene: ");
				DBG_REPORT_SEND_INT(i);
			}
		}
		mesh_tx_cmd_rsp(
				VD_RC_KEY_REPORT, (u8 *)&key_report,
				1, ele_addr, GATEWAY_UNICAST_ADDR, 0, 0
			);
	}
}

//End File
