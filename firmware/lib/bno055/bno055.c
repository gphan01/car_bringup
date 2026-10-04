#include "bno055.h"

enum {
    REG_CHIP_ID       = 0x00,
    REG_PAGE_ID       = 0x07,
    REG_ACC_DATA      = 0x08,
    REG_GYR_DATA      = 0x14,
    REG_QUA_DATA      = 0x20,
    REG_CALIB_STAT    = 0x35,
    REG_SYS_STATUS    = 0x39,
    REG_SYS_ERR       = 0x3A,
    REG_UNIT_SEL      = 0x3B,
    REG_OPR_MODE      = 0x3D,
    REG_SYS_TRIGGER   = 0x3F,
    REG_AXIS_MAP_CONFIG    = 0x41,
    REG_AXIS_MAP_SIGN      = 0x42,
    REG_ACC_OFFSETS_X_LSB            = 0x55,
};

enum {
    VAL_CHIP_ID        = 0xA0,
    VAL_MODE_CONFIG         = 0x00,
    VAL_MODE_IMU            = 0x08,
    VAL_UNIT_SEL_RAD_MS2        = 0x02,
    VAL_SYS_TRIGGER_EXT_CLK    = 0x80,
};

enum {
    DELAY_TO_CONFIG_MS = 19,
    DELAY_TO_OPMODE_MS = 7,
};

// Scaling factors 
static const float GYR_LSB_PER_RAD_S = 900.0f;
static const float ACC_LSB_PER_MS2 = 100.0f;
static const float QUA_LSB_PER_UNIT = 16384.0f;


static bno055_status_t read_regs(const bno055_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    int res = dev->read(dev->ctx, reg, buf, len);

    if (res != 0)
    {
        return BNO055_ERR_BUS;
    }

    return BNO055_OK;

}

static bno055_status_t write_regs(const bno055_t *dev, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    int res = dev->write(dev->ctx, reg, buf, len);

    if(res != 0)
    {
        return BNO055_ERR_BUS;
    }

    return BNO055_OK;
}

static bno055_status_t write_reg8(const bno055_t *dev, uint8_t reg, uint8_t val)
{
    return write_regs(dev, reg, &val, 1);
}

static bno055_status_t set_mode(const bno055_t *dev, uint8_t mode)
{
    bno055_status_t st;
    
    st = write_reg8(dev, REG_OPR_MODE, mode);
    if (st != BNO055_OK) 
    {
        return st;
    }

    switch (mode){ 
    case VAL_MODE_CONFIG:
        dev->delay_ms(DELAY_TO_CONFIG_MS);
        break;
    default:
        dev->delay_ms(DELAY_TO_OPMODE_MS);
        break;
    }

    return st;
}

/*
 * Read a 16-bit value that is stored little endian
 */
static int16_t le16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[1] << 8 | p[0]);
}

bno055_status_t bno055_read_gyro(const bno055_t *dev, bno055_vec3_t *out)
{
    if (!dev || !out) return BNO055_ERR_ARG; 

    uint8_t raw[6];

    bno055_status_t st = read_regs(dev, REG_GYR_DATA, raw, sizeof(raw));

    if (st != BNO055_OK)
    {
        return st;
    }

    out->x = le16(&raw[0]) / GYR_LSB_PER_RAD_S;

    out->y = le16(&raw[2]) / GYR_LSB_PER_RAD_S;

    out->z = le16(&raw[4]) / GYR_LSB_PER_RAD_S;
    return st;
}

bno055_status_t bno055_read_accel(const bno055_t *dev, bno055_vec3_t *out)
{
    if (!dev || !out) return BNO055_ERR_ARG; 

    uint8_t raw[6];

    bno055_status_t st = read_regs(dev, REG_ACC_DATA, raw, sizeof(raw));

    if (st != BNO055_OK)
    {
        return st;
    }

    out->x = le16(&raw[0]) / ACC_LSB_PER_MS2;

    out->y = le16(&raw[2]) / ACC_LSB_PER_MS2;

    out->z = le16(&raw[4]) / ACC_LSB_PER_MS2;
    return st;
}


bno055_status_t bno055_read_quat(const bno055_t *dev, bno055_quat_t *out)
{
    if (!dev || !out) return BNO055_ERR_ARG; 

    uint8_t raw[8];

    bno055_status_t st = read_regs(dev, REG_QUA_DATA, raw, sizeof(raw));

    if (st != BNO055_OK)
    {
        return st;
    }

    out->w = le16(&raw[0]) / QUA_LSB_PER_UNIT;
    
    out->x = le16(&raw[2]) / QUA_LSB_PER_UNIT;

    out->y = le16(&raw[4]) / QUA_LSB_PER_UNIT;

    out->z = le16(&raw[6]) / QUA_LSB_PER_UNIT;
    return st;
}


bno055_status_t bno055_read_calib(const bno055_t *dev, bno055_calib_t *out)
{
    if (!dev || !out) return BNO055_ERR_ARG;

    uint8_t buf;

    bno055_status_t st = read_regs(dev, REG_CALIB_STAT, &buf, sizeof(buf));

    if (st != BNO055_OK)
    {
        return st;
    }

    out->sys = (buf >> 6) & 0x03;
    out->gyr = (buf >> 4) & 0x03;
    out->acc = (buf >> 2) & 0x03;
    out->mag = (buf >> 0) & 0x03;
    
    return st;
}


bno055_status_t bno055_read_offsets(const bno055_t *dev, bno055_offsets_t *out)
{
    if(!dev || !out) return BNO055_ERR_ARG;

    bno055_status_t st = set_mode(dev, VAL_MODE_CONFIG);

    if (st != BNO055_OK)
    {
        return st;
    }

    bno055_offsets_t tmp;

    st = read_regs(dev, REG_ACC_OFFSETS_X_LSB, tmp.raw, sizeof(tmp.raw));
    
    bno055_status_t st2 = set_mode(dev, VAL_MODE_IMU);

    if (st != BNO055_OK)
    {
        return st;
    }

   if (st2 != BNO055_OK)
    {
        return st2;
    }

    *out = tmp;

    return BNO055_OK;
}








