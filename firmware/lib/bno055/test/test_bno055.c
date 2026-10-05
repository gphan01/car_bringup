#include "../bno055.h"
#include "fake_i2c.h"   
#include <math.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

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
    assert(q.y ==  1/16384.0f);
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

static void test_read_null_args(void)
{
    bno055_vec3_t g;

    fake_i2c_t f;
    bno055_t dev =  make_dev(&f);

    assert(bno055_read_gyro(NULL, &g) == BNO055_ERR_ARG);
    assert(bno055_read_gyro(&dev, NULL) == BNO055_ERR_ARG);

    bno055_vec3_t a;
    assert(bno055_read_accel(NULL, &a) == BNO055_ERR_ARG);
    assert(bno055_read_accel(&dev, NULL) == BNO055_ERR_ARG);

    bno055_quat_t q;
    assert(bno055_read_quat(NULL, &q) == BNO055_ERR_ARG);
    assert(bno055_read_quat(&dev, NULL) == BNO055_ERR_ARG);

    bno055_calib_t c;
    assert(bno055_read_calib(NULL, &c) == BNO055_ERR_ARG);
    assert(bno055_read_calib(&dev, NULL) == BNO055_ERR_ARG);
}

static void test_read_bus_error(void)
{
    fake_i2c_t f; 
    
    bno055_t dev = make_dev(&f);
    f.fail_reads = 1;

    bno055_vec3_t g = { 123.0f, 123.0f, 123.0f};

    assert(bno055_read_gyro(&dev, &g) == BNO055_ERR_BUS);

    bno055_calib_t c = {  0xAA, 0xAA, 0xAA, 0xAA};

    assert(bno055_read_calib(&dev, &c) == BNO055_ERR_BUS);
}

static void test_read_offsets_data(void)
{
    fake_i2c_t f;
    bno055_t dev = make_dev(&f);

    for (size_t i = 0; i < 22; i++)
    {
        f.regs[0x55 + i] = i + 1;
    }

    bno055_offsets_t out;
    
    assert(bno055_read_offsets(&dev, &out) == BNO055_OK);

    assert(memcmp(out.raw, &f.regs[0x55], 22) == 0);

    /* Check if the chip was left in IMU mode. */
    assert(f.regs[0x3D] == 0x08);
}

static void expect_write(const fake_ev_t *ev, uint8_t reg, const uint8_t *data, uint16_t len)
{
    assert(ev->reg == reg);
    assert(ev->len == len);
    assert(memcmp(ev->data, data, len) == 0);
}

static void expect_delay(const fake_ev_t *ev, uint32_t ms)
{
    assert(ev->type == FAKE_EV_DELAY);
    assert(ev->ms == ms);
}

static void test_write_offsets_sequence(void)
{
    bno055_offsets_t offsets;

    fake_i2c_t f;
    bno055_t dev = make_dev(&f);

    for (size_t i = 0; i < 22; i++)
    {
        offsets.raw[i] = i + 1;
    }
   
    assert(bno055_write_offsets(&dev, &offsets) == BNO055_OK);

    assert(f.log_count == 5);

    /* The offset register can only be written in CONFIG mode, so the
     * the first thing on the bus should be OPR_MODE(0x3D) == CONFIG (0x00)
     */
    expect_write(&f.log[0], 0x3D, (uint8_t[]){0x00}, 1);

    /*
     * Switching to CONFIG takes 19ms
     */
    expect_delay(&f.log[1], 19);
    /*
     * The offsets are 22 bytes starting at 0x55(register map), one write
     * to 0x55 length 22, with exactly the bytes the caller passed in.
     */
    expect_write(&f.log[2], 0x55, offsets.raw, 22);
    /*
     * Fusion has to run again afterwards, so OPR_MODE=IMU(0x08)
     */
    expect_write(&f.log[3], 0x3D, (uint8_t[]){0x08}, 1);

    /*
     * Switching to an operating mode takes 7ms.
     */
    expect_delay(&f.log[4], 7);

    
}

int main(void)
{
    test_read_gyro_scaling();
    test_read_accel_scaling();
    test_read_quat_scaling();
    test_le16_sign_extremes();
    test_read_calib();

    test_read_null_args();
    test_read_bus_error();

    test_read_offsets_data();

    test_write_offsets_sequence();

    printf("All tests passed\n");
    return 0;
}
