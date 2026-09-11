# car_bringup

Configs, launch files, and notes for an autonomous 1/10-scale LiDAR vehicle running ROS 2.

Development happens in the F1TENTH gym simulator first; the same configs are intended to port to the physical car (Traxxas chassis, Raspberry Pi 5, STM32 Nucleo-F446RE, RPLIDAR C1).

## Status

| Milestone | State |
|---|---|
| M0 — sim running, teleop driving | done |
| M1 — SLAM building a map from scans | done |
| M2 — Nav2 autonomous goal navigation | not started |
| M3 — hardware sensor bring-up | waiting on parts |
| M4 — STM32 actuation + odometry firmware | not started |
| M5 — real-world SLAM | not started |
| M6 — autonomy on hardware | not started |

## Environment

Host is Windows with WSL2 (Ubuntu 24.04). The simulator runs in a Docker container because
`f1tenth_gym_ros` targets ROS 2 Foxy, which does not run on 24.04. The physical car will run
ROS 2 Jazzy on Ubuntu 24.04 — the Pi 5 cannot run 22.04 or older because its I/O moved to the
RP1 southbridge, which those kernels don't support.

So: Foxy in a container for the sim, Jazzy natively for real work. They never need to talk to
each other, and mixing ROS distros on one network doesn't work anyway.

## Running the simulator

### Native display via WSLg (preferred)

The repo's own `docker-compose.yml` routes RViz through noVNC and a browser, which is laggy —
every frame is encoded, shipped over HTTP, and decoded. WSLg already provides a display server,
so mounting its socket into the container lets RViz open as a native Windows window with GPU
acceleration.

From the `f1tenth_gym_ros` repo root:

```bash
docker run -it \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /mnt/wslg:/mnt/wslg \
  -e DISPLAY=$DISPLAY \
  -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \
  -e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR \
  -v .:/sim_ws/src/f1tenth_gym_ros \
  --name f1tenth_sim \
  f1tenth_gym_ros:latest
```

Notes:
- **No trailing `/bin/bash`.** The image sets `ENTRYPOINT ["/bin/bash"]`, so a trailing
  `/bin/bash` becomes an argument to bash and fails with
  `cannot execute binary file`.
- **Run from the repo root**, not the nested `f1tenth_gym_ros/f1tenth_gym_ros/` package dir —
  `-v .:` mounts the current directory.
- `--rm` is omitted deliberately so apt installs and `.bashrc` edits survive. Restart later with
  `docker start -ai f1tenth_sim`.
- Check `echo $DISPLAY` in WSL first; it should print `:0`.

### Fallback: noVNC

`docker compose up --build`, then browse to `http://localhost:8080/vnc.html`. Works everywhere,
but noticeably choppy.

### Additional shells

```bash
docker exec -it f1tenth_sim /bin/bash
```

## First-time container setup

The image ships without these:

```bash
apt update && apt install -y ros-foxy-slam-toolbox ros-foxy-tf2-tools

echo "source /opt/ros/foxy/setup.bash" >> ~/.bashrc
echo "source /sim_ws/install/local_setup.bash" >> ~/.bashrc

cd /sim_ws && colcon build --symlink-install && source install/local_setup.bash
```

Use `--symlink-install`. Without it, colcon **copies** source files into `install/`, so editing
a Python file changes nothing until you rebuild.

## Launching

Four panes. Don't Ctrl+C the first two.

```bash
# 1 — simulator
ros2 launch f1tenth_gym_ros gym_bridge_launch.py

# 2 — SLAM
ros2 launch slam_toolbox online_async_launch.py \
  params_file:=/sim_ws/src/car_bringup/config/mapper_params_online_async.yaml

# 3 — teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# 4 — scratch (topic echo, view_frames, etc.)
```

Teleop keys are the `i / j / k / l` block, not WASD. `k` stops. Leave the speed multiplier at
the default — `q`/`z` compound 10% per press and it's easy to reach absurd values.

## Required patches to f1tenth_gym_ros

Both live on the `odom-frame-for-slam` branch of the fork.

### 1. Publish `odom → base_link` instead of `map → base_link`

`gym_bridge.py`, lines ~292 and ~339: change `'map'` to `'odom'`.

The simulator knows ground truth and publishes the robot's position on the map directly. That's
the answer SLAM is supposed to compute, and two publishers claiming the same transform conflict.
After the change the sim publishes only odometry, leaving `map → odom` for slam_toolbox to own —
which is the standard arrangement on real hardware.

### 2. Fix the scan angle fencepost

`gym_bridge.py`, line 98:

```python
self.angle_inc = scan_fov / (scan_beams - 1)   # was: / scan_beams
```

1080 beams span 1079 increments, not 1080. Without this, slam_toolbox computes
`(angle_max - angle_min) / angle_increment + 1 = 1081`, sees 1080 readings, and **rejects every
scan** with `LaserRangeScan contains 1080 range readings, expected 1081`. Nothing errors loudly;
the map simply never builds.

This is an upstream bug and worth reporting.

## Frames

```
map                        ← absolute, from slam_toolbox. Accurate but jumps on loop closure.
└── odom                   ← from wheel odometry. Smooth but drifts.
    └── ego_racecar/base_link
        ├── ego_racecar/laser
        └── wheels, hinges (static, from URDF)
```

Both `map` and `odom` exist because neither property is sufficient alone. Control loops need
smooth (`odom`); goals need absolute (`map`). slam_toolbox publishes `map → odom`, which is not
a position but a *correction* — the accumulated error in odometry.

On real hardware: the STM32 publishes `odom → base_link` from encoder + IMU via micro-ROS,
`robot_state_publisher` supplies the static offsets from a URDF, and slam_toolbox still owns
`map → odom`.

## Gotchas

**`params_file:=`, not `slam_params_file:=`.** Foxy's `online_async_launch.py` declares the
short name. `ros2 launch` silently ignores unknown arguments, so the wrong name loads the
default config with no error. Check what a launch file accepts with:

```bash
ros2 launch <pkg> <file> --show-args
```

**`base_frame` must be `ego_racecar/base_link`.** slam_toolbox ships with `base_footprint`,
which doesn't exist in this sim's tree. Symptom is a flood of
`Invalid frame ID "base_footprint" ... frame does not exist`.

**RViz's Fixed Frame must name a frame that exists.** With patch 1 applied and slam_toolbox
*not* running, `map` is absent, so RViz draws nothing — no car, no scans, RobotModel red. This
looks exactly like a frozen simulator and isn't. Either set Fixed Frame to `odom` or start
slam_toolbox.

**Verify on topics, not in RViz.** A viewer showing nothing is ambiguous between "no data" and
"can't render the data." When something looks wrong:

```bash
ros2 topic echo /ego_racecar/odom     # is the car actually moving?
ros2 param get /slam_toolbox base_frame   # what does the node actually believe?
ros2 run tf2_tools view_frames.py     # is the tree connected?
```

`ros2 param get` is the one that matters most — a config file is what you intended, `param get`
is what the node loaded. When they disagree, the bug is in the loading path.

**Root-owned files.** `colcon build` inside the container writes to the mounted volume as root,
so `build/`, `install/`, and `__pycache__` can't be deleted from WSL without `sudo`.

## Topics

Published by the sim:

| Topic | Type |
|---|---|
| `/scan` | `sensor_msgs/LaserScan` |
| `/ego_racecar/odom` | `nav_msgs/Odometry` |
| `/map` | `nav_msgs/OccupancyGrid` (from slam_toolbox once running) |

Subscribed by the sim:

| Topic | Type |
|---|---|
| `/drive` | `ackermann_msgs/AckermannDriveStamped` |
| `/cmd_vel` | `geometry_msgs/Twist` (teleop path) |
| `/initialpose` | RViz "2D Pose Estimate" — resets the sim |

`AckermannDriveStamped` carries a **steering angle and speed**, not a twist. Nav2's controllers
emit `Twist`, so M2 needs a conversion — which is the same bicycle-model math the STM32 will do
on the real car:

```
steering_angle = atan(wheelbase * angular.z / linear.x)
```

## Next

M2: Nav2 with Regulated Pure Pursuit (works with Ackermann; the default DWB controller assumes
differential drive and will command in-place rotations a car can't execute) and a
kinematically-feasible global planner such as Smac Hybrid-A\* that respects minimum turning
radius.
