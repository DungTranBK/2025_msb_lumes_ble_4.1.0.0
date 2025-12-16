/*
 * soft_i2c.c
 *
 *  Created on: Dec 2, 2025
 *      Author: DungTranBK
 */

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "proj/tl_common.h"
#include "drivers/8258/gpio_8258.h"
#include "../config_board.h"
#include "../utilities.h"
#include "soft_i2c.h"

#include "../debug.h"
#ifdef  SOFT_I2C_DBG_EN
	#define DBG_SOFT_I2C_SEND_STR(x)     Dbg_sendString((s8*)x)
	#define DBG_SOFT_I2C_SEND_INT(x)     Dbg_sendInt(x)
	#define DBG_SOFT_I2C_SEND_HEX(x)     Dbg_sendHex(x)
	#define DBG_SOFT_I2C_SEND_BYTE(x)    Dbg_sendOneByteHex(x)
    #define DBG_SOFT_I2C_SEND_HEX32(x)   Dbg_sendHex32(x)
#else
	#define DBG_SOFT_I2C_SEND_STR(x)
	#define DBG_SOFT_I2C_SEND_INT(x)
	#define DBG_SOFT_I2C_SEND_HEX(x)
	#define DBG_SOFT_I2C_SEND_BYTE(x)
    #define DBG_SOFT_I2C_SEND_HEX32(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
#define SDA_HIGH   gpio_write(SW_I2C_SDA_PIN, 1)
#define SDA_LOW    gpio_write(SW_I2C_SDA_PIN, 0)

#define SCL_HIGH   gpio_write(SW_I2C_SCL_PIN, 1)
#define SCL_LOW    gpio_write(SW_I2C_SCL_PIN, 0)

#define SDA_IN     ((gpio_read(SW_I2C_SDA_PIN)& get_io_bit_from_io_pin(SW_I2C_SDA_PIN))?1:0)

/******************************************************************************/
/*                             PRIVATE FUNCS                                  */
/******************************************************************************/
static void soft_i2c_ack_nack(u8 mode);
I2cStatus_Enum soft_i2c_wait_ack(void);

/******************************************************************************/
/*                        EXPORT FUNCTIONS DECLERATION                        */
/******************************************************************************/

/**
 * @func    soft_i2c_in
 * @brief
 * @param
 * @retval  None
 */
static void soft_i2c_in(void)
{
	// SDA
	gpio_set_output_en(SW_I2C_SDA_PIN, false);
	gpio_set_input_en(SW_I2C_SDA_PIN, true);
	gpio_setup_up_down_resistor(SW_I2C_SDA_PIN, PM_PIN_PULLUP_10K);
}

/**
 * @func    soft_i2c_out
 * @brief
 * @param
 * @retval  None
 */
static void soft_i2c_out(void)
{
	// SDA
	gpio_set_output_en(SW_I2C_SDA_PIN, true);
	gpio_set_input_en(SW_I2C_SDA_PIN, false);
	gpio_setup_up_down_resistor(SW_I2C_SDA_PIN, PM_PIN_UP_DOWN_FLOAT);
}

/**
 * @func    soft_i2c_init
 * @brief
 * @param
 * @retval  None
 */
void soft_i2c_init(void)
{
	// Pin function
	gpio_set_func(SW_I2C_SDA_PIN, AS_GPIO);
	gpio_set_func(SW_I2C_SCL_PIN, AS_GPIO);

	// SCL
	gpio_set_input_en(SW_I2C_SCL_PIN, false);
	gpio_set_output_en(SW_I2C_SCL_PIN, true);
	gpio_setup_up_down_resistor(SW_I2C_SCL_PIN, PM_PIN_PULLUP_10K);


	// GPIO
	soft_i2c_out();
	__delay_ms(10);
	SDA_HIGH;
	SCL_HIGH;
}

/**
 * @func    soft_i2c_start
 * @brief
 * @param
 * @retval  None
 */
void soft_i2c_start(void)
{
	soft_i2c_out();
	SDA_HIGH;
	SCL_HIGH;
	__delay_us(4);
	SDA_LOW;
	__delay_us(4);
	SCL_LOW;
}

/**
 * @func    soft_i2c_stop
 * @brief
 * @param
 * @retval  None
 */
void soft_i2c_stop(void)
{
	soft_i2c_out();
	SDA_LOW;
	SCL_HIGH;
	__delay_us(4);
	SDA_HIGH;
	__delay_us(4);
}

/**
 * @func    soft_i2c_read
 * @brief
 * @param
 * @retval  None
 */
u8 soft_i2c_read(u8 ack)
{
	u8 i = 0x08;
	u8 j = 0;

	soft_i2c_in();

	while(i > 0x00)
	{
		SCL_LOW;
		__delay_us(2);
		SCL_HIGH;
		__delay_us(2);
		j <<= 1;
		if(SDA_IN != 0x00)
		{
			j++;
		}
	    __delay_us(1);
	    i--;
	}
	switch(ack)
	{
		case I2C_ACK:
			soft_i2c_ack_nack(I2C_ACK);
			break;

		case I2C_NACK:
			soft_i2c_ack_nack(I2C_NACK);
			break;
	}
	return j;
}

/**
 * @func    soft_i2c_write
 * @brief
 * @param
 * @retval  None
 */
I2cStatus_Enum soft_i2c_write(u8 value)
{
	u8 i = 8;
	soft_i2c_out();
	SCL_LOW;

	while(i > 0)
	{
        if(((value & 0x80) >> 0x07) != 0x00) {
            SDA_HIGH;
        }
        else {
            SDA_LOW;
        }
        value <<= 1;
        __delay_us(2);
        SCL_HIGH;
        __delay_us(2);
        SCL_LOW;
        __delay_us(2);
        i--;
	}
	return soft_i2c_wait_ack();
}

/**
 * @func    soft_i2c_ack_nack
 * @brief
 * @param
 * @retval  None
 */
static void soft_i2c_ack_nack(u8 mode)
{
	SCL_LOW;
	soft_i2c_out();
	switch(mode)
	{
		case I2C_ACK:
			SDA_LOW;
			break;

		default:
			SDA_HIGH;
			break;
	}
	__delay_us(2);
	SCL_HIGH;
	__delay_us(1);
	SCL_LOW;
}

/**
 * @func    soft_i2c_wait_ack
 * @brief
 * @param
 * @retval  None
 */
I2cStatus_Enum soft_i2c_wait_ack(void)
{
	u16 timeout = 0;

	soft_i2c_in();

	__delay_us(1);
	 SCL_HIGH;
	__delay_us(1);

	while(SDA_IN != 0x00)
	{
		timeout++;
		if(timeout > I2C_TIMEOUT_DEFAULT) {
			soft_i2c_stop();
			return I2C_TIMEOUT;
		}
	};
	SCL_LOW;
	return I2C_SUCCESS;
}
