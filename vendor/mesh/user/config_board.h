/*
 * config_board.h
 *
 *  Created on: Aug 20, 2024
 *      Author: DungTranBK
 */

#ifndef CONFIG_BOARD_H_
#define CONFIG_BOARD_H_

#include "../app_config_8258.h"

#define NUMBER_RL               _RELAY_NUMBER_
#define NUMBER_SCENE            _SCENE_NUMBER_

#define NUMBER_LED_RL           NUMBER_RL
#define NUMBER_LED_SCENE        NUMBER_SCENE

#define NUMBER_BUTTON          ( NUMBER_RL + NUMBER_SCENE )
#define NUMBER_INPUT           NUMBER_BUTTON

#define NUMBER_SWITCH_BUTTON   NUMBER_RL
#define NUMBER_SCENE_BUTTON    NUMBER_SCENE

#define ELE_SCENE_OFFSET       0
#define ELE_RELAY_OFFSET       2

#if PCBA_8258_SEL == PCBA_8258_DONGLE_48PIN

/*
 *  Output RL
 */
// RL1
#define PIN_CONTROL_RL0_ON      GPIO_PB2
#define BIT_CONTROL_RL0_ON      2

#define PIN_CONTROL_RL0_OFF     GPIO_PB3
#define BIT_CONTROL_RL0_OFF     3

// RL2
#define PIN_CONTROL_RL1_ON      GPIO_PB4
#define BIT_CONTROL_RL1_ON      4

#define PIN_CONTROL_RL1_OFF     GPIO_PB5
#define BIT_CONTROL_RL1_OFF     5

// RL3
#define PIN_CONTROL_RL2_ON      GPIO_PC2
#define BIT_CONTROL_RL2_ON      2

#define PIN_CONTROL_RL2_OFF     GPIO_PC3
#define BIT_CONTROL_RL2_OFF     3

/*
 *  Input button
 */
#define PIN_SC_BUTTON_0         GPIO_PD5
#define BIT_SC_BUTTON_0         5

#define PIN_SC_BUTTON_1         GPIO_PD6
#define BIT_SC_BUTTON_1         6

#define PIN_BUTTON_0            GPIO_PD3
#define BIT_BUTTON_0            3

#define PIN_BUTTON_1            GPIO_PD4
#define BIT_BUTTON_1            4

#define PIN_BUTTON_2            GPIO_PC0
#define BIT_BUTTON_2            0

/*
 *  Output led
 */
#define PIN_SC_LED_WHITE_0      GPIO_PC4
#define PIN_SC_LED_WHITE_1      GPIO_PC5

#define PIN_LED_RED_0           GPIO_PA2
#define PIN_LED_BLUE_0          GPIO_PA3

#define PIN_LED_RED_1           GPIO_PA4
#define PIN_LED_BLUE_1          GPIO_PB0

#define PIN_LED_RED_2           GPIO_PB1
#define PIN_LED_BLUE_2          GPIO_PA1

/*
 *  UART
 */
#define HOST_UART_TX            UART_TX_PB1
#define HOST_UART_RX            UART_RX_PB0

/*
 *  RADAR
 */
#define PIN_RADAR_ENABLE        GPIO_PB2
#define PIN_RADAR_OT2           GPIO_PB3

/*
 *  ZVD
 */
#define PIN_ZERO_CROSSING       GPIO_PA3

#elif PCBA_8258_SEL == PCBA_8258_LUMI_V1_0
/*
 *  Output RL
 */
// RL1
#define PIN_CONTROL_RL0_ON      GPIO_PA6
#define BIT_CONTROL_RL0_ON      6

#define PIN_CONTROL_RL0_OFF     GPIO_PD5
#define BIT_CONTROL_RL0_OFF     5

// RL2
#define PIN_CONTROL_RL1_ON      GPIO_PA5
#define BIT_CONTROL_RL1_ON      5

#define PIN_CONTROL_RL1_OFF     GPIO_PD6
#define BIT_CONTROL_RL1_OFF     6

// RL3
#define PIN_CONTROL_RL2_ON      GPIO_PA4
#define BIT_CONTROL_RL2_ON      4

#define PIN_CONTROL_RL2_OFF     GPIO_PA1
#define BIT_CONTROL_RL2_OFF     1

/*
 *  Input button
 */
#define PIN_SC_BUTTON_0         GPIO_PB4
#define BIT_SC_BUTTON_0         4

#define PIN_SC_BUTTON_1         GPIO_PD3
#define BIT_SC_BUTTON_1         3

#if NUMBER_RL == 1 || NUMBER_RL == 2
	#define PIN_BUTTON_0            GPIO_PC0
	#define BIT_BUTTON_0            0

	#define PIN_BUTTON_1            GPIO_PC6
	#define BIT_BUTTON_1            6
#elif NUMBER_RL == 3
	#define PIN_BUTTON_0            GPIO_PC0
	#define BIT_BUTTON_0            0

	#define PIN_BUTTON_1            GPIO_PC3
	#define BIT_BUTTON_1            3

	#define PIN_BUTTON_2            GPIO_PC6
	#define BIT_BUTTON_2            6
#endif

/*
 *  Output led
 */
#define PIN_SC_LED_WHITE_1      GPIO_PD4
#define PIN_SC_LED_WHITE_0      GPIO_PB5

#if   NUMBER_RL == 1
	#define PIN_LED_RED_0           GPIO_PC2
	#define PIN_LED_BLUE_0          GPIO_PC1
#elif NUMBER_RL == 2
	#define PIN_LED_RED_0           GPIO_PB7
	#define PIN_LED_BLUE_0          GPIO_PB6

	#define PIN_LED_RED_1           GPIO_PC5
	#define PIN_LED_BLUE_1          GPIO_PC4
#elif NUMBER_RL == 3
	#define PIN_LED_RED_0           GPIO_PB7
	#define PIN_LED_BLUE_0          GPIO_PB6

	#define PIN_LED_RED_1           GPIO_PC2
	#define PIN_LED_BLUE_1          GPIO_PC1

	#define PIN_LED_RED_2           GPIO_PC5
	#define PIN_LED_BLUE_2          GPIO_PC4
#endif
/*
 *  UART
 */
#define HOST_UART_TX            UART_TX_PB1
#define HOST_UART_RX            UART_RX_PB0

/*
 *  RADAR
 */
#define PIN_RADAR_ENABLE        GPIO_PB2
#define PIN_RADAR_OT2           GPIO_PB3

/*
 *  ZVD
 */
#define PIN_ZERO_DETECT         GPIO_PD2

/*
 * Detect relay type
 */
#define PIN_DETECT_RELAY_TYPE   GPIO_PA3


#elif PCBA_8258_SEL == PCBA_8258_LUMI_V1_1
/*
 *  Output RL
 */
// RL1
#define PIN_CONTROL_RL0_ON      GPIO_PA6
#define BIT_CONTROL_RL0_ON      6

#define PIN_CONTROL_RL0_OFF     GPIO_PD5
#define BIT_CONTROL_RL0_OFF     5

// RL2
#define PIN_CONTROL_RL1_ON      GPIO_PA5
#define BIT_CONTROL_RL1_ON      5

#define PIN_CONTROL_RL1_OFF     GPIO_PD5
#define BIT_CONTROL_RL1_OFF     5

// RL3
#define PIN_CONTROL_RL2_ON      GPIO_PA4
#define BIT_CONTROL_RL2_ON      4

#define PIN_CONTROL_RL2_OFF     GPIO_PD5
#define BIT_CONTROL_RL2_OFF     5

/*
 *  Input button
 */
#define PIN_SC_BUTTON_0         GPIO_PB4
#define BIT_SC_BUTTON_0         4

#define PIN_SC_BUTTON_1         GPIO_PD3
#define BIT_SC_BUTTON_1         3

#define PIN_BUTTON_0            GPIO_PC0
#define BIT_BUTTON_0            0

#define PIN_BUTTON_1            GPIO_PC3
#define BIT_BUTTON_1            3

#define PIN_BUTTON_2            GPIO_PC7
#define BIT_BUTTON_2            7

/*
 *  Output led
 */
#define PIN_SC_LED_WHITE_0      GPIO_PB5
#define PIN_SC_LED_WHITE_1      GPIO_PD4

#define PIN_LED_RED_0           GPIO_PB7
#define PIN_LED_BLUE_0          GPIO_PB6

#define PIN_LED_RED_1           GPIO_PC2
#define PIN_LED_BLUE_1          GPIO_PC1

#define PIN_LED_RED_2           GPIO_PC6
#define PIN_LED_BLUE_2          GPIO_PC4

/*
 *  UART
 */
#define HOST_UART_TX            UART_TX_PB1
#define HOST_UART_RX            UART_RX_PB0

/*
 *  RADAR
 */
#define PIN_RADAR_ENABLE        GPIO_PB2
#define PIN_RADAR_OT2           GPIO_PB3

/*
 *  ZVD
 */
#define PIN_ZERO_DETECT         GPIO_PD2

/*
 * Detect relay type
 */
#define PIN_DETECT_RELAY_TYPE   GPIO_PA3


#elif PCBA_8258_SEL == PCBA_8258_C1T139A30_V1_2

/*
 *  Output RL
 */
// RL1
#define PIN_CONTROL_RL0_ON      GPIO_PC6
#define BIT_CONTROL_RL0_ON      6

#define PIN_CONTROL_RL0_OFF     GPIO_PC7
#define BIT_CONTROL_RL0_OFF     7

// RL2
#define PIN_CONTROL_RL1_ON      GPIO_PC2
#define BIT_CONTROL_RL1_ON      2

#define PIN_CONTROL_RL1_OFF     GPIO_PC3
#define BIT_CONTROL_RL1_OFF     3

// RL3
#define PIN_CONTROL_RL2_ON      GPIO_PA4
#define BIT_CONTROL_RL2_ON      4

#define PIN_CONTROL_RL2_OFF     GPIO_PA1
#define BIT_CONTROL_RL2_OFF     1

/*
 *  Input button
 */
#define PIN_SC_BUTTON_0         GPIO_PB1
#define BIT_SC_BUTTON_0         1

#define PIN_SC_BUTTON_1         GPIO_PB2
#define BIT_SC_BUTTON_1         2

#define PIN_BUTTON_0            GPIO_PB3
#define BIT_BUTTON_0            3

#define PIN_BUTTON_1            GPIO_PB4
#define BIT_BUTTON_1            4

#define PIN_BUTTON_2            GPIO_PB5
#define BIT_BUTTON_2            5

/*
 *  Output led
 */
#define PIN_SC_LED_WHITE_0      GPIO_PD2
#define PIN_SC_LED_WHITE_1      GPIO_PD3

#define PIN_LED_RED_0           GPIO_PD4
#define PIN_LED_BLUE_0          GPIO_PD5

#define PIN_LED_RED_1           GPIO_PD0
#define PIN_LED_BLUE_1          GPIO_PD1

#define PIN_LED_RED_2           GPIO_PD6
#define PIN_LED_BLUE_2          GPIO_PD7

/*
 *  UART
 */
#define HOST_UART_TX            UART_TX_PB1
#define HOST_UART_RX            UART_RX_PB0

/*
 *  RADAR
 */
#define PIN_RADAR_ENABLE        GPIO_PB2
#define PIN_RADAR_OT2           GPIO_PB3

/*
 *  ZVD
 */
#define PIN_ZERO_CROSSING       GPIO_PA3



#endif
/*
 * BACKUP mask
 */
#define BACKUP_MASK_RL          0x07
#define BACKUP_MASK_SCENE       0x03
#define BACKUP_MASK_ALL         0x1F

#endif /* CONFIG_BOARD_H_ */
