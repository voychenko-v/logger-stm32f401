#include "uart_log.h"
#include <stdio.h>

#define UART_LOG_TX_TIMEOUT_MS  100U

static UART_HandleTypeDef *s_huart = NULL;

void uart_log_init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    setvbuf(stdout, NULL, _IONBF, 0);
}

int _write(int fd, char *ptr, int len)
{
    (void)fd;

    if (s_huart == NULL) {
        return -1;
    }
    if (HAL_UART_Transmit(s_huart, (uint8_t *)ptr, (uint16_t)len,
                          UART_LOG_TX_TIMEOUT_MS) != HAL_OK) {
        return -1;
    }
    return len;
}
