#ifndef BNO055_PORT_H
#define BNO055_PORT_H

#include "stm32f4xx_hal.h"
#include "bno055.h"

enum
{
    BNO055_PORT_I2C_TIMEOUT_MS = 10
};

enum
{
    BNO055_HAL_ADDR = BNO055_I2C_ADDR_A << 1
};

void bno055_port_init(bno055_t *dev, I2C_HandleTypeDef *hi2c);

#endif
