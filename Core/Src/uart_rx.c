#include "uart_rx.h"

#define RX_BUF_SIZE  128U
#define RX_BUF_MASK  (RX_BUF_SIZE - 1U)

static UART_HandleTypeDef *s_huart = NULL;
static uint8_t             s_byte;
static uint8_t             s_buf[RX_BUF_SIZE];
static volatile uint16_t   s_head = 0;
static volatile uint16_t   s_tail = 0;
static volatile uint32_t   s_overflow = 0;

void uart_rx_init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    HAL_UART_Receive_IT(huart, &s_byte, 1);
}

bool uart_rx_get(uint8_t *c)
{
    if (s_tail == s_head) {
        return false;
    }
    *c = s_buf[s_tail];
    s_tail = (uint16_t)((s_tail + 1U) & RX_BUF_MASK);
    return true;
}

uint32_t uart_rx_overflow(void)
{
    return s_overflow;
}

void uart_rx_on_byte(UART_HandleTypeDef *huart)
{
    if (huart != s_huart) {
        return;
    }

    uint16_t next = (uint16_t)((s_head + 1U) & RX_BUF_MASK);
    if (next != s_tail) {
        s_buf[s_head] = s_byte;
        s_head = next;
    } else {
        s_overflow++;
    }

    HAL_UART_Receive_IT(s_huart, &s_byte, 1);
}

void uart_rx_on_error(UART_HandleTypeDef *huart)
{
    if (huart != s_huart) {
        return;
    }

    HAL_UART_Receive_IT(s_huart, &s_byte, 1);
}
