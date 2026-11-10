#ifndef UART_RX_H
#define UART_RX_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

void     uart_rx_init(UART_HandleTypeDef *huart);
bool     uart_rx_get(uint8_t *c);
uint32_t uart_rx_overflow(void);

void uart_rx_on_byte(UART_HandleTypeDef *huart);    /* HAL_UART_RxCpltCallback */
void uart_rx_on_error(UART_HandleTypeDef *huart);   /* HAL_UART_ErrorCallback */

#endif /* UART_RX_H */
