/*
 * flash.h
 *
 *  Created on: Feb 20, 2021
 *      Author: DungTranBK
 */

#ifndef FLASH_H_
#define FLASH_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "../../../proj/tl_common.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/


/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void flash_user_store(
			int* flash_index,
			const u32 flash_adr,
			const u16 flash_size,
			const u16 block_size,
			u8* data
		);
void flash_user_restore(
			int flash_index,
			const u32 flash_adr,
			const u16 block_size,
			u8* data
		);
void flash_user_get_flash_index(
			int* flash_index,
			const u32 flash_adr,
			const u16 flash_size,
			const u16 block_size,
			u8* data
		);
#endif /* FLASH_H_ */
