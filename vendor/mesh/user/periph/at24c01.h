/*
 * at24c01.h
 *
 *  Created on: Dec 2, 2025
 *      Author: DungTranBK
 */

#ifndef AT24C01_H_
#define AT24C01_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#define AT24C01_ADDR_MAX         0xFF
#define AT24C01_I2C_ADDRESS      0x50

#define AT24C01_WR               0xA0
#define AT24C01_RD               0xA1

#define AT24C01_PAGE_SIZE        8

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/


/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void at24c01_init();
int at24c01_read_multi_byte(u8 address, u8 *rd_buff, u8 len);
int at24c01_write_multi_byte(u8 address, u8 *wr_buff, u8 len);
u8 at24c01_read_byte(u8 address);
void at24c01_write_byte(u8 address, u8 value);

#endif /* AT24C01_H_ */
