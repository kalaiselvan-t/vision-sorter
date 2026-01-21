# Franka FR3 Vision-Based Pick and Place

> Autonomous pick-and-place system using Franka FR3 robotic arm with RGBD vision and MoveIt Task Constructor

![Demo](docs/single_cube_demo.gif)

[![ROS2 Humble](https://img.shields.io/badge/ROS2-Humble-blue)](https://docs.ros.org/en/humble/)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-green)](https://ubuntu.com/)

---

## Key Features

- 🎥 **Vision Pipeline** - RGBD camera-based object detection and 6D pose estimation
- 🎯 **Dynamic Tracking** - Real-time cube position updates and motion triggering
- 🤖 **Motion Planning** - MoveIt Task Constructor for complex pick-and-place sequences
- 🌍 **Simulation** - Full Gazebo environment with custom worlds and models
- 📦 **Flexible Configuration** - Adjustable grasp parameters, table boundaries, and detection thresholds
- 🐳 **Docker Ready** - Containerized development environment for easy setup

---

## Demo

![Demo](docs/multi_cube_demo.gif)

### What it does:
1. RGBD camera detects cubes on the workspace table
2. Vision pipeline estimates pose
3. MoveIt Task Constructor plans collision-free pick-and-place trajectory
4. Franka FR3 robot executes the motion to pick and place cubes in bins
5. System continuously monitors for new cubes and repeats

---

## Table of Contents

- [System Architecture](#-system-architecture)
- [Prerequisites](#-prerequisites)
- [Installation](#-installation)
  - [Docker Installation (Recommended)](#option-1-docker-recommended)
  - [Native Installation](#option-2-native-installation)
- [Usage](#-usage)
- [Configuration](#️-configuration)
- [Project Structure](#-project-structure)
- [Troubleshooting](#-troubleshooting)
- [Roadmap](#️-roadmap)
- [Contributing](#-contributing)
- [License](#-license)
- [Acknowledgments](#-acknowledgments)

---

## System Architecture

```
┌─────────────┐      ┌──────────────────┐      ┌─────────────────┐
│ RGBD Camera │─────▶│  Vision Pipeline │─────▶│ Detected Poses  │
└─────────────┘      │   (OpenCV)       │      │  (vision_msgs)  │
                     └──────────────────┘      └────────┬────────┘
                                                         │
                     ┌──────────────────┐              │
                     │   MTC Planner    │◀─────────────┘
                     │  (pick & place)  │
                     └────────┬─────────┘
                              │
                     ┌────────▼─────────┐
                     │  Franka FR3 Arm  │
                     │   (Gazebo Sim)   │
                     └──────────────────┘
```

### Components:

**Vision Pipeline** (`vision_pipeline_node`)
- Subscribes to RGBD camera topics (`/rgbd_camera/image`, `/rgbd_camera/depth_image`)
- Performs table segmentation and cube detection using OpenCV
- Publishes detected cube poses to `/vision/detected_cube_pose`

**Motion Planner** (`franka_pick_place_node`)
- Subscribes to detected cube poses
- Generates MoveIt Task Constructor pipeline with stages:
  - Current state → Approach → Grasp → Lift → Place → Retreat
- Executes motion and monitors for position changes

**Simulation Environment**
- Gazebo Ignition with custom world (`pick_place_with_bins.sdf`)
- RGBD camera mounted above workspace
- Franka FR3 robot with gripper
- Spawnable cubes and bins

---

## Prerequisites

- **OS:** Ubuntu 22.04 LTS
- **ROS 2:** Humble Hawksbill
- **Docker:** 20.10+ (for containerized setup)
- **Hardware:**
  - 8GB+ RAM recommended

### Required ROS 2 Packages (installed automatically):
- MoveIt 2
- ros2_control
- Gazebo Ignition (Fortress)
- OpenCV
- yaml-cpp

---

## Installation

### Option 1: Docker (Recommended)

Docker provides an isolated, reproducible environment and is the easiest way to get started.

#### 1. Clone the repository
```bash
git clone --recurse-submodules https://github.com/kalaiselvan-t/vision-sorter.git
cd pick-and-place
```

#### 2. Build Docker image
```bash
cd src
docker compose build
```

#### 3. Start container
```bash
docker compose up -d franka_ros2
docker exec -it franka_ros2 bash
```

#### 4. Build the workspace (inside container)
```bash
cd /ros2_ws
source /opt/ros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
```

---

### Option 2: Native Installation

For users who prefer a local installation without Docker.

#### 1. Install ROS 2 Humble
Follow the [official ROS 2 Humble installation guide](https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html).

Install Desktop version for full GUI tools:
```bash
sudo apt update && sudo apt install ros-humble-desktop
```

#### 2. Install dependencies
```bash
sudo apt install -y \
    ros-humble-moveit \
    ros-humble-ros2-control \
    ros-humble-ros2-controllers \
    ros-humble-ros-gz-sim \
    ros-humble-ros-gz-bridge \
    python3-colcon-common-extensions \
    libyaml-cpp-dev \
    libopencv-dev
```

#### 3. Clone and build
```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone --recurse-submodules https://github.com/kalaiselvan-t/vision-sorter.git .

# Import additional dependencies
vcs import < franka.repos

# Build
cd ~/ros2_ws
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

---

## Usage

### Quick Start

**Terminal 1: Launch Gazebo simulation with robot**
```bash
source install/setup.bash
ros2 launch franka_pick_place gazebo_fr3.launch.py
```

This launches:
- Gazebo with custom world (table, bins, camera)
- Franka FR3 robot with controllers
- MoveIt move_group
- RViz for visualization
- MTC pick-and-place node

**Terminal 2: Start vision pipeline**
```bash
source install/setup.bash
ros2 run franka_pick_place vision_pipeline_node
```

**Terminal 3: Spawn cubes**
```bash
python3 spawn_multiple_cubes.py 3
```

The robot will automatically detect and pick the cubes!

---

### Advanced Usage

#### Adjust verbosity
```bash
# Launch with DEBUG logging
ros2 launch franka_pick_place gazebo_fr3.launch.py log_level:=DEBUG
```

#### Monitor topics
```bash
# View detected cube poses
ros2 topic echo /vision/detected_cube_pose

# View joint states
ros2 topic echo /joint_states
```

---

## Configuration

Edit `franka_pick_place/config/params.yaml` to customize behavior:

| Parameter | Description | Default | Unit |
|-----------|-------------|---------|------|
| `cube_size` | Side length of cube | 0.03 | meters |
| `approach_distance` | Distance to stop before grasp | 0.15 | meters |
| `pre_grasp_distance` | Distance to open gripper | 0.06 | meters |
| `lift_distance` | Height to lift after grasp | 0.08 | meters |
| `place_approach_distance` | Distance above bin for placement | 0.30 | meters |
| `cube_position_change_tolerance` | Minimum movement to trigger re-plan | 0.05 | meters |
| `cube_orientation_change_tolerance` | Minimum rotation to trigger re-plan | 1.0 | degrees |

### Planning Scene Configuration

Edit `franka_pick_place/config/planning_scene.yaml` to adjust:
- Table dimensions and position
- Collision objects (bins, walls)
- Workspace boundaries

---

## Project Structure

```
franka_pick_place/
├── config/
│   ├── params.yaml              # Pick-place parameters
│   ├── planning_scene.yaml      # Collision objects & table
│   └── moveit_controllers.yaml  # MoveIt controller config
├── include/
│   ├── franka_pick_and_place.hpp
│   └── vision.hpp
├── launch/
│   └── gazebo_fr3.launch.py     # Main launch file
├── models/
│   ├── cube/                    # Gazebo cube model
│   ├── bins/                    # Target bins
│   └── camera/                  # RGBD camera
├── scripts/
│   ├── spawn_cube.py            # Spawn single cube
│   └── spawn_multiple_cubes.py  # Spawn multiple cubes
├── src/
│   ├── main.cpp                 # Pick-place node entry
│   ├── vision_main.cpp          # Vision node entry
│   ├── mtc/
│   │   ├── franka_pick_and_place.cpp  # Main MTC logic
│   │   ├── cube_tracking.cpp          # Pose tracking
│   │   ├── mtc.cpp                    # MTC pipeline
│   │   └── planning_scene_loader.cpp  # Scene setup
│   ├── vision/
│   │   └── vision.cpp           # Detection & pose estimation
│   └── utilities/
│       └── test_utilities.cpp   # Helper functions
└── worlds/
    └── pick_place_with_bins.sdf # Gazebo world
```

---

## Troubleshooting

### Robot doesn't move

**Check if MoveIt service is running:**
```bash
ros2 service list | grep plan_kinematic_path
```

**Verify controllers are active:**
```bash
ros2 control list_controllers
```

Expected output:
```
arm_controller[active]
gripper_controller[active]
joint_state_broadcaster[active]
```

### Vision not detecting cubes

**Check camera topics:**
```bash
ros2 topic list | grep rgbd_camera
ros2 topic hz /rgbd_camera/image
```

**Verify cube is on table:**
- Cubes must be within table boundaries (see `planning_scene.yaml`)
- Check RViz to see camera view

**Adjust detection thresholds:**
Edit vision pipeline source if needed (src/vision/vision.cpp)

### Build errors

**Missing dependencies:**
```bash
rosdep install --from-paths src --ignore-src -r -y
```

**Clean build:**
```bash
rm -rf build/ install/ log/
colcon build --symlink-install
```

### Gazebo crashes or freezes

**Increase shared memory (Docker):**
```bash
# In docker-compose.yml, add:
shm_size: '2gb'
```

**Check GPU drivers (if using):**
```bash
nvidia-smi
```

---

## Roadmap

- [ ] Add unit tests for vision and MTC components
- [ ] Support for different object shapes (cylinders, boxes)
- [ ] Multi-object sorting by color/size
- [ ] Real Franka FR3 hardware integration
- [ ] Performance benchmarks and optimization
- [ ] VLA (Vision-Language-Action) model integration
- [ ] ROS 2 parameters for runtime configuration
- [ ] CI/CD pipeline with automated testing

---

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### How to contribute:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'feat: add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE) file for details.

Copyright 2024-2025 Kalaiselvan Thangaraj

---

## Acknowledgments

This project builds upon excellent open-source work:

- **[franka_ros2](https://github.com/frankaemika/franka_ros2)** - Franka Robotics ROS 2 integration
- **[MoveIt 2](https://moveit.ros.org/)** - Motion planning framework
- **[MoveIt Task Constructor](https://github.com/moveit/moveit_task_constructor)** - Task-level motion planning
- **Franka Robotics** - For creating amazing research robots

Special thanks to the ROS and robotics open-source community!

---

<div align="center">

</div>
