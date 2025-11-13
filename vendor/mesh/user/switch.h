/*
 * switch.h
 *
 *  Created on: Mar 3, 2021
 *      Author: DungTranBK
 */

#ifndef SWITCH_H_
#define SWITCH_H_



void Switch_handleExitFactMode(void);
void Switch_handleFactConfirmOrActivate(uint8_t state);
void sw_loop_100ms_interval(void);

#endif /* SWITCH_H_ */
