# car_bringup

Autonomous 1/10-scale LiDAR car on ROS 2: SLAM-built maps, Nav2 path planning, and Ackermann
control. Developed in the F1TENTH gym simulator, then ported to hardware - Traxxas chassis,
Raspberry Pi 5, STM32 Nucleo-F446RE, Slamtec RPLIDAR C1.

## Status

| Milestone | What "done" means | State |
|---|---|---|
| M0 | Sim running, drive with keyboard | done |
| M1 | slam_toolbox builds a map from scans | done |
| M2 | Click a goal in RViz, Nav2 drives there | **done** - stalls on tight turnaround loops ([known issues](docs/nav2.md#known-issues)) |
| M3 | Real LiDAR publishing `/scan` on the Pi, handheld SLAM map | **done** ([hardware](docs/hardware.md)) |
| M4 | STM32: PWM to ESC/servo, wheel odometry, IMU, power harness | **next** |
| M5 | Real-world SLAM, driving by teleop | - |
| M6 | Autonomy on hardware | - |

<img src="docs/images/room_handheld.png" width="400" alt="Handheld SLAM map from the real RPLIDAR C1">

*M3: first map from the real LiDAR, carried by hand (no odometry yet, hence the doubled room).*

## How it fits together

```
 LiDAR ──/scan──► slam_toolbox ──/map, map→odom──► Nav2 ──/cmd_vel_nav──► ackermann_converter
   ▲                                                ▲                            │
   │              odometry ──odom→base_link─────────┘                       /drive
   │                                                                             ▼
   └──────────────────── sim bridge (now) / RPLIDAR + STM32 (hardware) ◄─────────┘
```

Everything above the bottom line is identical in sim and on the car - only the source of
`/scan` and odometry, and the consumer of `/drive`, change. Details:
[architecture](docs/architecture.md).

## Quick start (normal session)

1. Start **Docker Desktop** on Windows. Nothing below works without it.
2. In WSL:
   ```bash
   sim        # alias for ~/f1tenth_ws/src/car_bringup/sim.sh
   ```
3. Extra shells into the same container, from other WSL tabs:
   ```bash
   simsh      # alias for docker exec -it f1tenth_sim /bin/bash
   ```
   Confirm the sim patches are checked out:
   `git -C ~/f1tenth_ws/src/f1tenth_gym_ros branch --show-current` → `slam-fixes`
4. Launch everything with one command:
   ```bash
   ros2 launch /sim_ws/src/car_bringup/launch/sim_bringup_launch.py 2>&1 | tee /tmp/run.log
   ```
   Starts sim + RViz, slam_toolbox, the converter, then Nav2 after 8 s (so `map` exists first).
   Wait for `Managed nodes are active`. **Ctrl+C stops all of it.**

   Optional, separate pane - manual driving (stop it before sending Nav2 goals; both publish
   to `/cmd_vel_nav` and fight):
   ```bash
   ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r cmd_vel:=cmd_vel_nav
   ```
5. RViz: **Fixed Frame = `map`**. Useful displays (Add → By topic): `/plan` (Path),
   `/local_costmap/costmap` and `/global_costmap/costmap` (Map, color scheme `costmap`).
6. Send goals with **2D Goal Pose**. Never use **2D Pose Estimate** while SLAM runs - it
   teleports the sim car and SLAM stitches the map at the wrong pose (doubled corridors).
   Drag the arrow roughly along the car's current heading.

First time, or after the container is wiped: [sim setup](docs/sim-setup.md).
Clean reset when things get confusing: `docker restart f1tenth_sim` from WSL.

## Docs

| | |
|---|---|
| [Sim setup](docs/sim-setup.md) | WSL, Docker, container, aliases, where things run, workspace layout |
| [Architecture](docs/architecture.md) | converter node, TF frames, topics |
| [Sim patches](docs/sim-patches.md) | the three `f1tenth_gym_ros` fixes and why |
| [Nav2](docs/nav2.md) | config changes for Ackermann, known issues, tuning |
| [Troubleshooting](docs/troubleshooting.md) | symptom → cause → fix |
| [Hardware](docs/hardware.md) | Pi 5 + Jazzy setup, LiDAR, handheld SLAM, power, M4 plan |

## Repo layout

```
car_bringup/
├── car_bringup/ackermann_converter.py   Twist → AckermannDriveStamped node
├── config/        slam_toolbox + Nav2 params
├── launch/        sim_bringup_launch.py, nav2_launch.py
├── maps/          saved maps (.pgm + .yaml)
├── docs/          (+ docs/images/)
├── sim.sh         start/create the sim container
└── setup_container.sh
```
