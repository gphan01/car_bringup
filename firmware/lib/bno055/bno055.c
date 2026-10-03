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
    VAL_UNIT_TRIGGER_EXT_CLK    = 0x80,
};

enum {
    DELAY_TO_CONFIG_MS = 19,
    DELAY_TO_OPMODE_MS = 7,
};

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
    bno055_status_t status;
    
    status = write_reg8(dev, REG_OPR_MODE, mode);
    if (status != BNO055_OK) 
    {
        return BNO055_ERR_BUS;
    }

    switch (mode){ 
    case VAL_MODE_CONFIG:
        dev->delay_ms(DELAY_TO_CONFIG_MS);
        break;
    default:
        dev->delay_ms(DELAY_TO_OPMODE_MS);
        break;
    }

    return status;
}










