#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "main.h"

#define I2C_BUS_SCAN_TIMEOUT_MS  10U

uint8_t i2c_bus_scan(I2C_HandleTypeDef *hi2c);

#endif /* I2C_BUS_H */
