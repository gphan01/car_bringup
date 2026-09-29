#!/bin/bash
# Start the F1TENTH sim container, creating it if it doesn't exist.
# Run from WSL. Docker Desktop must be running first.
#
# RViz renders through WSLg as a native Windows window (not noVNC).
# The whole src/ directory is mounted, so both f1tenth_gym_ros and
# car_bringup are visible to colcon inside the container.

NAME=f1tenth_sim
IMAGE=f1tenth_gym_ros:latest

if ! docker info >/dev/null 2>&1; then
  echo "Docker isn't reachable. Start Docker Desktop and check"
  echo "Settings > Resources > WSL Integration is on for this distro."
  exit 1
fi

if docker ps -a --format '{{.Names}}' | grep -q "^${NAME}\$"; then
  docker start -ai "$NAME"
else
  echo "Creating $NAME. After it starts, run inside it:"
  echo "  bash /sim_ws/src/car_bringup/setup_container.sh"
  # No trailing /bin/bash: the image's ENTRYPOINT is already bash.
  # No --rm: installs and dotfiles survive between sessions.
  docker run -it \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v /mnt/wslg:/mnt/wslg \
    -e DISPLAY="$DISPLAY" \
    -e WAYLAND_DISPLAY="$WAYLAND_DISPLAY" \
    -e XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    -v ~/f1tenth_ws/src:/sim_ws/src \
    --name "$NAME" \
    "$IMAGE"
fi
