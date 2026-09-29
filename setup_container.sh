#!/bin/bash
# One-time setup for a fresh f1tenth_sim container.
# Run INSIDE the container after creating it with sim.sh.
# Safe to re-run: apt skips installed packages, and the dotfile lines are only added once.

set -e

apt update
apt install -y \
  ros-foxy-slam-toolbox \
  ros-foxy-tf2-tools \
  ros-foxy-navigation2 \
  ros-foxy-nav2-bringup \
  python3-argcomplete

add_once() {
  grep -qxF "$1" "$2" 2>/dev/null || echo "$1" >> "$2"
}

add_once "source /opt/ros/foxy/setup.bash" ~/.bashrc
add_once "source /sim_ws/install/local_setup.bash" ~/.bashrc
add_once "set -g mouse on" ~/.tmux.conf
add_once "set -g history-limit 50000" ~/.tmux.conf

source /opt/ros/foxy/setup.bash
cd /sim_ws && colcon build --symlink-install

echo
echo "Done. Open a new shell (or: source ~/.bashrc) to pick up the environment."
echo "Files created in /sim_ws/src are owned by root. From WSL, run:"
echo "  sudo chown -R \$USER:\$USER ~/f1tenth_ws/src"
