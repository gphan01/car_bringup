#include "fake_i2c.h"
#include <assert.h>
#include <string.h>

static fake_i2c_t *curr; /* The fake that fake_i2c_delay logs into */

void fake_i2c_init(fake_i2c_t *i2c)
{
    assert(i2c != NULL);
    *i2c = (fake_i2c_t){0};
    i2c->fail_write_reg = -1;

    curr = i2c; 

}


int fake_i2c_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len)
{
    assert(ctx != NULL);
    assert(buf != NULL);
    assert(len > 0);

    fake_i2c_t *f = ctx;
    assert(reg + len <= sizeof(f->regs));

    f->read_count++;

    if (f->fail_reads > 0)
    {
        f->fail_reads--;
        return -1; /* Fake a failed I2C transfer */
    }

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
    assert(len <= FAKE_MAX_WRITE);
    assert(f->log_count < FAKE_MAX_EVENTS);

    fake_ev_t *ev = &f->log[f->log_count++];
    
    ev->type = FAKE_EV_WRITE;
    ev->reg  = reg;
    ev->len  = len;
    memcpy(ev->data, buf, len);

    if (reg == f->fail_write_reg)
    {
        return -1;
    }

    memcpy(&f->regs[reg], buf, len);

    return 0;
}

void fake_i2c_delay(uint32_t ms)
{
    assert(curr != NULL);
    assert(curr->log_count < FAKE_MAX_EVENTS);

    fake_ev_t *ev = &curr->log[curr->log_count++];
    ev->type = FAKE_EV_DELAY;
    ev->ms = ms;
}
