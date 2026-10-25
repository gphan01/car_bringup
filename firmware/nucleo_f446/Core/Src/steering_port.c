#include "steering_port.h"
#include "steering.h"

static steering_status_t port_write(void *ctx, uint16_t pulse_us)
{
    TIM_HandleTypeDef *htim = ctx;
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, pulse_us);
    return STEERING_OK;
}

steering_status_t steering_port_init(steering_dev_t *dev, TIM_HandleTypeDef *htim)
{
    dev->write = port_write;
    dev->ctx = htim;
    port_write(dev->ctx, 1500);
    HAL_TIM_PWM_Start(htim, TIM_CHANNEL_1);
    return STEERING_OK;
}
