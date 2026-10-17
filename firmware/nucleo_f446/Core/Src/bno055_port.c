#include "bno055_port.h"
#include "bno055.h"

static int bno055_port_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *h = ctx;
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(h, BNO055_I2C_ADDR_A << 1, reg, I2C_MEMADD_SIZE_8BIT,
                                            buf, len, BNO055_PORT_I2C_TIMEOUT_MS);

    return (st == HAL_OK) ? 0 : -1;
}

static int bno055_port_write(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *h = ctx;
    /* HAL_I2C_Mem_Write's pData isn't const, but it only reads the buffer*/
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(h, BNO055_HAL_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                                             (uint8_t *)buf, len, BNO055_PORT_I2C_TIMEOUT_MS);

    return (st == HAL_OK) ? 0 : -1;
}

static void bno055_port_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

void bno055_port_init(bno055_t *dev, I2C_HandleTypeDef *hi2c)
{
    dev->read = bno055_port_read;
    dev->write = bno055_port_write;
    dev->ctx = hi2c;
    dev->delay_ms = bno055_port_delay;
}
