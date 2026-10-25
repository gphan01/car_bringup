#ifndef STEERING_PORT_H
#define STEERING_PORT_H

#include "stm32f4xx_hal.h"
#include "steering.h"

steering_status_t steering_port_init(steering_dev_t *dev, TIM_HandleTypeDef *htim);

static steering_status_t port_write(void *ctx, uint16_t pulse_us);

#endif
