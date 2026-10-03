# Hardware

Traxxas brushed chassis (XL-5 ESC), Raspberry Pi 5 (Ubuntu 24.04, ROS 2 Jazzy), STM32
Nucleo-F446RE, Slamtec RPLIDAR C1.

Three environments, two ROS distros that **cannot talk to each other**:

| Where | OS | ROS 2 | Used for |
|---|---|---|---|
| Docker container `f1tenth_sim` | Ubuntu 20.04 | Foxy (EOL) | the F1TENTH sim only |
| Raspberry Pi 5 (`carpi`) | Ubuntu 24.04 Server | Jazzy | drivers, SLAM, eventually everything on the car |
| WSL (native, not in Docker) | Ubuntu 24.04 | Jazzy desktop | RViz / viewing the Pi's topics |

The Pi 5 needs Ubuntu 24.04: older kernels don't support its RP1 I/O chip, and 24.04 pins
you to Jazzy. That's why the real car and the sim are on different distros.

---

## M3 result — real LiDAR + handheld SLAM map

![Handheld SLAM map of a small room](images/room_handheld.png)

RPLIDAR C1 on the Pi publishing `/scan`, viewed live in RViz on the desktop, and a
slam_toolbox map built by carrying the LiDAR around a room.

The map has a **rotated duplicate of the room** overlaid on itself. That's expected for
handheld mapping with no odometry, not a bug in the setup (see
[why the room doubled](#why-the-room-doubled)). Re-mapping by hand wasn't worth it — the
fix is real wheel odometry, which is M4. The proper map comes in M5, on the car.

---

## Pi setup

### Flashing

Raspberry Pi Imager → **Ubuntu Server 24.04 LTS (64-bit)**. In the OS customization screen:

- hostname `carpi`, user `gphan`
- Wi-Fi SSID/password and country
- SSH enabled, **public-key auth only**, paste `~/.ssh/id_ed25519.pub` from WSL
  (the `.pub` — never the private key)

Running on a 16 GB card (the 128 GB one wouldn't boot). Enough for `ros-base` plus the
workspace; move to a bigger card or SSD if logs/bags fill it.

First boot takes a few minutes (cloud-init runs, resizes the filesystem, then may reboot).
Green LED flickering = SD activity, good. Solid red only = power but not booting.

### SSH from WSL

`~/.ssh/config`:

```
Host carpi
    HostName carpi.local
    User gphan
```

Then just `ssh carpi`. Notes:

- `Permission denied (publickey)` was the **wrong username** (WSL user ≠ Pi user), not a key
  problem.
- `carpi.local` (mDNS) only resolves from WSL with mirrored networking (below).
- After a re-flash the host key changes and SSH refuses to connect:
  `ssh-keygen -R carpi.local`.

### The `noble-updates` fix

`apt install ros-jazzy-ros-base` failed with unmet dependencies (`liblz4-dev`,
`libzstd-dev`, ...). Cause: the image's `/etc/apt/sources.list.d/ubuntu.sources` only listed
the `noble` suite. ROS packages are built against the **updated** versions of those
libraries, which live in `noble-updates`.

Ubuntu splits each release into suites:

| Suite | Contains |
|---|---|
| `noble` | packages as frozen at release |
| `noble-updates` | bug-fix updates since release |
| `noble-security` | security fixes |
| `noble-backports` | newer versions backported (opt-in) |

Diagnose with `apt-cache policy liblz4-1` — if the installed version comes from a suite not
in your sources, that's it. Fix:

```bash
sudo sed -i 's/^Suites: noble$/Suites: noble noble-updates noble-backports/' \
  /etc/apt/sources.list.d/ubuntu.sources
sudo apt update && sudo apt full-upgrade -y
```

(ARM boards pull from `ports.ubuntu.com`, not `archive.ubuntu.com` — same suites.)

### ROS 2 Jazzy

Standard ROS apt repo setup, then:

```bash
sudo apt install -y ros-jazzy-ros-base ros-dev-tools     # ros-base: no GUI, Pi is headless
sudo rosdep init && rosdep update
echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
```

Note the package name is `ros-jazzy-ros-base` (dashes), not `ros_base`.

### Temperatures

CanaKit case with fan. `vcgencmd measure_temp` — idle ~45 °C, LiDAR + SLAM well under the
80 °C throttle point. Check `vcgencmd get_throttled` (want `0x0`) if things get slow;
non-zero also flags undervoltage, which matters once it's on battery.

---

## LiDAR driver

```bash
mkdir -p ~/car_ws/src && cd ~/car_ws/src
git clone https://github.com/Slamtec/sllidar_ros2.git
cd ~/car_ws && rosdep install --from-paths src -y --ignore-src
colcon build --symlink-install
echo "source ~/car_ws/install/setup.bash" >> ~/.bashrc
```

Build warnings from the vendored SDK are harmless.

### Serial access

The C1 ships with a CP2102 USB-UART adapter → shows up as `/dev/ttyUSB0`.

```bash
sudo usermod -aG dialout $USER     # then log out and back in — groups apply at login
ls -l /dev/ttyUSB0
```

No `/dev/ttyUSB0`? Check `dmesg | tail` while plugging in. Mine was the USB-C end into the
adapter **not fully seated** — it powered nothing and enumerated nothing.

### Run it

```bash
ros2 launch sllidar_ros2 sllidar_c1_launch.py serial_port:=/dev/ttyUSB0
ros2 topic hz /scan      # second shell: ~10 Hz
```

The motor spins up when the driver starts. Scans are published in frame `laser`.

---

## Viewing the Pi's topics from WSL

Install the desktop variant in WSL (native, **not** in the Foxy container — Foxy can't see
Jazzy):

```bash
sudo apt install -y ros-jazzy-desktop
```

### Networking

ROS 2 discovery uses UDP multicast, which WSL's default NAT network drops. Two pieces:

1. `C:\Users\<you>\.wslconfig`:
   ```ini
   [wsl2]
   networkingMode=mirrored
   ```
2. Hyper-V firewall lets inbound traffic into WSL — PowerShell **as admin**:
   ```powershell
   Set-NetFirewallHyperVVMSetting -Name '{40E0AC32-46A5-438A-A0B2-2B479E8F2E90}' -DefaultInboundAction Allow
   ```

Then `wsl --shutdown` and reopen. Check: `ip addr` in WSL should show your real LAN IP
(same subnet as the Pi), not `172.x`.

Both machines must be on the same network and the same `ROS_DOMAIN_ID` (default 0).

### Check and view

```bash
ros2 topic list --no-daemon      # /scan should be there
rviz2
```

RViz: **Fixed Frame = `laser`** (there's no `map` or `base_link` yet), Add → By topic →
`/scan`.

If `ros2 topic list` shows nothing and `ros2 daemon stop` hangs, the CLI daemon cached an
empty graph from before networking was fixed: `pkill -f _ros2_daemon`, then use
`--no-daemon`.

### Reading a scan

The C1 is a DTOF (direct time-of-flight) sensor: it times a light pulse out and back, for
each angle, ~10 times a second. Each `sensor_msgs/LaserScan` is one revolution:

- `angle_min`, `angle_max`, `angle_increment` — which direction each sample points
- `ranges[]` — distance per angle in meters; `inf` = no return (too far, glass, black
  surfaces)
- `intensities[]` — return strength; RViz colors points by it by default

The dots in RViz are the walls/furniture at the LiDAR's height, seen from above. It's a 2D
slice — a table is invisible if the beam passes under it.

---

## Handheld SLAM

Carry the LiDAR around, slam_toolbox stitches scans into a map. Four shells on the Pi:

```bash
# 1. driver
ros2 launch sllidar_ros2 sllidar_c1_launch.py serial_port:=/dev/ttyUSB0

# 2. fake odometry: odom -> base_link fixed (we have no wheels yet)
ros2 run tf2_ros static_transform_publisher --frame-id odom --child-frame-id base_link

# 3. LiDAR mounted at base_link
ros2 run tf2_ros static_transform_publisher --frame-id base_link --child-frame-id laser

# 4. SLAM
ros2 launch slam_toolbox online_async_launch.py \
  slam_params_file:=$HOME/car_ws/config/mapper_real.yaml use_sim_time:=false
```

Note Jazzy's argument is `slam_params_file`; Foxy's was `params_file`.

`~/car_ws/config/mapper_real.yaml` is Jazzy's default `mapper_params_online_async.yaml`
with:

| Param | Value | Why |
|---|---|---|
| `base_frame` | `base_link` | default is `base_footprint`, which doesn't exist |
| `use_sim_time` | `false` | real hardware, wall clock |
| `minimum_travel_distance` | `0.0` | odometry never moves, so the default threshold would never add a scan |
| `minimum_travel_heading` | `0.0` | same |

Hold the LiDAR **level, motor axis vertical**, and turn slowly. In RViz set Fixed Frame
`map`, add `/map`.

### Why the room doubled

slam_toolbox doesn't search every possible pose for a new scan. It starts from the
odometry's guess of how far you moved and searches a limited window around it
(roughly ±20° of rotation). With the static `odom → base_link`, the guess is always
"didn't move". Rotate faster than that window between processed scans and the matcher
locks onto the wrong alignment — the room gets pasted in again, rotated.

This is the whole argument for wheel odometry + IMU (M4): a good motion guess is what keeps
scan matching in the right basin.

### Saving the map

```bash
sudo apt install -y ros-jazzy-nav2-map-server
mkdir -p ~/car_ws/maps
ros2 run nav2_map_server map_saver_cli -f ~/car_ws/maps/room_handheld
```

Produces `room_handheld.pgm` (the image: white free, black occupied, gray unknown) and
`room_handheld.yaml` (resolution, origin). Same name overwrites. Copy to the PC:

```bash
# from WSL
scp gphan@carpi.local:~/car_ws/maps/room_handheld.* ~/f1tenth_ws/src/car_bringup/maps/
```

Screenshot for the README: RViz view type **TopDownOrtho**, Fixed Frame `map`.

---

## Power (M4)

Bench: Pi on its official 27 W USB-C supply. On the car, one 2S/3S pack feeds everything:

```
 Battery ──► Y-harness ──┬──► XL-5 ESC ──► motor   (ESC's BEC also powers the servo)
   (fuse)                │
                         └──► 5 V / 6 A UBEC ──► 1000 µF cap ──► Pi GPIO pin 2 (5V), pin 6 (GND)
```

- **Polarity check before plugging into GPIO.** Back-feeding 5 V through the header bypasses
  the USB-C input's protection — reversed or >5.25 V kills the Pi. Meter it first.
- Powered via GPIO, the Pi can't negotiate 5 A and limits USB current. Add to
  `/boot/firmware/config.txt`:
  ```
  usb_max_current_enable=1
  ```
- Y-harness: battery-gauge wire on the main leg, thinner OK on the UBEC leg; inline fuse on
  the UBEC leg so a short on the logic side doesn't dump the pack.
- Cap at the Pi end absorbs drops when the motor draws a spike.
- Common ground between ESC, servo, STM32 and Pi — PWM is referenced to it.

## Traxxas XL-5 notes

- Servo-style PWM: 1.0 ms full reverse/left, **1.5 ms neutral**, 2.0 ms full forward/right,
  at 50 Hz.
- **Calibration** maps the ESC to the transmitter's (later: the STM32's) neutral and
  endpoints. Redo it once the STM32 drives it — its 1.5 ms won't match the stock radio's.
  Procedure: hold the set button with neutral applied, then full throttle, full reverse,
  release per the Traxxas manual's LED sequence.
- **Training mode** caps power at ~50%. Leave it on for first autonomy runs.
- **Low-voltage detection** must be on with a LiPo pack (over-discharge ruins it). Only turn
  it off for NiMH.

---

## M4 plan

Order of work, each step testable on its own:

1. Power harness (above), Pi running off the battery, `get_throttled` = `0x0`
2. STM32: servo PWM, sweep steering
3. STM32: ESC PWM, recalibrate the XL-5, wheels off the ground
4. Traxxas RPM sensor → timer input capture → wheel speed
5. BNO085 IMU over I2C
6. micro-ROS on the STM32: subscribe `/drive`, publish wheel speed + IMU
7. `robot_localization` EKF fuses wheels + IMU → `odom → base_link`
8. URDF from the measurements below (replaces the static transforms)

Parts:

| Part | Status |
|---|---|
| Traxxas #6520 RPM sensor + #6540 mount | have |
| 5 V / 6 A UBEC | to order |
| BNO085 breakout | to order |
| 1000 µF cap, connectors, fuse holder, wire | to order |
| Soldering gear | at home, bringing it back |

## Traxxas measurements

For the URDF and converter parameters. `base_link` = center of the rear axle, on the ground.

- [ ] Wheelbase (front axle to rear axle)
- [ ] Track width
- [ ] Max steering angle (full lock)
- [ ] Overall length and width (Nav2 footprint)
- [ ] LiDAR mount offset from `base_link` (x, y, z) once mounted — level and rigid matter more
      than millimeters

---
[← README](../README.md)
