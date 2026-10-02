# Hardware

Traxxas brushed chassis, Raspberry Pi 5 (Ubuntu 24.04, ROS 2 Jazzy), STM32 Nucleo-F446RE, Slamtec RPLIDAR C1.

## M3 — real LiDAR (next)

RPLIDAR C1 publishing `/scan` on the Pi 5 via `sllidar_ros2` (Jazzy), visible in RViz.
Bench first, then SLAM by carrying it around a room. No chassis needed.

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
