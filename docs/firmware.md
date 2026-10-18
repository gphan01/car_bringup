# Firmware (STM32 Nucleo-F446RE)

The STM32 replaces the RC receiver. It generates the steering and throttle PWM, reads wheel speed
and the BNO055 IMU, and later talks to the Pi over micro-ROS. It's powered, and talks to the
Pi, over the Pi's USB. Power and the harness are in [hardware.md](hardware.md); the overall order
of work is in hardware.md → "M4 plan".

## Status

| Module | What it does | State |
|---|---|---|
| `bno055` | I2C IMU: gyro (rad/s), accel (m/s²), quaternion, calibration offsets | **done**: driver + port + host tests; hardware check open |
| Steering PWM | TIM3 CH1 → servo | **next** |
| Throttle PWM | TIM3 CH2 → XL-5 ESC | — |
| Wheel speed | Traxxas RPM sensor → timer input capture | — |
| micro-ROS | `/drive` in; IMU + wheel speed out over USB | — |

## Pins

| Function | Peripheral | Pin | Notes |
|---|---|---|---|
| Steering servo | TIM3 CH1 | PA6 (D12) | 1 MHz tick, ARR 19999 (50 Hz) |
| ESC throttle | TIM3 CH2 | PA7 (D11) | same timer and period |
| BNO055 | I2C1 | SCL/SDA (D15/D14) | 7-bit addr 0x28 (HAL wants `0x28 << 1`) |

Confirm the pins are free in CubeMX before wiring. TIM3 is on APB1: read its clock off the
CubeMX clock tree and set `PSC = timer_clock / 1 MHz − 1` (89 at a 90 MHz timer clock).

## Wiring without the receiver

The receiver used to be the junction where the ESC's BEC powered the servo. Now a mini
breadboard does that job (servo/ESC plugs are 2.54 mm, so they plug straight in):

- ESC red (6 V BEC) ↔ servo red **only**. It never goes to the Nucleo; 6 V on a 3.3 V pin kills it.
- ESC black, servo black and Nucleo GND are all tied together. PWM needs the common reference.
- ESC signal → PA7, servo signal → PA6. A 3.3 V signal is fine for both (input threshold
  ~1.5–2.5 V).
- The ESC must be on for the servo to get power. Keep the ESC signal disconnected (or the wheels
  in the air) until the throttle step; an ESC on a floating input can do odd things.

## Steering and throttle PWM

Servo-style pulses at 50 Hz: **1.0 ms = full left/reverse, 1.5 ms = center/neutral,
2.0 ms = full right/forward**. With a 1 MHz tick, the compare value *is* the pulse width in µs.

- Load `CCR = 1500` **before** enabling the outputs, so the ESC's first pulse is neutral.
- `*_set_us()` setters clamp to 1000–2000, so a bug higher up can't send a 3 ms pulse.
- **Steering endpoints:** the linkage hits its mechanical stops before 1000/2000 µs. Step out
  from 1500 in small increments, record where the wheels stop turning, and use those as the
  limits. Driving a servo into a stop stalls it and cooks it. Also measure the steering angle at
  each endpoint (→ µs per degree, and `max_steering` for the converter).
- **ESC:** recalibrate the XL-5 to the STM32's neutral/endpoints (hardware.md → "Traxxas XL-5
  notes"). Leave Training mode on for the first runs. Wheels off the ground.

## Wheel speed

Traxxas #6520 RPM sensor + magnet on the spur gear, one pulse per spur revolution. Timer input
capture measures the period between pulses:

`v = (pulses/s ÷ 2.72) × π × 0.1065 m`. That's the 2.72:1 transmission and a 106.5 mm tire.

## BNO055 driver

`firmware/lib/bno055/`: platform-independent core (read/write/delay callbacks + `ctx`),
STM32 port (`bno055_port.c`, HAL blocking I2C), host unit tests against a fake register bus
with failure injection (init sequence, mode restore on failure, scaling, sign extension,
axis-remap validation).

Init: poll CHIP_ID (retries on NACK *and* on a wrong ID, 1 s timeout) → page 0 → CONFIG →
external crystal → units rad/s + m/s² → optional offsets → IMU mode → verify SYS_STATUS = fusion
running and SYS_ERR clear.

Open items:
- [ ] `AXIS_MAP_SIGN` header comment: check bit order against datasheet Table 3-9 (likely
      bit 2 = X, bit 1 = Y, bit 0 = Z). Matters when setting the mounting remap.
- [ ] Port: use one address macro for read and write.
- [ ] I2C bus-recovery routine at startup (clock SCL up to 9× if SDA is stuck low), before the
      I2C init.
- [ ] Tests: don't put driver calls inside `assert()` (they vanish under `NDEBUG`); check
      `ev->type` in `expect_write`; assert the untouched-on-error sentinels; log reads in the
      fake to prove CONFIG-before-read ordering.
- [ ] Hardware check: CHIP_ID 0xA0; gyro z **positive** when rotated CCW seen from above.
- [ ] When moving to FreeRTOS/micro-ROS: port delay → `vTaskDelay`, HAL timebase off SysTick.

---
[← README](../README.md) · [Hardware](hardware.md)
