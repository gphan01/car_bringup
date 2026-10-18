# BNO055 driver

Hardware-independent C11 driver for the Bosch BNO055 IMU, used on the car's
STM32 Nucleo-F446RE (Adafruit breakout on I2C1, PB8/PB9).

- Runs in **IMU fusion mode**: accelerometer + gyroscope, no magnetometer
  (the motor's field makes the magnetometer unreliable indoors).
- External 32.768 kHz crystal, gyro in **rad/s**, accel in **m/s²**.
- No HAL, no globals, no dynamic memory. All bus access goes through three
  function pointers, so the same `bno055.c` runs on the STM32 and on a PC.

## Files

| File | Purpose |
|---|---|
| `bno055.h` | Public API and bus contract |
| `bno055.c` | Driver |
| `test/` | Host tests: fake I2C register file + test runner |

## Porting

Implement three functions for your platform and put them in a `bno055_t`:

```c
int  read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len);        /* 0 = OK */
int  write(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len); /* 0 = OK */
void delay_ms(uint32_t ms);
```

- Return 0 on success and nonzero on any failure. The driver maps every
  failure to `BNO055_ERR_BUS`.
- The driver never sees the I2C device address. The port owns it: use
  `BNO055_I2C_ADDR_A` (0x28, 7-bit) and shift it if your bus API wants the
  8-bit form. **STM32 HAL does:** pass `BNO055_I2C_ADDR_A << 1`.
- Reads and writes must be single burst transfers with a repeated start
  (`HAL_I2C_Mem_Read` / `HAL_I2C_Mem_Write` on STM32).
- `ctx` is passed through untouched, e.g. `&hi2c1`.

The STM32 port lives in `nucleo_f446/Core/Src/bno055_port.c`.

## Usage

```c
bno055_t dev = {0};
bno055_port_init(&dev, &hi2c1);

bno055_status_t st = bno055_init(&dev, NULL);   /* or &saved_offsets */

bno055_vec3_t gyro, accel;
bno055_read_gyro(&dev, &gyro);    /* rad/s */
bno055_read_accel(&dev, &accel);  /* m/s^2, gravity included */
```

Initialize `dev` with `{0}`: an uninitialized function pointer would jump to
garbage instead of failing with `ERR_ARG`.

## Error codes

| Code | Meaning | First thing to check |
|---|---|---|
| `ERR_ARG` | NULL or invalid argument; nothing was sent | caller code |
| `ERR_TIMEOUT` | nothing answered during the ~1 s boot poll | power, wiring, SDA/SCL swapped, address shift |
| `ERR_ID` | a device answered, CHIP_ID was not 0xA0 | wrong device or wrong ADR setting |
| `ERR_BUS` | an I2C transfer failed after the chip was found | loose wire, bus noise, clock stretching |
| `ERR_STATE` | fusion did not start, or SYS_ERR is set | read SYS_STATUS / SYS_ERR (0x39 / 0x3A) |

## Timing

| Call | Blocks for |
|---|---|
| `bno055_init` | ~40 ms if already booted, up to ~1.1 s after power-on |
| `read_offsets`, `write_offsets`, `set_axis_remap` | ~26 ms (CONFIG round trip); fusion output invalid meanwhile |
| `read_gyro`, `read_accel`, `read_quat`, `read_calib` | one I2C transfer, under 1 ms at 100 kHz |

Call the mode-switching functions at startup, not from the control loop.

## Calibration

Offsets live in RAM on the BNO055 and are lost at power-off, so calibrate
once and restore at every boot:

1. Run with `bno055_init(&dev, NULL)` and poll `bno055_read_calib()`.
2. Gyro: keep the board still for a few seconds until `gyr == 3`.
3. Accel: hold the board still in ~6 orientations (each face up/down),
   a few seconds each, until `acc == 3`.
4. `bno055_read_offsets()` and copy the 22 bytes into a `const
   bno055_offsets_t` in the firmware.
5. Pass that array to `bno055_init()` from then on.

Offsets belong to one specific chip in one mounting. Recalibrate after
swapping the breakout.

## Axis remap

`bno055_set_axis_remap(dev, config, sign)` maps the sensor axes to the
car frame (ROS REP 103: x forward, y left, z up). See the comment in
`bno055.h` for the bit layout. Invalid mappings return `ERR_ARG` instead of
being silently ignored by the chip.

## Host tests

Run from this directory, in Linux/WSL:

```bash
make -C test test                         # build and run
valgrind --error-exitcode=1 ./test/test_bno055
bear -- make -C test                      # regenerate compile_commands.json for clangd
```

The fake I2C (`test/fake_i2c.c`) is a 256-byte register file with:

- an event log of every write and delay, for checking exact sequences
- `fail_reads`: countdown, the next N reads fail
- `fail_write_reg`: writes starting at that register fail (-1 = off)
- `read_count`: number of read attempts

Tests cover scaling and sign handling for every read, NULL arguments, bus
errors leaving outputs untouched, offset and remap mode switching with
restore-on-failure, and every `init` path: happy path, offsets, slow boot,
fusion not running, and a write failing mid-init.

## Hardware notes

- The chip needs ~650 ms after power-on before it answers. `init` polls for it.
- Configuration registers (units, offsets, remap) only accept writes in
  CONFIG mode. Writes in IMU mode are silently ignored.
- Mode switches need 19 ms into CONFIG and 7 ms out of it.
- The BNO055 stretches the I2C clock heavily. STM32 I2C handles it; the
  Raspberry Pi's I2C controller does not, so keep the IMU on the MCU.
