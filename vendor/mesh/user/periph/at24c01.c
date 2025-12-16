/*
 * at24c01.c
 *
 *  Created on: Dec 2, 2025
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj/tl_common.h"
#include "../config_board.h"
#include "../utilities.h"
#include "soft_i2c.h"
#include "at24c01.h"

#include "../debug.h"
#ifdef  AT24C01_DBG_EN
	#define DBG_AT24C01_SEND_STR(x)     Dbg_sendString((s8*)x)
	#define DBG_AT24C01_SEND_INT(x)     Dbg_sendInt(x)
	#define DBG_AT24C01_SEND_HEX(x)     Dbg_sendHex(x)
	#define DBG_AT24C01_SEND_BYTE(x)    Dbg_sendOneByteHex(x)
    #define DBG_AT24C01_SEND_HEX32(x)   Dbg_sendHex32(x)
#else
	#define DBG_AT24C01_SEND_STR(x)
	#define DBG_AT24C01_SEND_INT(x)
	#define DBG_AT24C01_SEND_HEX(x)
	#define DBG_AT24C01_SEND_BYTE(x)
    #define DBG_AT24C01_SEND_HEX32(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/


/******************************************************************************/
/*                             PRIVATE FUNCS                                  */
/******************************************************************************/

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

/**
 * @func    at24c01_init
 * @brief
 * @param
 * @retval  None
 */
void at24c01_init(void)
{
	soft_i2c_init();
}

/**
 * @func    at24c01_read_multi_byte
 * @brief
 * @param
 * @retval  None
 */
int at24c01_read_multi_byte(u8 address, u8 *rd_buff, u8 len)
{
	if(address > AT24C01_ADDR_MAX) {
		return -1;
	}
	soft_i2c_start();
	if(soft_i2c_write(AT24C01_WR) == I2C_TIMEOUT) {
		soft_i2c_stop();
		return -1;
	}
	if(soft_i2c_write(address) == I2C_TIMEOUT) {
		soft_i2c_stop();
		return -1;
	}
	soft_i2c_start();
	if(soft_i2c_write(AT24C01_RD) == I2C_TIMEOUT) {
		soft_i2c_stop();
		return -1;
	}
	foreach(i, len-1) {
		rd_buff[i] = soft_i2c_read(I2C_ACK);
	}
	rd_buff[len-1] = soft_i2c_read(I2C_NACK);
	soft_i2c_stop();
	return 0;
}

/**
 * @func    at24c01_write_multi_byte
 * @brief
 * @param
 * @retval  None
 */
int at24c01_write_multi_byte(u8 address, u8 *wr_buff, u8 len)
{
	if(address > AT24C01_ADDR_MAX) {
		return -1;
	}
	u8 total_page = 1;
	if(len%AT24C01_PAGE_SIZE == 0) {
		total_page = len/AT24C01_PAGE_SIZE;
	}
	else {
		total_page = len/AT24C01_PAGE_SIZE + 1;
	}
	foreach(i, total_page) {
		soft_i2c_start();
		soft_i2c_write(AT24C01_WR);
		soft_i2c_write(address+i*AT24C01_PAGE_SIZE);
		foreach(j, AT24C01_PAGE_SIZE) {
			soft_i2c_write(wr_buff[i*AT24C01_PAGE_SIZE + j]);
		}
		soft_i2c_stop();
		__delay_ms(50);
	}
	return 0;
}

/**
 * @func    at24c01_read_byte
 * @brief
 * @param
 * @retval  None
 */
u8 at24c01_read_byte(u8 address)
{
	u8 val = 0;
	soft_i2c_start();
	soft_i2c_write(AT24C01_WR);
	soft_i2c_write(address);

	soft_i2c_start();
	soft_i2c_write(AT24C01_RD);
	val = soft_i2c_read(I2C_NACK);
	soft_i2c_stop();
	return val;
}

/**
 * @func    at24c01_write_byte
 * @brief
 * @param
 * @retval  None
 */
void at24c01_write_byte(u8 address, u8 value)
{
	soft_i2c_start();
	soft_i2c_write(AT24C01_WR);
	soft_i2c_write(address);
	soft_i2c_write(value);
	soft_i2c_stop();
}
