/*
 * execution_scene.h
 *
 *  Created on: Apr 15, 2020
 *      Author: DungTran BK
 */

#ifndef EXECUTION_SCENE_H_
#define EXECUTION_SCENE_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj/tl_common.h"
#include "key_report.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define SCENE_ID_INVALID        0x0000
#define MAX_PAYLOAD_LEN         14
#define MAX_SCENE_DEST_ADR      5

typedef struct
{
	u16 dest_addr;
	u8  payload_len;
	u8  payload[MAX_PAYLOAD_LEN];
}control_infor_t;

typedef struct {
	u16 in_scene_id;
	u8  in_trans_t;
	u8  in_delay_t;
}scene_infor_t;

typedef struct
{
	u8 btn_event;
	u16 scene_id;
	union{
	    scene_infor_t scene_control[MAX_SCENE_DEST_ADR];
		control_infor_t control_infor[MAX_SCENE_DEST_ADR];
	};
	u8 trans_time;
}par_execution_scene_t;

typedef struct {
	u8 msg_type:3;
	u8 msg_id:5;
}msg_type_and_id_t;

typedef struct{
	u8 key_event:4;
	u8 key_nbr:4;
}setup_event_t;

typedef struct {
	u8 comparision:4;     // comparision_operation
	u8 condition:4;       // id_condition
}id_and_comparison_t;

typedef enum {
	COM_BIGGER           = 0x00,
	COM_LESS             = 0x01,
	COM_EQUAL            = 0x02,
	COM_GREATER_OR_EQUAL = 0x03,
	COM_LESS_OR_EQUAL    = 0x04,
	COM_RFU              = 0x05,
}comparision_operation_enum;

typedef enum {
	CON_LIGHT_INTENSITY   = 0x00,
	CON_TEMPERATURE       = 0x01,
	CON_HUMIDITY          = 0x02,
	CON_RFU               = 0x04,
}id_condition_enum;

typedef enum {
	MSG_ADD        = 0x00,
	MSG_DELETE     = 0x01,
	MSG_DELETE_ALL = 0x02,
	MSG_GET        = 0x03,
	MSG_RFU        = 0x04
}msg_type_setup_execution_scene_enum;

typedef enum {
	MSG_ADD_RSP        = 0x00,
	MSG_DELETE_RSP     = 0x01,
	MSG_DELETE_ALL_RSP = 0x02,
	MSG_GET_RSP        = 0x03,
	MSG_RSP_RFU        = 0x04
}msg_type_setup_execution_scene_status_enum;

// Setup Execution Scene Set

typedef struct{
	union {
		u8 msg_type_and_id;
		msg_type_and_id_t msg_type_and_id_st;
	    };
	union {
		u8  event;
		setup_event_t setup_event_st;
    	};
	u16 scene_id;
	union{
	    u8  nums_dest;
	    u8  nums_scene;
	};
	struct set_control_infor_t{
		u16 dest_addr;
		u8  payload_len;
		u8  payload[MAX_PAYLOAD_LEN];
	}set_control_infor[MAX_SCENE_DEST_ADR];
	u8  trans_time;
	struct add_condition_t{
		u8 logical;
		union{
			u8 condition_and_comparison;
			id_and_comparison_t id_and_comparision_st;
			};
		u16 compare_val;;
	}add_condition;

}setup_execution_scene_set_t;

typedef struct {
	u8  msg_type;
	u16 scene_id;
	u8  tid;
	u8  trans_time;
	u8  delay_time;
}vendor_scene_recall_t;

enum {
	VENDOR_SCENE_NOACK         = 0x00,
	VENDOR_SCENE_RECALL_NOACK  = 0x01,
	VENDOR_SCENE_DEL_NOACK     = 0x02
};

typedef struct {
	u8  msg_type;
	u16 opcode;
	u8 *par;
}control_by_op_t;

typedef struct {
	union {
		u8 msg_type_and_id;
		msg_type_and_id_t msg_type_and_id_st;
	  };
	u8  status_code;
	u8  event;
	u16 scene_id;
}execution_scene_add_rsp_str;

typedef struct {
	union {
		u8 msg_type_and_id;
		msg_type_and_id_t msg_type_and_id_st;
	  };
	u8  status_code;
	u8  event;
}execution_scene_del_rsp_str;

typedef struct {
	union {
		u8 msg_type_and_id;
		msg_type_and_id_t msg_type_and_id_st;
	  };
	u8  status_code;
}execution_scene_del_all_rsp_str;

enum {
	EX_STATUS_SUCCESS        = 0x00,
	EX_STATUS_FAIL           = 0x01,
	EX_STATUS_NOT_SET_BEFORE = 0x02,
	END_EX_STATUS            = 0x04
}vendor_execution_scene_status_enum;


// Header of get execution scene msg
typedef struct{
	union {
		u8 msg_type_and_id;
		msg_type_and_id_t msg_type_and_id_st;
	  };
	u8  status_code;
	u8  event;
	u16 scene_id;
	union{
	    u8  nbr_of_dest;
	    u8  nbr_of_scene;
	};
}header_execution_scene_get_rsp_t;


typedef struct {
	u32     send_last_t_ms;
	bool    en_send;
	u8      btn_key;
	u8      send_ev_idx;
}send_exe_scene_delay_t;

typedef struct {
	union {
		u8 msg_type_and_id;
		msg_type_and_id_t msg_type_and_id_st;
	 };
	u8  event;
	union {
		u8  event;
		setup_event_t evt_st;
    };

}msg_get_execution_scene_t;


enum dim_ud_present_enum {
	UD_UP,
	UD_DOWN,
	UD_UNKNOWN
};

enum control_direction_t{
    DIR_UP,
    DIR_DOWN,
    DIR_UP_SMOOTH,
    DIR_DOWN_SMOOTH,
    DIR_STOP,
    DIR_UP_DOWN_SMOOTH,
    DIR_UP_SMOOTH_LIMIT,
    DIR_DOWN_SMOOTH_LIMIT,
    DIR_RSV
};
typedef u8 control_direction_t;

typedef struct {
	u16 model_id;
	u8  dir;
	u16 step;
	u8  trans_t_ms;
}vd_up_down_control_t;

#define LM_SCENE_IN_INVALID                  0xFFFF

#define EXECUTION_SCENE_RETRY_TIME           0x1

#define EXECUTION_SCENE_DIMMING_LIMIT_EN     0

/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/

int scene_dimming_control(u8 idx, u8 evt, u8 st);
bool evt_dimming_exist(u8 idx, u8* new_evt);
void execution_scene_init(void);
void execution_scene_active(u8 key_number, u8 key_event);
void execution_set_up_auto_send(void);
void execution_loop_task(void);

int  execution_dimming_control_by_key_number(u8 key_number, u8 evt, u8 st);
bool execution_dimming_is_exist(u8 idx, u8* evt);

#endif /* EXECUTION_SCENE_H_ */
