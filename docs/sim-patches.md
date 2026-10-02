# Required patches to f1tenth_gym_ros

On branch **`slam-fixes`** of `github.com/gphan01/f1tenth_gym_ros`.
A fresh upstream clone does **not** have these. `main` stays identical to F1TENTH's `main`
so upstream updates pull cleanly; rebase `slam-fixes` on top when needed.

Remotes live in the clone's `.git/config`, so a fresh clone forgets the fork. After re-cloning:
```bash
git remote add fork git@github.com:gphan01/f1tenth_gym_ros.git
git fetch fork
git checkout -b slam-fixes fork/slam-fixes
```

## 1. Publish `odom → base_link`, not `map → base_link`

`gym_bridge.py`, lines ~292 and ~339: `'map'` → `'odom'`.

The sim knows ground truth and publishes the car's map position directly — which is exactly
what SLAM is supposed to compute. After the patch the sim only publishes odometry, and
slam_toolbox owns `map → odom`, the same arrangement as the real car.

**Side effect:** with this patch, `map` only exists while slam_toolbox is running.

## 2. Scan angle fencepost

`gym_bridge.py`, line 98:
```python
self.angle_inc = scan_fov / (scan_beams - 1)   # upstream: / scan_beams
```

1080 beams span 1079 gaps. Without this, slam_toolbox expects 1081 readings, gets 1080, and
**silently rejects every scan** — map never builds. Log line:
`LaserRangeScan contains 1080 range readings, expected 1081`. Upstream bug; worth reporting.

## 3. Don't launch the prefab map server

`launch/gym_bridge_launch.py`: comment out the two lines that register the nodes.
```python
    # ld.add_action(nav_lifecycle_node)   # prefab map; slam_toolbox owns /map
    # ld.add_action(map_server_node)
```

`map_server` publishes the PNG track map on `/map`, latched. With slam_toolbox also publishing
`/map`, RViz blends the two and you get a doubled, rotated corridor. Same root problem as patch
1: the sim handing out ground truth that SLAM is supposed to produce.

`lifecycle_manager_localization` only exists to activate `map_server` (Nav2 nodes are
lifecycle nodes — they start idle and must be configured + activated).

After editing any patch, if behavior doesn't change, the build is stale:
```bash
cd /sim_ws
rm -rf build/f1tenth_gym_ros install/f1tenth_gym_ros
colcon build --symlink-install --packages-select f1tenth_gym_ros
```

---
[← README](../README.md)
