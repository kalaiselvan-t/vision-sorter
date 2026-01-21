#!/bin/bash

cd /ros2_ws

# Clone Franka dependencies into the workspace
vcs import src < src/franka.repos --recursive --skip-existing

# apt update
sudo apt update

# rosdep
rosdep update
rosdep install --from-paths  src --ignore-src -r -y

# Setup workspace environment (VNC, etc)
source /usr/local/bin/setup_workspace.sh

exec "$@"