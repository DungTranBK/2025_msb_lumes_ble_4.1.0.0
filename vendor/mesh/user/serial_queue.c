/*
 * serial_queue.c
 *
 *  Created on: Sep 21, 2020
 *      Author: DungTran BK
 */


/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "serial_queue.h"

#include "debug.h"
#ifdef SERIAL_DBG_EN
#define DBG_SERIAL_QUEUE_SEND_STR(x)   Dbg_sendString((s8*)x)
#define DBG_SERIAL_QUEUE_SEND_INT(x)   Dbg_sendInt(x)
#define DBG_SERIAL_QUEUE_SEND_HEX(x)   Dbg_sendHex(x)
#define DBG_SERIAL_QUEUE_SEND_BYTE(x)  Dbg_sendHexOneByte(x)
#else
#define DBG_SERIAL_QUEUE_SEND_STR(x)
#define DBG_SERIAL_QUEUE_SEND_INT(x)
#define DBG_SERIAL_QUEUE_SEND_HEX(x)
#define DBG_SERIAL_QUEUE_SEND_BYTE(x)
#endif

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static u8 serial_fifo_queue_buff[sizeof(uart_fifo_queue_t)];
static uart_fifo_queue_p serial_queue_rx = (uart_fifo_queue_p)serial_fifo_queue_buff;

/******************************************************************************/
/*                          PRIVATE FUNCTIONS DECLERATION                     */
/******************************************************************************/


/******************************************************************************/
/*                           EXPORT FUNCTIONS DECLERATION                     */
/******************************************************************************/

/**
 * @func   serial_reset_uart_queue
 * @brief
 * @param  None
 * @retval None
 */
void serial_reset_uart_queue(void)
{
	for( u16 i = 0; i < sizeof(uart_fifo_queue_t); i++ )
	{
		serial_fifo_queue_buff[i] = 0;
	}
}

/**
 * @func   serial_pop_byte_from_queue
 * @brief
 * @param  None
 * @retval None
 */
u8 serial_pop_byte_from_queue(void)
{
	return __UART_FIFO_DEQUEUE(serial_queue_rx);
}

/**
 * @func   serial_push_byte_to_queue
 * @brief
 * @param  None
 * @retval None
 */
void serial_push_byte_to_queue(u8 data)
{
	__UART_FIFO_ENQUEUE(serial_queue_rx, data);
}

/**
 * @func   serial_check_data_available
 * @brief
 * @param  None
 * @retval None
 */
bool serial_check_data_available(void)
{
	bool temp = UART_BUFFER_EMPTY;
    temp = (serial_queue_rx->head != serial_queue_rx->tail)?           \
                       (UART_BUFFER_HAS_DATA):(UART_BUFFER_EMPTY);
	return temp;
}

// End File
