# Sim environment setup

Windows + WSL2 (Ubuntu 24.04) + Docker. The simulator runs in a container because `f1tenth_gym_ros` targets ROS 2 Foxy.

## First time / after the container is wiped

`sim.sh` creates the container if it's missing. Then, **inside** it:
```bash
bash /sim_ws/src/car_bringup/setup_container.sh
```
Then from **WSL**:
```bash
sudo chown -R $USER:$USER ~/f1tenth_ws/src
```

## Aliases (once, in WSL)

```bash
echo "alias sim='~/f1tenth_ws/src/car_bringup/sim.sh'" >> ~/.bashrc
echo "alias simsh='docker exec -it f1tenth_sim /bin/bash'" >> ~/.bashrc
```

## Clean reset

When state gets confusing (half-dead nodes, duplicates):
```bash
docker restart f1tenth_sim      # from WSL — kills every process in the container
```

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
│       ├── launch/
│       │   ├── sim_bringup_launch.py  one-command bringup
│       │   └── nav2_launch.py         Foxy navigation_launch.py + cmd_vel remap
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
[← README](../README.md)
