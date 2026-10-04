#include "fake_i2c.h"
#include <assert.h>
#include <string.h>

void fake_i2c_init(fake_i2c_t *i2c)
{
    assert(i2c != NULL);
    *i2c = (fake_i2c_t){0};
}


int fake_i2c_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len)
{
    assert(ctx != NULL);
    assert(buf != NULL);
    assert(len > 0);

    fake_i2c_t *f = ctx;
    assert(reg + len <= sizeof(f->regs));

    memcpy(buf, &f->regs[reg], len);

    return 0;
}


int fake_i2c_write(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    assert(ctx != NULL);
    assert(buf != NULL);
    assert(len > 0);

    fake_i2c_t *f = ctx;
    assert(reg + len <= sizeof(f->regs));

    memcpy(&f->regs[reg], buf, len);
    
    return 0;
}

void fake_i2c_delay(uint32_t ms)
{
    (void)ms;
}
