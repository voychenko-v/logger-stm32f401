#include "i2c_bus.h"
#include <stdio.h>

#define I2C_ADDR_FIRST  0x08U
#define I2C_ADDR_LAST   0x77U

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
