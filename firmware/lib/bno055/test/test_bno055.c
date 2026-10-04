#include "../bno055.h"
#include "fake_i2c.h"   
#include <math.h>

#include <assert.h>
#include <stdio.h>

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

static void test_read_quat_scaling(void)
{
    fake_i2c_t f;
    bno055_t dev = make_dev(&f);
    
    /* w = 16384 x = -8192,  y = 4096 z = 0, LSB first */
    f.regs[0x20] =  0x00; f.regs[0x21] = 0x40;
    f.regs[0x22] =  0x00; f.regs[0x23] = 0xE0;
    f.regs[0x24] =  0x00; f.regs[0x25] = 0x10;
    f.regs[0x26] =  0x00; f.regs[0x27] = 0x08;

    bno055_quat_t q;
    assert(bno055_read_quat(&dev, &q) == BNO055_OK);
    assert(q.w == 1.0f);
    assert(q.x == -0.5f);
    assert(q.y ==  0.25f);
    assert(q.z ==  0.125f); 
}

static void test_le16_sign_extremes(void)
{
    fake_i2c_t f;
    bno055_t dev = make_dev(&f);
    
    /* w = -32768 x = -8192,  y = 1 z = -1, LSB first */
    f.regs[0x20] =  0x00; f.regs[0x21] = 0x80;
    f.regs[0x22] =  0xFF; f.regs[0x23] = 0x7F;
    f.regs[0x24] =  0x01; f.regs[0x25] = 0x00;
    f.regs[0x26] =  0xFF; f.regs[0x27] = 0xFF;

    bno055_quat_t q;
    assert(bno055_read_quat(&dev, &q) == BNO055_OK);
    assert(q.w == -2.0f);
    assert(q.x == 32767/16384.0f);
    assert(q.y ==  0.000061f);
    assert(q.z ==  -1/16384.0f); 
}

static void test_read_calib(void)
{
    fake_i2c_t f;
    bno055_t dev = make_dev(&f);

    /* Case 1: sys = 3, gyr = 2, acc = 1, mag = 0 */
    f.regs[0x35] = 0xE4;
    bno055_calib_t c;
    
    assert(bno055_read_calib(&dev, &c) == BNO055_OK);
    assert(c.sys == 3U);
    assert(c.gyr == 2U);
    assert(c.acc == 1U);
    assert(c.mag == 0U);

    f.regs[0x35] = 0x1B;

    /* Case 2: sys = 0, gyr = 1, acc = 2, mag = 3 */
    assert(bno055_read_calib(&dev, &c) == BNO055_OK);
    assert(c.sys == 0U);
    assert(c.gyr == 1U);
    assert(c.acc == 2U);
    assert(c.mag == 3U);
}

int main(void)
{
    test_read_gyro_scaling();
    test_read_accel_scaling();
    test_read_quat_scaling();
    test_le16_sign_extremes();
    test_read_calib();

    printf("all tests passed\n");
    return 0;
}
