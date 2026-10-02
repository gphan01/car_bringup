# Troubleshooting

Symptom → cause → fix. Search this file for the error text you're seeing.

## Method — read first

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

**When something that worked stops working, list what changed since.** The M2 regressions all
traced to the re-clone, the container rebuild, the mount change, and the config move —
not to new bugs.

## SLAM / time / frames

**This sim publishes no `/clock` → `use_sim_time:=false` on every launch.** With it `true`,
nodes read time from `/clock`; nobody publishes it, so their clock sits at 0 while scans are
stamped with wall time. slam_toolbox then drops every scan:
`Message Filter dropping message: frame 'ego_racecar/laser' … for reason 'Unknown'`.
Diagnosis: `view_frames` showed slam's `map → odom` stamped `0.2` vs sim transforms at
~`1.79e9`; `ros2 topic info /clock` showed 0 publishers. Pass it as a **launch argument** —
Foxy's launch files apply `{'use_sim_time': …}` after the YAML, so a YAML value gets
overridden. Same for Nav2. (`/clock` appearing in `ros2 topic list` means nothing — the list
includes topics that only have subscribers.)

**A few "dropping message" lines are normal.** A scan that arrives milliseconds before its
transform gets dropped and the next one is used. Only a continuous flood with no map growth
is a problem. Judge by whether the map builds, not by the log.

**RViz Fixed Frame must be a frame that exists.** Symptoms of a wrong one look exactly like a
frozen simulator: car won't move, no lasers, RobotModel red, white box instead of the car.
The car was driving the whole time. Check Global Status first. Use `odom` if slam_toolbox isn't
running.

**`base_frame: ego_racecar/base_link`.** slam_toolbox ships with `base_footprint`, which
doesn't exist here. Symptom: `Invalid frame ID "base_footprint" … does not exist`.

**Foxy's slam_toolbox takes `params_file:=`, not `slam_params_file:=`.** `ros2 launch` silently
ignores unknown arguments and loads the default config. Check with:
```bash
ros2 launch <pkg> <launch_file> --show-args
```

## Processes and launch

**Leftover processes.** Closing a pane/tab doesn't always kill what was running in it.
Symptoms: duplicate node names (`WARNING: … nodes … share an exact name`), half a Nav2 stack
alive. Find them: `ps aux | grep -E "nav2|controller|planner|bt_nav|slam|converter"` —
the `pts/N` column is which terminal started it. Use `sim_bringup_launch.py` so one Ctrl+C
kills everything, or `docker restart f1tenth_sim` to wipe the slate.

**`ros2 node list` can lie.** The CLI caches the graph in a background daemon, which goes
stale after many restarts. `ros2 daemon stop` then `ros2 node list --no-daemon`.

**Startup order: sim → slam_toolbox → Nav2.** Nav2's global costmap needs `map → odom`; started
first it hangs at configure or fails goals with `send_goal failed` / `"map" does not exist`.
The bringup launch file enforces this with an 8 s delay.

**Source every new shell.** `ros2: command not found` or `package not found` almost always means
unsourced, not broken. `setup_container.sh` puts it in `.bashrc`.

**Don't crank teleop speed.** `q`/`z` compound 10% per press. At 36 m/s the physics diverged
(odom showed `1e-139` and `9516 rad/s`) and the sim stayed broken until restart. Keep defaults.

## Docker / WSL / display

**RViz process runs but no window appears → `wsl --shutdown` from PowerShell, then restart.**
The container mounts WSLg's display socket at creation; after sleep/reboot WSLg makes a new
one and the container points at a dead socket. Test WSLg alone with `xeyes` in plain WSL.

**Docker image ≠ container.** `f1tenth_gym_ros:latest` is the image (template).
`f1tenth_sim` is the container (running instance). `docker start` wants the container name.
`docker ps -a` lists containers.

**No trailing `/bin/bash` on `docker run`.** The image's ENTRYPOINT is already bash; the extra
one becomes an argument and fails with `cannot execute binary file`.

**`--rm` deletes the container on exit.** That's how the container vanished once.

**Root-owned files.** Anything created in `/sim_ws/src` from the container is owned by root →
read-only from WSL. Fix: `sudo chown -R $USER:$USER ~/f1tenth_ws/src`.

**tmux copy mode freezes the pane.** A yellow `[n/N]` top-right means you're scrolled back
and seeing old output. Press `q`.

## Git

**Don't run git in the container.** It refuses (`dubious ownership`), and forcing it leaves
root-owned files in `.git/`.

**`git checkout <ref> -- <file>` overwrites uncommitted work with no undo.** That's how the angle
fix got lost once. Commit before experimenting, or use `git stash`.

**`git stash` only touches uncommitted changes.** Stashing to "test the original" does nothing
if your edits are already committed — check out the base file instead.

## Python

**Python only reports errors when the line runs.** Typos in the converter surfaced only on the
first message. `pyflakes <file>.py` catches undefined names before running.

---
[← README](../README.md)
