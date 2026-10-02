# Nav2 configuration

Pipeline: `bt_navigator` (behavior tree) → `planner_server` (route) → `controller_server`
(driving) → `/cmd_vel_nav` → converter → `/drive`. Costmaps are the world model: global from
SLAM's `/map`, local from live `/scan`. Every Nav2 node is a **lifecycle node** — starts idle,
must be configured + activated by `lifecycle_manager`.

Changes from Foxy's stock `nav2_params.yaml` (`git diff` against the baseline commit shows all):

| What | Stock | Ours | Why |
|---|---|---|---|
| Frames | `base_link` | `ego_racecar/base_link` | sim namespaces frames |
| `bt_navigator.odom_topic` | `/odom` | `/ego_racecar/odom` | sim's odom topic |
| `use_sim_time` | true | **false** | sim publishes no `/clock` |
| Controller | DWB | **Regulated Pure Pursuit** | DWB assumes spin-in-place |
| `use_rotate_to_heading` | — | **false** | a car can't rotate in place |
| Planner | NavFn | **`smac_planner/SmacPlanner`**, `DUBIN` | plans arcs a car can drive (Foxy name has no `nav2_` prefix) |
| `minimum_turning_radius` | — | 0.8 m | wheelbase / tan(max_steer) = 0.33 / tan(0.419) ≈ 0.74 + margin |
| Footprint | `robot_radius: 0.22` | rectangle ±0.30 × ±0.17 m | car is a rectangle, not a circle |
| `yaw_goal_tolerance` | 0.25 | **3.14** | see below |

`nav2_launch.py` is a copy of Foxy's `navigation_launch.py` with one change on the
`controller_server` node: `remappings=remappings + [('cmd_vel', 'cmd_vel_nav')]`. Without it
Nav2 publishes to `/cmd_vel`, which the sim bridge reads through its crude path.

`spin` recovery left in on purpose: Foxy's default behavior tree calls it, and removing the
plugin can break tree loading. On a car it commands zero speed → harmless no-op.

## Known issues

**Turnaround loops stall.** Goal behind the car in a narrow corridor → Smac correctly plans
forward to an intersection, loops, and comes back (a car can't U-turn in less than ~1.8 m).
RPP then stalls partway through the loop. Untested candidates, change **one at a time**:
- `max_allowed_time_to_collision: 0.5` — RPP projects 1 s ahead and stops if it touches lethal
  cost; tight loops near walls trip it. Log: `Detected collision ahead`.
- `regulated_linear_scaling_min_radius: 0.6`, `regulated_linear_scaling_min_speed: 0.4` —
  curves tighter than 0.9 m crawl, then the progress checker gives up. Log: `Failed to make progress`.
- `minimum_turning_radius: 0.9` — loop is at the car's exact steering limit; margin lets it recover.

Find which with:
```bash
grep -v "dropping" /tmp/run.log | grep -i "collision\|progress\|running\|abort\|fail" | tail -20
```
(`Activating spin/back_up/wait` at startup is just plugins loading. A recovery actually
firing looks like `Running spin`.)

**No reversing / three-point turns.** Foxy's RPP can't drive backward (`allow_reversing` came
in later Nav2). Jazzy on the real car has it — pair with `REEDS_SHEPP` in Smac.

**`yaw_goal_tolerance`** is how close the final heading (the dragged arrow) must be, in
radians. With `stateful: True`, once position is reached only heading is checked — a car that
arrives at the wrong angle circles forever trying to fix it. 3.14 = heading ignored.

**A wall hit freezes the sim.** F1TENTH gym marks the episode `done` on collision and stops
stepping physics. Ctrl+C the bringup and relaunch.

---
[← README](../README.md)
