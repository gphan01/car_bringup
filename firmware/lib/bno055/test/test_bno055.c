#include "../bno055.h"
#include "fake_i2c.h"   

#include <assert.h>

static bno055_t make_dev(fake_i2c_t *f)
{
    fake_i2c_init(f);

    return (bno055_t){
        .read      = fake_i2c_read,
        .write     = fake_i2c_write,
        .delay_ms  = fake_i2c_delay,
        .ctx       = f,
    };
}


static void test_read_gyro_scaling(void)
{
    fake_i2c_t f;
    bno055_t dev = make_dev(&f);
    
    /* x = +900,  y = -900 z = -450, LSB first */
    f.regs[0x14] =  0x84; f.regs[15] = 0x03;
    f.regs[0x16] =  0x7C; f.regs[17] = 0xFC;
    f.regs[0x18] =  0x3E; f.regs[19] = 0xFE;

    bno055_vec3_t g;
    assert(bno055_read_gyro(&dev, &g) == BNO055_OK);
    assert(g.x == 1.0f);
    assert(g.y == -1.0f);
    assert(g.z == -0.5f);

}
