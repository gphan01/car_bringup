# car_bringup

Configs, nodes, and notes for an autonomous 1/10-scale LiDAR car on ROS 2.

Everything is developed in the F1TENTH gym simulator first, then ported to the real car
(Traxxas chassis, Raspberry Pi 5, STM32 Nucleo-F446RE, RPLIDAR C1).

## Status

| Milestone | What "done" means | State |
|---|---|---|
| M0 | Sim running, drive with keyboard | done |
| M1 | slam_toolbox builds a map from scans | done |
| M2 | Click a goal in RViz, Nav2 drives there | in progress — converter done, SLAM healthy, Nav2 config next |
| M3 | Real LiDAR publishing `/scan` | LiDAR in hand |
| M4 | STM32: PWM to ESC/servo, encoder odometry | chassis arrived |
| M5 | Real-world SLAM, driving by teleop | — |
| M6 | Autonomy on hardware | — |

---

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
   Aliases, once, in WSL:
   ```bash
   echo "alias sim='~/f1tenth_ws/src/car_bringup/sim.sh'" >> ~/.bashrc
   echo "alias simsh='docker exec -it f1tenth_sim /bin/bash'" >> ~/.bashrc
   ```
   Confirm the sim patches are checked out:
   `git -C ~/f1tenth_ws/src/f1tenth_gym_ros branch --show-current` → `slam-fixes`
4. Launch, one per pane. Don't Ctrl+C panes 1 and 2 while working.
   ```bash
   # 1 — simulator (opens RViz as a Windows window)
   ros2 launch f1tenth_gym_ros gym_bridge_launch.py

   # 2 — SLAM
   ros2 launch slam_toolbox online_async_launch.py \
     params_file:=/sim_ws/src/car_bringup/config/mapper_params_online_async.yaml \
     use_sim_time:=false

   # 3 — Twist → Ackermann converter
   ros2 run car_bringup ackermann_converter

   # 4 — keyboard driving, routed through the converter
   ros2 run teleop_twist_keyboard teleop_twist_keyboard \
     --ros-args -r cmd_vel:=cmd_vel_nav
   ```
5. RViz: **Fixed Frame = `map`** (only exists while slam_toolbox runs).

### First time / after the container is wiped

`sim.sh` creates the container if it's missing. Then, **inside** it:
```bash
bash /sim_ws/src/car_bringup/setup_container.sh
```
Then from **WSL**:
```bash
sudo chown -R $USER:$USER ~/f1tenth_ws/src
```

---

## Where things run

| Thing | Where | Why |
|---|---|---|
| `ros2 …`, `colcon …`, `apt install ros-…` | container | ROS only exists there |
| `git`, `nvim`, SSH keys | WSL | your config and keys live there |
| `docker …` | WSL | acts on containers from the outside |
| `sudo chown` | WSL | container creates files as root |

The mount makes one directory visible from both sides:

```
WSL:        ~/f1tenth_ws/src/          (car_bringup/, f1tenth_gym_ros/)
container:  /sim_ws/src/               same files
```

Container-only (lost if the container is deleted): apt installs, `~/.bashrc`, `~/.tmux.conf`,
`/sim_ws/build`, `/sim_ws/install`. That's what `setup_container.sh` restores.

**Why a container at all:** `f1tenth_gym_ros` targets ROS 2 Foxy (Ubuntu 20.04). WSL is 24.04.
The real car will run Jazzy on 24.04, because the Pi 5's RP1 I/O chip isn't supported by older
kernels. So: Foxy in a container for the sim, Jazzy for the car.

---

## Workspace layout

```
/sim_ws/                               WORKSPACE (colcon build from here)
├── src/
│   ├── f1tenth_gym_ros/               package — the simulator (forked, 3 patches)
│   └── car_bringup/                   package — mine
│       ├── package.xml                name + dependencies
│       ├── setup.py                   install rules + entry_points (registers nodes)
│       ├── config/
│       │   ├── mapper_params_online_async.yaml
│       │   └── nav2_params.yaml
│       ├── car_bringup/
│       │   └── ackermann_converter.py node
│       ├── sim.sh
│       └── setup_container.sh
├── build/  install/  log/             colcon output — never edit
```

**Workspace → package → node.** A folder is a package once it has `package.xml` + `setup.py`.
A node is a Python file in the inner folder. `ros2 run <pkg> <exe>` only finds it if it's
registered in `setup.py` `entry_points` **and** the package was rebuilt afterward.

Rebuild only after changing `setup.py` or `package.xml`. With `--symlink-install`, editing a
`.py` file just needs a node restart.

---

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

---

## Required patches to f1tenth_gym_ros

On branch **`slam-fixes`** of `github.com/gphan01/f1tenth_gym_ros`.
A fresh upstream clone does **not** have these. `main` stays identical to F1TENTH's `main`
so upstream updates pull cleanly; rebase `slam-fixes` on top when needed.

Remotes live in the clone's `.git/config`, so a fresh clone forgets the fork. After re-cloning:
```bash
git remote add fork git@github.com:gphan01/f1tenth_gym_ros.git
git fetch fork
git checkout -b slam-fixes fork/slam-fixes
```

### 1. Publish `odom → base_link`, not `map → base_link`

`gym_bridge.py`, lines ~292 and ~339: `'map'` → `'odom'`.

The sim knows ground truth and publishes the car's map position directly — which is exactly
what SLAM is supposed to compute. After the patch the sim only publishes odometry, and
slam_toolbox owns `map → odom`, the same arrangement as the real car.

**Side effect:** with this patch, `map` only exists while slam_toolbox is running.

### 2. Scan angle fencepost

`gym_bridge.py`, line 98:
```python
self.angle_inc = scan_fov / (scan_beams - 1)   # upstream: / scan_beams
```

1080 beams span 1079 gaps. Without this, slam_toolbox expects 1081 readings, gets 1080, and
**silently rejects every scan** — map never builds. Log line:
`LaserRangeScan contains 1080 range readings, expected 1081`. Upstream bug; worth reporting.

### 3. Don't launch the prefab map server

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

---

## Gotchas that cost real time

**This sim publishes no `/clock` → `use_sim_time:=false` on every launch.** With it `true`,
nodes read time from `/clock`; nobody publishes it, so their clock sits at 0 while scans are
stamped with wall time. slam_toolbox then drops every scan:
`Message Filter dropping message: frame 'ego_racecar/laser' … for reason 'Unknown'`.
Diagnosis: `view_frames` showed slam's `map → odom` stamped `0.2` vs sim transforms at
~`1.79e9`; `ros2 topic info /clock` showed 0 publishers. Pass it as a **launch argument** —
Foxy's launch files apply `{'use_sim_time': …}` after the YAML, so a YAML value gets
overridden. Same for Nav2. (`/clock` appearing in `ros2 topic list` means nothing — the list
includes topics that only have subscribers.)

**RViz process runs but no window appears → `wsl --shutdown` from PowerShell, then restart.**
The container mounts WSLg's display socket at creation; after sleep/reboot WSLg makes a new
one and the container points at a dead socket. Test WSLg alone with `xeyes` in plain WSL.

**A few "dropping message" lines are normal.** A scan that arrives milliseconds before its
transform gets dropped and the next one is used. Only a continuous flood with no map growth
is a problem. Judge by whether the map builds, not by the log.

**When something that worked stops working, list what changed since.** The M2 regressions all
traced to the re-clone, the container rebuild, the mount change, and the config move —
not to new bugs.

**RViz Fixed Frame must be a frame that exists.** Symptoms of a wrong one look exactly like a
frozen simulator: car won't move, no lasers, RobotModel red, white box instead of the car.
The car was driving the whole time. Check Global Status first. Use `odom` if slam_toolbox isn't
running.

**Verify on topics before debugging the system.** RViz showing nothing is ambiguous between
"no data" and "can't draw the data."
```bash
ros2 topic echo /ego_racecar/odom           # is the car actually moving?
ros2 node list                              # is the node actually alive?
ros2 param get /slam_toolbox base_frame     # what did the node actually load?
ros2 run tf2_tools view_frames.py           # is the tree connected?
ros2 topic info /drive --verbose            # who publishes / subscribes?
```

**Walk the chain one link at a time.** Converter debugging went: `/cmd_vel_nav` has data? →
`/drive` has data? → node listed? → read its traceback.

**Foxy's slam_toolbox takes `params_file:=`, not `slam_params_file:=`.** `ros2 launch` silently
ignores unknown arguments and loads the default config. Check with:
```bash
ros2 launch <pkg> <launch_file> --show-args
```

**`base_frame: ego_racecar/base_link`.** slam_toolbox ships with `base_footprint`, which
doesn't exist here. Symptom: `Invalid frame ID "base_footprint" … does not exist`.

**Source every new shell.** `ros2: command not found` or `package not found` almost always means
unsourced, not broken. `setup_container.sh` puts it in `.bashrc`.

**Docker image ≠ container.** `f1tenth_gym_ros:latest` is the image (template).
`f1tenth_sim` is the container (running instance). `docker start` wants the container name.
`docker ps -a` lists containers.

**No trailing `/bin/bash` on `docker run`.** The image's ENTRYPOINT is already bash; the extra
one becomes an argument and fails with `cannot execute binary file`.

**`--rm` deletes the container on exit.** That's how the container vanished once.

**Root-owned files.** Anything created in `/sim_ws/src` from the container is owned by root →
read-only from WSL. Fix: `sudo chown -R $USER:$USER ~/f1tenth_ws/src`.

**Don't run git in the container.** It refuses (`dubious ownership`), and forcing it leaves
root-owned files in `.git/`.

**`git checkout <ref> -- <file>` overwrites uncommitted work with no undo.** That's how the angle
fix got lost once. Commit before experimenting, or use `git stash`.

**Don't crank teleop speed.** `q`/`z` compound 10% per press. At 36 m/s the physics diverged
(odom showed `1e-139` and `9516 rad/s`) and the sim stayed broken until restart. Keep defaults.

**`git stash` only touches uncommitted changes.** Stashing to "test the original" does nothing
if your edits are already committed — check out the base file instead.

**Python only reports errors when the line runs.** Typos in the converter surfaced only on the
first message. `pyflakes <file>.py` catches undefined names before running.

---

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

## Next: rest of M2

1. Check the Ackermann plugins exist in Foxy:
   ```bash
   ros2 pkg list | grep -i "smac\|regulated"
   ```
2. Edit `config/nav2_params.yaml` (commit the stock version first so diffs are readable):
   - every `base_link` → `ego_racecar/base_link`; `odom_topic: /ego_racecar/odom`
   - controller: DWB → **Regulated Pure Pursuit** (DWB assumes the robot can spin in place)
   - planner: NavFn → **Smac Hybrid-A\***, `minimum_turning_radius ≈ 0.75`
     (`wheelbase / tan(max_steer)` = 0.33 / tan(0.419))
   - costmaps: `robot_radius` → `footprint` rectangle, ~0.58 × 0.31 m
   - remove `spin` from recoveries — a car can't turn in place
   - `use_sim_time: false` everywhere, and `use_sim_time:=false` at launch (no `/clock`)
3. Launch **only** the navigation nodes (not `bringup_launch.py`, which starts AMCL + map_server
   and fights slam_toolbox), with the controller's `/cmd_vel` remapped to `/cmd_vel_nav`.
   If Nav2 starts but does nothing, check a node isn't stuck unactivated:
   `ros2 lifecycle get /controller_server`.
4. RViz **2D Goal Pose** → car drives there.

Then: a launch file in `launch/` that starts all of the above with one command — and sets
`use_sim_time` explicitly, so no hidden launch-file defaults.

## Traxxas measurements (chassis is here)

For the URDF and converter parameters. `base_link` = center of the rear axle, on the ground.

- [ ] Wheelbase (front axle to rear axle)
- [ ] Track width
- [ ] Max steering angle (full lock)
- [ ] Overall length and width (Nav2 footprint)
- [ ] LiDAR mount offset from `base_link` (x, y, z) once mounted — level and rigid matter more
      than millimeters
