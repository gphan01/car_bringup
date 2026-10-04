#ifndef FAKE_I2C_H
#define FAKE_I2C_H

#include <stdint.h>

typedef struct
{
    uint8_t regs[256];
} fake_i2c_t;


void fake_i2c_init(fake_i2c_t *i2c);

int fake_i2c_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len);
int fake_i2c_write(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len);
void fake_i2c_delay(uint32_t ms);

#endif





