#ifndef FAKE_I2C_H
#define FAKE_I2C_H

#include <stdint.h>

enum {
    FAKE_MAX_WRITE = 32,
    FAKE_MAX_EVENTS = 64,
};

typedef enum {
    FAKE_EV_NONE,
    FAKE_EV_WRITE,
    FAKE_EV_DELAY,   
} fake_ev_type_t;

typedef struct {
    fake_ev_type_t type;
    uint8_t reg;
    uint16_t len;
    uint8_t data[FAKE_MAX_WRITE];
    uint32_t ms;
} fake_ev_t;

typedef struct
{
    uint8_t regs[256];
    int fail_reads;
    fake_ev_t log[FAKE_MAX_EVENTS];
    int log_count;
    int fail_write_reg; // -1 means never fail  
} fake_i2c_t;

void fake_i2c_init(fake_i2c_t *i2c);

int fake_i2c_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len);
int fake_i2c_write(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len);
void fake_i2c_delay(uint32_t ms);

#endif





