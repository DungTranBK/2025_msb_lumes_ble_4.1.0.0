/*
 * soft_i2c.h
 *
 *  Created on: Dec 2, 2025
 *      Author: DungTranBK
 */

#ifndef SOFT_I2C_H_
#define SOFT_I2C_H_


/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#define SW_I2C_SDA_PIN       GPIO_PA1
#define SW_I2C_SCL_PIN       GPIO_PD6

#define I2C_ACK                 0xFF
#define I2C_NACK                0x00

#define I2C_TIMEOUT_DEFAULT     1000

enum {
	I2C_SUCCESS,
	I2C_TIMEOUT,
};
typedef u8 I2cStatus_Enum;

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/


/******************************************************************************/
/*                             EXPORT FUNCTIONS                               */
/******************************************************************************/
void soft_i2c_init(void);
void soft_i2c_start(void);

void soft_i2c_stop(void);
u8 soft_i2c_read(u8 ack);

I2cStatus_Enum soft_i2c_write(u8 value);
u8 soft_i2c_wait_ack(void);

#endif /* SOFT_I2C_H_ */
