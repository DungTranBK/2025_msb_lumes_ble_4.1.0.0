/*
 * uart_simu.h
 *
 *  Created on: Oct 2, 2024
 *      Author: DungTranBK
 */

#ifndef UART_SIMU_H_
#define UART_SIMU_H_

#define SIM_BAUD_115200    115200
#define SIM_BAUD_230400    230400
#define SIM_BAUD_1M        1000000

#define SIM_BAUD_USE       SIM_BAUD_115200

#define SIM_UART_IRQ_EN    (1&&!blcOta.ota_start_flag)


extern  void uart_info_send_bytes(u8 *p,int len);

#endif /* UART_SIMU_H_ */
