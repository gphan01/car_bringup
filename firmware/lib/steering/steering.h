#ifndef STEERING_H
#define STEERING_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    STEERING_OK = 0,
    STEERING_ERR_PARAM,
    STEERING_ERR_NOT_INIT,
    STEERING_ERR_IO,
} steering_status_t;

typedef steering_status_t (*steering_write_fn)(void *ctx, uint16_t pulse_us);

typedef struct
{
    uint16_t left_us;
    uint16_t center_us;
    uint16_t right_us;
    float max_left_rad;
    float max_right_rad;
} steering_cal_t;

typedef struct
{
    steering_write_fn write;
    steering_cal_t cal;
    bool initialized;
    void *ctx;
    uint32_t clamp_count;
} steering_dev_t;

steering_status_t steering_init(steering_dev_t *dev, const steering_cal_t *cal);
steering_status_t steering_set_pulse_us(steering_dev_t *dev, uint16_t pulse_us);
steering_status_t steering_set_angle(steering_dev_t *dev, float angle_rad);

#endif
