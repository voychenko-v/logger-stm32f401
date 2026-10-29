#include "uart_log.h"
#include <stdio.h>

#define TX_BUF_SIZE   1024U
#define TX_BUF_MASK   (TX_BUF_SIZE - 1U)

static UART_HandleTypeDef *s_huart = NULL;

static uint8_t           s_buf[TX_BUF_SIZE];
static volatile uint16_t s_head = 0;
static volatile uint16_t s_tail = 0;
static volatile uint16_t s_dma_len = 0;
static volatile uint32_t s_dropped = 0;


static void tx_kick(void)
{
    if (s_dma_len != 0U) {
        return;
    }

    uint16_t head = s_head;
    uint16_t tail = s_tail;
    if (head == tail) {
        return;
    }

    uint16_t len = (head > tail) ? (uint16_t)(head - tail)
                                 : (uint16_t)(TX_BUF_SIZE - tail);

    if (HAL_UART_Transmit_DMA(s_huart, &s_buf[tail], len) == HAL_OK) {
        s_dma_len = len;
    }
}

void uart_log_init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    setvbuf(stdout, NULL, _IONBF, 0);
}

uint32_t uart_log_dropped(void)
{
    return s_dropped;
}

int _write(int fd, char *ptr, int len)
{
    (void)fd;

    if (s_huart == NULL) {
        return -1;
    }

    uint16_t head = s_head;
    uint16_t free_space = (uint16_t)((s_tail - head - 1U) & TX_BUF_MASK);

    if ((uint16_t)len > free_space) {
        s_dropped++;
        return len;
    }

    for (int i = 0; i < len; i++) {
        s_buf[head] = (uint8_t)ptr[i];
        head = (uint16_t)((head + 1U) & TX_BUF_MASK);
    }
    s_head = head;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    tx_kick();
    __set_PRIMASK(primask);

    return len;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != s_huart) {
        return;
    }
    s_tail = (uint16_t)((s_tail + s_dma_len) & TX_BUF_MASK);
    s_dma_len = 0;
    tx_kick();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != s_huart) {
        return;
    }
    s_tail = (uint16_t)((s_tail + s_dma_len) & TX_BUF_MASK);
    s_dma_len = 0;
    s_dropped++;
    tx_kick();
}
