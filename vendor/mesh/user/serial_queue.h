/*
 * serial_queue.h
 *
 *  Created on: Sep 21, 2020
 *      Author: DungTran BK
 */

#ifndef SERIAL_QUEUE_H_
#define SERIAL_QUEUE_H_

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/
#include "../../../proj/tl_common.h"
/******************************************************************************/
/*                       EXPORT TYPE AND DEFINITION                           */
/******************************************************************************/

#define SIZE_BUFF_DATA_RX       256

typedef struct{
    /*! Indexes of next byte to send*/
    uint8_t head;
    /*! Index of where to enqueue next message*/
    uint8_t tail;
    /*! FIFO of queue data*/
    uint8_t fifo[SIZE_BUFF_DATA_RX];
}uart_fifo_queue_t;

/// Handle pointer
typedef uart_fifo_queue_t * uart_fifo_queue_p;

enum UartBufferState_enum{
	UART_BUFFER_EMPTY    = 0,
	UART_BUFFER_HAS_DATA = 1
};
typedef bool BufferState_enum;

  /**
 * @brief Macro that enqueues a byte of data in a FIFO queue.
 *
 * @param queue  Pointer to the FIFO queue.
 *
 * @param data   Data byte to be enqueued.
 *
 * @param size   Size used to control the wrap-around of the FIFO pointers.
 */
#define __UART_FIFO_ENQUEUE(queue, data)           \
    (queue)->fifo[(queue)->head] = (data);         \
    (queue)->head = (((queue)->head + 1) % (SIZE_BUFF_DATA_RX))
/**
 * @brief Macro that de-queues a byte of data from a FIFO queue.
 *
 * @param queue Pointer to the FIFO queue.
 *
 * @param size  Size used to control the wrap-around of the FIFO pointers.
 */
#define __UART_FIFO_DEQUEUE(queue)                \
  (queue)->fifo[(queue)->tail];                   \
  (queue)->fifo[(queue)->tail] = 0;               \
  (queue)->tail = (((queue)->tail + 1) % (SIZE_BUFF_DATA_RX))

/******************************************************************************/
/*                              EXPORT FUNCTION                               */
/******************************************************************************/

void serial_reset_uart_queue(void);
u8 serial_pop_byte_from_queue(void);
void serial_push_byte_to_queue(u8 data);
bool serial_check_data_available(void);

#endif /* SERIAL_QUEUE_H_ */
