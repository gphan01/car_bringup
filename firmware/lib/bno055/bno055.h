#ifndef BNO055_H
#define BNO055_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define BNO055_OFFSETS_LEN  (22U)

typedef struct {uint8_t raw[BNO055_OFFSETS_LEN]; } bno055_offsets_t; // regs 0x55-0x6A, opaque

enum {
    BNO055_I2C_ADDR_A = 0x28, /* 7-bit, ADR low */
    BNO055_I2C_ADDR_B = 0x29, /* 7-bit, ADR high */
};

typedef int (*bno055_read_fn)(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len);
typedef int (*bno055_write_fn)(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len);
typedef void (*bno055_delay_fn)(uint32_t ms);

typedef struct {
    bno055_read_fn read;
    bno055_write_fn write;
    bno055_delay_fn    delay_ms;
    void          *ctx;  // e.g. &hi2c1 on the STM32, a fake register array on the PC
} bno055_t;

typedef struct { float x, y, z; } bno055_vec3_t;
typedef struct { float w, x, y, z; } bno055_quat_t;
typedef struct { uint8_t sys, gyr, acc, mag; } bno055_calib_t;

typedef enum {
    BNO055_OK = 0,
    BNO055_ERR_BUS,
    BNO055_ERR_ID,
    BNO055_ERR_ARG,
    BNO055_ERR_TIMEOUT,
} bno055_status_t;


bno055_status_t bno055_init(const bno055_t *dev, bno055_offsets_t *offsets);
bno055_status_t bno055_read_gyro(const bno055_t *dev, bno055_vec3_t *out); // rad/s
bno055_status_t bno055_read_quat(const bno055_t *dev, bno055_quat_t *out); // unit quaternion
bno055_status_t bno055_read_accel(const bno055_t *dev, bno055_vec3_t *out); // m/s^2 ACC (0x08) 
bno055_status_t bno055_read_calib(const bno055_t *dev, bno055_calib_t *out); // 0-3 each
bno055_status_t bno055_set_axis_remap(const bno055_t *dev, uint8_t config, uint8_t sign);

bno055_status_t bno055_read_offsets(const bno055_t *dev, bno055_offsets_t *out);
bno055_status_t bno055_write_offsets(const bno055_t *dev, const bno055_offsets_t *in);

#ifdef __cplusplus
}
#endif

#endif  // BNO055_H
