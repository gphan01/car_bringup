/*
 * bno055.h - Hardware-independent driver for the Bosch BNO055 IMU.
 *
 * Runs the BNO055 in IMU fusion mode (accelerometer + gyroscope, no
 * magnetometer), clocked from the external 32.768 kHz crystal, with gyro
 * output in rad/s and accel output in m/s^2.
 *
 * Bus contract (implemented by a platform port, e.g. bno055_port.c):
 *   - read/write return 0 on success and nonzero on any failure.
 *   - The driver passes register addresses only. The port owns the I2C
 *     device address (7-bit, see BNO055_I2C_ADDR_A/B) and shifts it if
 *     its bus API wants the 8-bit form (STM32 HAL does).
 *   - delay_ms blocks for at least the requested number of milliseconds.
 *   - ctx is passed through untouched to read/write.
 *
 * All functions block until their bus transfers and delays complete.
 * The driver keeps no internal state, but it is not thread-safe: use one
 * bno055_t from a single task.
 */
#ifndef BNO055_H
#define BNO055_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ---- I2C addresses (7-bit) ---- */

    enum
    {
        BNO055_I2C_ADDR_A = 0x28, /* ADR pin low (Adafruit default) */
        BNO055_I2C_ADDR_B = 0x29, /* ADR pin high */
    };

    /* ---- Bus interface ---- */

    /* Read len bytes starting at register reg into buf. 0 = success. */
    typedef int (*bno055_read_fn)(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len);

    /* Write len bytes from buf starting at register reg. 0 = success. */
    typedef int (*bno055_write_fn)(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len);

    /* Block for at least ms milliseconds. */
    typedef void (*bno055_delay_fn)(uint32_t ms);

    /* Driver handle. Fill every field before calling bno055_init(). */
    typedef struct
    {
        bno055_read_fn read;
        bno055_write_fn write;
        bno055_delay_fn delay_ms;
        void *ctx; /* e.g. &hi2c1 on STM32, a fake register file in host tests */
    } bno055_t;

    /* ---- Data types ---- */

    typedef struct
    {
        float x, y, z;
    } bno055_vec3_t;

    typedef struct
    {
        float w, x, y, z;
    } bno055_quat_t;

    /* Calibration status, 0 (uncalibrated) to 3 (fully calibrated) per field.
     * mag always reads 0 in IMU mode because the magnetometer is unused. */
    typedef struct
    {
        uint8_t sys, gyr, acc, mag;
    } bno055_calib_t;

#define BNO055_OFFSETS_LEN (22U)

    /* Calibration offsets: an opaque copy of registers 0x55-0x6A.
     * Save once after calibrating, restore at every boot. Do not interpret
     * or modify the bytes. */
    typedef struct
    {
        uint8_t raw[BNO055_OFFSETS_LEN];
    } bno055_offsets_t;

    /* ---- Status codes ---- */

    typedef enum
    {
        BNO055_OK = 0,
        BNO055_ERR_BUS,     /* an I2C transfer failed */
        BNO055_ERR_ID,      /* a device answered, but CHIP_ID was not 0xA0 */
        BNO055_ERR_ARG,     /* invalid argument; nothing was sent on the bus */
        BNO055_ERR_TIMEOUT, /* no response during the boot poll */
        BNO055_ERR_STATE,   /* chip answered, but fusion is not running or SYS_ERR is set */
    } bno055_status_t;

    /* ---- API ---- */

    /*
     * Wait for the chip to boot, configure it, and start IMU fusion.
     *
     * Polls CHIP_ID for up to ~1 s, then: page 0, CONFIG mode, external
     * crystal, units (rad/s, m/s^2), optional offsets, IMU mode. Finally
     * verifies SYS_STATUS reports fusion running and SYS_ERR is clear.
     *
     * offsets: saved calibration to restore, or NULL to skip.
     *
     * Blocks ~40 ms if the chip is already booted, up to ~1.1 s right after
     * power-on. On any error the chip may be left in CONFIG mode.
     *
     * Returns OK, ERR_ARG, ERR_TIMEOUT, ERR_ID, ERR_BUS or ERR_STATE.
     */
    bno055_status_t bno055_init(const bno055_t *dev, const bno055_offsets_t *offsets);

    /* Angular rate in rad/s (GYR_DATA, 0x14). One 6-byte burst read.
     * out is left untouched on error. */
    bno055_status_t bno055_read_gyro(const bno055_t *dev, bno055_vec3_t *out);

    /* Acceleration in m/s^2, gravity included (ACC_DATA, 0x08). Lying flat
     * and still, z reads about +9.81. One 6-byte burst read.
     * out is left untouched on error. */
    bno055_status_t bno055_read_accel(const bno055_t *dev, bno055_vec3_t *out);

    /* Orientation as a unit quaternion (QUA_DATA, 0x20), w first.
     * In IMU mode the heading is relative to power-up and drifts slowly.
     * One 8-byte burst read. out is left untouched on error. */
    bno055_status_t bno055_read_quat(const bno055_t *dev, bno055_quat_t *out);

    /* Calibration status (CALIB_STAT, 0x35), each field 0-3.
     * out is left untouched on error. */
    bno055_status_t bno055_read_calib(const bno055_t *dev, bno055_calib_t *out);

    /*
     * Copy the chip's current calibration offsets into out.
     * Call once accel and gyro calibration both read 3, then store the bytes.
     *
     * Switches to CONFIG mode and back to IMU mode: blocks ~26 ms, and fusion
     * output is invalid during the call. IMU mode is restored even if the
     * transfer fails. out is left untouched on error.
     */
    bno055_status_t bno055_read_offsets(const bno055_t *dev, bno055_offsets_t *out);

    /*
     * Write saved calibration offsets to the chip.
     * Same mode switching and timing as bno055_read_offsets().
     * bno055_init() can do this for you at boot.
     */
    bno055_status_t bno055_write_offsets(const bno055_t *dev, const bno055_offsets_t *in);

    /*
     * Remap and/or flip the sensor axes to match the mounting orientation.
     *
     * config: which physical axis feeds each output axis, 2 bits each:
     *         bits 1:0 = X source, 3:2 = Y source, 5:4 = Z source,
     *         with 0 = X, 1 = Y, 2 = Z. Each axis must be used exactly once.
     *         Default (no remap) is 0x24.
     * sign:   bit 0/1/2 set = invert output X/Y/Z. Default is 0x00.
     *
     * Returns ERR_ARG without touching the bus if reserved bits are set or
     * the mapping is not a permutation (the chip would silently ignore it).
     * Otherwise switches to CONFIG and back: blocks ~26 ms, IMU mode is
     * restored even if the write fails.
     */
    bno055_status_t bno055_set_axis_remap(const bno055_t *dev, uint8_t config, uint8_t sign);

#ifdef __cplusplus
}
#endif

#endif /* BNO055_H */
