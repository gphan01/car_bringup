# Architecture

## The converter node

`car_bringup/ackermann_converter.py` — subscribes `/cmd_vel_nav` (`Twist`), publishes `/drive`
(`AckermannDriveStamped`).

```
steering_angle = atan(wheelbase * angular.z / linear.x)
```

A twist says "rotate at ω while moving at v." A car can only do that by steering to an angle
that depends on its length — the bicycle model. Guarded for v ≈ 0 (a stopped car can't turn)
and clamped to `max_steering` (the wheels physically stop).

`wheelbase` (0.3302 m) and `max_steering` (0.4189 rad) are **parameters**, set for the sim car.
On the Traxxas, measure and override them — no code change.

**Why `/cmd_vel_nav` and not `/cmd_vel`:** the sim bridge also subscribes to `/cmd_vel` and does
its own crude conversion (steering jumps to ±0.3, no math). Listening on a separate topic keeps
the two from fighting. Teleop and Nav2 get remapped to `/cmd_vel_nav`.

```
teleop / Nav2  →  /cmd_vel_nav  →  ackermann_converter  →  /drive  →  sim (later: STM32)
```

## Frames

```
map                          absolute; from slam_toolbox; jumps on loop closure
└── odom                     from odometry; smooth; drifts
    └── ego_racecar/base_link
        ├── ego_racecar/laser
        └── wheels, hinges   static offsets from the URDF
```

- **Frame** = a coordinate system. **Transform** = how two frames relate (translation + rotation).
  Each frame stores only its link to its parent; TF chains them on request.
- `map → odom` is not a position — it's the **correction** for accumulated odometry drift.
- Controllers use `odom` (smooth). Goals use `map` (absolute).
- `ego_racecar/` is a namespace so two sim cars don't collide. The real car is plain `base_link`.

## Topics

| Topic | Type | Who |
|---|---|---|
| `/scan` | `sensor_msgs/LaserScan` | sim → slam_toolbox, RViz |
| `/ego_racecar/odom` | `nav_msgs/Odometry` | sim |
| `/map` | `nav_msgs/OccupancyGrid` | slam_toolbox |
| `/tf`, `/tf_static` | `tf2_msgs/TFMessage` | sim, slam_toolbox |
| `/cmd_vel_nav` | `geometry_msgs/Twist` | teleop / Nav2 → converter |
| `/drive` | `ackermann_msgs/AckermannDriveStamped` | converter → sim |
| `/cmd_vel` | `geometry_msgs/Twist` | sim's crude built-in path — unused |
| `/initialpose` | RViz "2D Pose Estimate" | resets the sim |

Teleop keys: `i` forward, `,` back, `j`/`l` turn, `u`/`o` forward+turn, `k` stop.
The teleop terminal needs focus.

---
[← README](../README.md)
