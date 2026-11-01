#ifndef UART_LOG_H
#define UART_LOG_H

#include "main.h"

void     uart_log_init(UART_HandleTypeDef *huart);
uint32_t uart_log_dropped(void);
void uart_log_on_tx_complete(UART_HandleTypeDef *huart);
void uart_log_on_error(UART_HandleTypeDef *huart);

#endif /* UART_LOG_H */
