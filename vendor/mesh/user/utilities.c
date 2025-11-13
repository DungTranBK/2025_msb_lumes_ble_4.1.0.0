/*
 * utilities.c
 *
 *  Created on: Feb 18, 2020
 *      Author: DungTran BK
 */

#include "proj_lib/sig_mesh/app_mesh.h"
#include "config_board.h"
#include "utilities.h"

/**
 * @func   IsMatchVal
 * @brief  None
 * @param
 * @retval val is exist in *buff or not
 */
inline bool IsMatchVal(uint8_t *buff, uint8_t size, uint8_t val)
{
	for(u8 i = 0; i < size; i++) {
        if (buff[i] != val) return false;
    }
    return true;
}

/**
 * @func    __delay_us
 * @brief
 * @param   None
 * @retval  None
 */
inline void __delay_us(uint32_t us)
{
	uint32_t temp_clock_time = 0;
	temp_clock_time = clock_time();
	while((clock_time() - temp_clock_time) < (CLOCK_SYS_CLOCK_MHZ*us));
}

/**
 * @func    __delay_us
 * @brief
 * @param   None
 * @retval  None
 */
void __delay_ms(uint32_t ms)
{
	__delay_us(ms*1000);
}

/**
 * @func    get_io_bit_from_io_pin
 * @brief
 * @param   None
 * @retval  None
 */
u8 get_io_bit_from_io_pin(u16 io_pin)
{
	return (u8)io_pin;
}

/**
 * @func    get_ele_support_type
 * @brief
 * @param   None
 * @retval  None
 */
ElementSupportType_Enum get_ele_support_type(u8 idx)
{
	ElementSupportType_Enum type = ELE_NOT_SUPPORT;
	if(idx >= ELE_SCENE_OFFSET
			&& idx < ELE_SCENE_OFFSET + NUMBER_SCENE)
	{
		type =  ELE_SUPPORT_SCENE;
	}
	else if(idx >= ELE_RELAY_OFFSET
			&& idx < ELE_RELAY_OFFSET + NUMBER_RL)
	{
		type = ELE_SUPPORT_RELAY;
	}
	return type;
}

/**
 * @func    get_relay_idx_follow_model_index
 * @brief
 * @param   None
 * @retval  None
 */
bool get_relay_idx_follow_model_index(int model_idx, u8 *ptr_idx)
{
	*ptr_idx = model_idx - ELE_RELAY_OFFSET;
	if(*ptr_idx < NUMBER_RL)
	{
		return true;
	}
	return false;
}

/**
 * @func    get_scene_idx_follow_model_index
 * @brief
 * @param   None
 * @retval  None
 */
bool get_scene_idx_follow_model_index(int model_idx, u8 *ptr_idx)
{
	*ptr_idx = model_idx - ELE_SCENE_OFFSET;
	if(*ptr_idx < NUMBER_SCENE)
	{
		return true;
	}
	return false;
}

/**
 * @func    get_ele_address_base_type_and_idx
 * @brief
 * @param
 * @retval  None
 */
u16 get_ele_address_base_type_and_idx(ElementSupportType_Enum type, u8 idx)
{
	if(type == ELE_SUPPORT_SCENE)
	{
		return (ELE_SCENE_OFFSET + idx);
	}
	else if(type == ELE_SUPPORT_RELAY)
	{
		return (ELE_RELAY_OFFSET + idx);
	}
	return ADR_UNASSIGNED;
}
