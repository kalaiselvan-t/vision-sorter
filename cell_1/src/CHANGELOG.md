# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-11-05

### Added
- Initial release of Franka FR3 pick and place system
- Vision pipeline for cube detection using RGBD camera
- MoveIt Task Constructor integration for motion planning
- Gazebo simulation environment with custom world files
- Pick and place automation with dynamic object tracking
- Docker development environment with multi-container setup
- Multiple cube spawning and detection support
- Planning scene configuration for collision avoidance

### Features
- Vision-based object pose estimation
- Real-time cube tracking and pose updates
- Integrated motion planning and execution
- Configurable approach, grasp, and place parameters
- RViz visualization support
- Comprehensive launch file for Gazebo simulation

### Dependencies
- ROS 2 Humble
- MoveIt 2
- Franka ROS 2 (based on v2.0.2)
- OpenCV for computer vision
- Ignition Gazebo 6

## [Unreleased]

### Planned
- Improve grasp performance
- Add unit tests
- Improve error handling
- Add support for different object shapes
- Implement multi-object sorting
- Add VLA (Vision-Language-Action) model integration
