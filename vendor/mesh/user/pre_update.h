/*
 * pre_update.h
 *
 *  Created on: Sep 25, 2020
 *      Author: DungTran BK
 */

#ifndef PRE_UPDATE_H_
#define PRE_UPDATE_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "../../../proj/tl_common.h"
/******************************************************************************/
/*                       EXPORT TYPE AND DEFINITION                           */
/******************************************************************************/

typedef struct {
	u32 last_t_s;
	u32 interval_t_s;
}update_status_periodically_t;

#define MESH_PERIODIC_PUBLISH_RANDOM_TIME       (100*60)  // 100 minutes
#define MESH_PERIODIC_PUBLISH_TIME	            (20*60)   // 20 minutes

#define MESH_PERIODIC_PUBLISH_RANDOM_TIME_PW_ON	10
#define MESH_PERIODIC_PUBLISH_TIME_PW_ON	    120

/******************************************************************************/
/*                            EXPORTED FUNCTIONS                              */
/******************************************************************************/

void update_dev_status_init(void);
void update_handle_update_device_st_periodically(void);
void send_get_device_status_manual(u8 idx);

#endif /* PRE_UPDATE_H_ */
