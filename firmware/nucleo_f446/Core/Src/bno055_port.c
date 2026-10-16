#include "bno055_port.h"

static int bno055_port_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *h = ctx;
    HAL_Status_Def_Type st = HAL_I2C_Mem_Read(h, BNO055_I2C_ADDR_A << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, BNO055_PORT_I2C_TIMEOUT_MS); 
    
}:
