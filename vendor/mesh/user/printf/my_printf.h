/*
 * my_printf.h
 *
 *  Created on: Nov 6, 2025
 *      Author: DungTranBK
 */

#ifndef MY_PRINTF_H_
#define MY_PRINTF_H_

#define MY_PRINTF_BAUD_115200    115200
#define MY_PRINTF_BAUD_230400    230400
#define MY_PRINTF_BAUD_1M        1000000

#define MY_PRINTF_BAUD_USE       MY_PRINTF_BAUD_115200

#define MY_PRINTF_IRQ_EN    (1&&!blcOta.ota_start_flag)


extern  void my_printf_send_bytes(u8 *p,int len);

#endif /* MY_PRINTF_H_ */
