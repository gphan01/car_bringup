#include "steering.h"

enum
{
    STEERING_PULSE_MIN_US = 1000,
    STEERING_PULSE_MAX_US = 2000,
};

static bool pulse_in_range(uint16_t pulse_us)
{
    return pulse_us >= STEERING_PULSE_MIN_US && pulse_us <= STEERING_PULSE_MAX_US;
}

steering_status_t steering_init(steering_dev_t *dev, const steering_cal_t *cal)
{
    if (!dev || !cal || !dev->write) return STEERING_ERR_PARAM;
    dev->initialized = false;
    dev->clamp_count = 0;

    if (!pulse_in_range(cal->left_us) || !pulse_in_range(cal->center_us) ||
        !pulse_in_range(cal->right_us))
    {
        return STEERING_ERR_PARAM;
    }

    bool left_low = (cal->left_us < cal->center_us) && (cal->center_us < cal->right_us);
    bool left_high = (cal->right_us < cal->center_us) && (cal->center_us < cal->left_us);
    if (!left_low && !left_high) return STEERING_ERR_PARAM;

    dev->cal = *cal;
    /* Move the servo to the center*/
    if (dev->write(dev->ctx, dev->cal.center_us) != STEERING_OK) return STEERING_ERR_IO;

    dev->initialized = true;

    return STEERING_OK;
}

steering_status_t steering_set_pulse_us(steering_dev_t *dev, uint16_t pulse_us)
{
    if (!dev) return STEERING_ERR_PARAM;

    if (!dev->initialized) return STEERING_ERR_NOT_INIT;

    /* Determin the bounds*/
    uint16_t lo = (dev->cal.left_us < dev->cal.right_us) ? dev->cal.left_us : dev->cal.right_us;
    uint16_t hi = (dev->cal.left_us < dev->cal.right_us) ? dev->cal.right_us : dev->cal.left_us;

    if (pulse_us < lo)
    {
        pulse_us = lo;
        dev->clamp_count++;
    }
    else if (pulse_us > hi)
    {
        pulse_us = hi;
        dev->clamp_count++;
    }

    return dev->write(dev->ctx, pulse_us);
}
