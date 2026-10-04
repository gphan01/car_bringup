#include "../bno055.h"
#include "fake_i2c.h"   
#include <math.h>

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
    f.regs[0x14] =  0x84; f.regs[0x15] = 0x03;
    f.regs[0x16] =  0x7C; f.regs[0x17] = 0xFC;
    f.regs[0x18] =  0x3E; f.regs[0x19] = 0xFE;

    bno055_vec3_t g;
    assert(bno055_read_gyro(&dev, &g) == BNO055_OK);
    assert(g.x == 1.0f);
    assert(g.y == -1.0f);
    assert(g.z == -0.5f);

}

static void test_read_accel_scaling(void)
{
    fake_i2c_t f;
    bno055_t dev = make_dev(&f);
    
    /* x = +100,  y = -250 z = +981, LSB first */
    f.regs[0x08] =  0x64; f.regs[0x09] = 0x00;
    f.regs[0x0A] =  0x06; f.regs[0x0B] = 0xFF;
    f.regs[0x0C] =  0xD5; f.regs[0x0D] = 0x03;

    bno055_vec3_t a;
    assert(bno055_read_accel(&dev, &a) == BNO055_OK);
    assert(a.x == 1.0f);
    assert(a.y == -2.5f);
    assert(fabsf(a.z - 9.81f) < 1e-4f);
}
