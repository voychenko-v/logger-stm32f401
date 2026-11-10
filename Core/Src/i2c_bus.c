#include "i2c_bus.h"
#include <stdio.h>
#include "i2c.h"      /* MX_I2C1_Init */

#define I2C_ADDR_FIRST  0x08U
#define I2C_ADDR_LAST   0x77U

#define I2C_SCL_PORT              GPIOB
#define I2C_SCL_PIN               GPIO_PIN_6
#define I2C_SDA_PORT              GPIOB
#define I2C_SDA_PIN               GPIO_PIN_7
#define I2C_RECOVERY_CLOCKS       9U
#define I2C_RECOVERY_HALF_US      5U


static void delay_us(uint32_t us)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;

    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks) { }
}


uint8_t i2c_bus_scan(I2C_HandleTypeDef *hi2c)
{
    uint8_t found = 0;

    printf("I2C scan:\r\n");
    for (uint8_t addr = I2C_ADDR_FIRST; addr <= I2C_ADDR_LAST; addr++)
    {

        if (HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(addr << 1), 1,
                                  I2C_BUS_SCAN_TIMEOUT_MS) == HAL_OK)
        {
            printf("  found 0x%02X\r\n", addr);
            found++;
        }
    }
    printf("I2C scan done, devices: %u\r\n", found);
    return found;
}


status_t i2c_bus_recover(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_DeInit(hi2c);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    delay_us(I2C_RECOVERY_HALF_US);

    for (uint32_t i = 0; i < I2C_RECOVERY_CLOCKS; i++) {
        if (HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN) == GPIO_PIN_SET) {
            break;
        }
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
        delay_us(I2C_RECOVERY_HALF_US);
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        delay_us(I2C_RECOVERY_HALF_US);
    }

    /* STOP: SDA 0→1 при SCL=1 */
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    delay_us(I2C_RECOVERY_HALF_US);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    delay_us(I2C_RECOVERY_HALF_US);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    delay_us(I2C_RECOVERY_HALF_US);

    status_t res = (HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN) == GPIO_PIN_SET)
                   ? STATUS_OK : STATUS_ERR_I2C;

    MX_I2C1_Init();
    return res;
}

