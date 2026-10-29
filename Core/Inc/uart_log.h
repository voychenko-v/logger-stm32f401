#ifndef UART_LOG_H
#define UART_LOG_H

#include "main.h"

void     uart_log_init(UART_HandleTypeDef *huart);
uint32_t uart_log_dropped(void);

#endif /* UART_LOG_H */
