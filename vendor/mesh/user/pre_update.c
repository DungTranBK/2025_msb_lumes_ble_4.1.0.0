/*
 * st_update.c
 *
 *  Created on: Sep 25, 2020
 *      Author: DungTran BK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "utilities.h"
#include "vendor/common/system_time.h"
#include "vendor/common/mesh_node.h"
#include "timestamp.h"
#include "sw_config.h"
#include "pre_update.h"

#include "debug.h"
#ifdef  PRE_DBG_EN
#define DBG_PRE_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_PRE_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_PRE_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_PRE_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_PRE_SEND_STR(x)
#define DBG_PRE_SEND_INT(x)
#define DBG_PRE_SEND_HEX(x)
#define DBG_PRE_SEND_BYTE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static update_status_periodically_t update_status[ELE_CNT];

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/
/**
 * @func    update_dev_status_init
 * @brief   None
 * @param
 * @retval  None
 */
void update_dev_status_init(void)
{
	foreach_arr(i, update_status){
		update_status[i].last_t_s = 0;
		update_status[i].interval_t_s    \
				= MESH_PERIODIC_PUBLISH_RANDOM_TIME_PW_ON + ((rand()% MESH_PERIODIC_PUBLISH_TIME_PW_ON));
	}
}

/**
 * @func    update_handle_update_device_st_periodically
 * @brief   None
 * @param
 * @retval  None
 */
void update_handle_update_device_st_periodically(void)
{
	foreach_arr(i, update_status)
	{
		if(clock_time_exceed_s(update_status[i].last_t_s, update_status[i].interval_t_s)){
			send_get_device_status_manual(i);
			update_status[i].last_t_s = clock_time_s();
			update_status[i].interval_t_s =   \
					MESH_PERIODIC_PUBLISH_TIME + ((rand()% MESH_PERIODIC_PUBLISH_RANDOM_TIME));
			if(i == 0) {
				if(timestamp_device_is_have_time() == false) {
					timestamp_send_query_time(ele_adr_primary);
				}
			}
		}
	}
}

// End file
