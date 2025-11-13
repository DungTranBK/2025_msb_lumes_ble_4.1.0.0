/*
 * flash_user.c
 *
 *  Created on: Feb 20, 2021
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../../proj_lib/sig_mesh/app_mesh.h"
#include "../../common/system_time.h"
#include "utilities.h"

#include "debug.h"
#ifdef FS_USER_DBG_EN
	#define DBG_FS_USER_SEND_STR(x)          Dbg_sendString((s8*)x)
	#define DBG_FS_USER_SEND_INT(x)          Dbg_sendInt(x)
	#define DBG_FS_USER_SEND_HEX(x)          Dbg_sendHex(x)
    #define DBG_FS_USER_SEND_HEX_ONE_BYTE(x) Dbg_sendHexOneByte(x)
#else
	#define DBG_FS_USER_SEND_STR(x)
	#define DBG_FS_USER_SEND_INT(x)
	#define DBG_FS_USER_SEND_HEX(x)
    #define DBG_FS_USER_SEND_HEX_ONE_BYTE(x)
#endif

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/
/**
 * @func    flash_user_store
 * @brief
 * @param
 * @retval  None
 */
void flash_user_store(
			int* flash_index, const u32 flash_adr, const u16 flash_size, const u16 block_size, u8* data
		)
{
	*flash_index += block_size;
	if(*flash_index >= flash_size) {
		*flash_index = 0;
		flash_erase_sector(flash_adr);
	}
	flash_write_page (
			flash_adr + *flash_index, block_size, data
		);
}
/**
 * @func    flash_user_restore
 * @brief
 * @param
 * @retval  None
 */
void flash_user_restore(
			int flash_index, const u32 flash_adr, const u16 block_size, u8* data
		)
{
	flash_read_page(
			flash_adr + flash_index,
			block_size,
			data
		);
}
/**
 * @func    flash_user_get_flash_index
 * @brief
 * @param
 * @retval  None
 */
void flash_user_get_flash_index(
				int* flash_index, const u32 flash_adr, const u16 flash_size, const u16 block_size, u8* data
			)
{
	uint16_t i = 0;
	u8 tmp[block_size];

	while(i < flash_size) {
		flash_read_page(
				flash_adr + i, block_size, (u8*)(&tmp)
			);
		if(IsMatchVal(&tmp[0], block_size, MAX_U8)) {
			break;
		}
		i += block_size;
	}
	if(i == 0) {
		*flash_index = flash_size - block_size;
	}
	else {
		*flash_index = i - block_size;
	}
}
// End File
