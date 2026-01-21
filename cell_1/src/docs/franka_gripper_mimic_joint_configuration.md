# FR3 Gripper Mimic Joint Configuration

## Overview

The Franka FR3 gripper uses a parallel gripper design with two finger joints. The right finger (`finger_joint2`) mimics the left finger (`finger_joint1`) to create symmetric grasping motion. This document explains the configuration required for proper mimic joint behavior in Gazebo simulation using gz_ros2_control.

## Problem Statement

In Gazebo simulation, if only `finger_joint1` is configured in ros2_control, `finger_joint2` will not move, resulting in only one gripper finger being controlled. The mimic relationship defined in the URDF is not automatically enforced by the physics engine in all configurations.

## Solution

The mimic joint must be explicitly configured in the ros2_control system with both command interfaces and mimic parameters. This allows gz_ros2_control to emulate the mimic behavior by sending matching commands to both joints.

## Implementation

### URDF Level Configuration

Location: `franka_description/end_effectors/common/franka_hand.xacro:130`

The URDF defines the mimic relationship between the finger joints:

```xml
<joint name="${ee_prefix}finger_joint2" type="prismatic">
  <parent link="${ee_prefix}hand" />
  <child link="${ee_prefix}rightfinger" />
  <origin xyz="0 0 0.0584" rpy="0 0 ${pi}" />
  <axis xyz="0 1 0" />
  <limit effort="100" lower="0.0" upper="0.04" velocity="0.2" />
  <mimic joint="${ee_prefix}finger_joint1" />
  <dynamics damping="0.3" />
</joint>
```

Key specifications:
- Joint type: Prismatic (linear motion along Y-axis)
- Position range: 0.0 to 0.04 meters (40mm maximum opening)
- Velocity limit: 0.2 m/s
- Mimic relationship: Mirrors `finger_joint1` movements

### ROS2 Control Configuration

Location: `franka_description/robots/common/franka_arm.ros2_control.xacro:82-95`

For Gazebo simulation, `finger_joint2` must be configured with command interfaces and mimic parameters:

```xml
<xacro:if value="${gazebo and hand}">
  <xacro:configure_joint joint_name="${arm_id}_finger_joint1" initial_position="0.0" />
  <joint name="${arm_id}_finger_joint2">
    <command_interface name="position"/>
    <command_interface name="velocity"/>
    <state_interface name="position">
      <param name="initial_value">0.0</param>
    </state_interface>
    <state_interface name="velocity">
      <param name="initial_value">0.0</param>
    </state_interface>
    <state_interface name="effort"/>
    <param name="mimic">${arm_id}_finger_joint1</param>
    <param name="multiplier">1.0</param>
  </joint>
</xacro:if>
```

Critical configuration elements:
- Command interfaces: Both position and velocity interfaces are required
- State interfaces: Position, velocity, and effort for state feedback
- Mimic parameter: References `finger_joint1` as the leader joint
- Multiplier: Set to 1.0 for 1:1 motion mirroring

## Important Notes

### Command Interfaces Requirement

Unlike physical hardware where mimic joints are mechanically coupled, Gazebo simulation requires command interfaces on mimic joints. The gz_ros2_control plugin emulates mimic behavior by sending matching commands to both joints. Without command interfaces, the mimic joint will not receive position updates and will remain stationary.

### Physics Engine Considerations

The default DART physics engine in Gazebo does not support mimic joint constraints. However, this limitation does not affect functionality because gz_ros2_control handles the mimic behavior at the control level rather than relying on physics engine constraints.

Alternative physics engines like Bullet-Featherstone support mimic constraints natively, but switching physics engines is not required for proper gripper operation.

### Controller Configuration

Location: `franka_gazebo/franka_gazebo_bringup/config/franka_gazebo_controllers.yaml:110-114`

The gripper controller should only control `finger_joint1`:

```yaml
gripper_controller:
  ros__parameters:
    joint: fr3_finger_joint1
    command_interface: position
    use_sim_time: true
```

The controller sends commands only to `finger_joint1`. The gz_ros2_control system automatically propagates these commands to `finger_joint2` based on the mimic configuration.

## Behavior

With this configuration:
1. The gripper controller sends position commands to `finger_joint1`
2. gz_ros2_control receives the command and applies it to `finger_joint1`
3. gz_ros2_control automatically calculates the mimic joint position using the multiplier
4. The calculated position is applied to `finger_joint2`
5. Both fingers move symmetrically, creating a parallel grasp pattern

## Troubleshooting

### Only one finger moves
- Verify `finger_joint2` is configured in the ros2_control section
- Ensure both command interfaces (position and velocity) are present
- Check that mimic and multiplier parameters are set correctly

### Segmentation fault during simulation
- Confirm command interfaces are present on the mimic joint
- Verify the mimic parameter references the correct leader joint name
- Check that initial position values are specified for state interfaces

### Physics engine errors
- These warnings can be safely ignored if using the default physics engine
- The mimic behavior works through gz_ros2_control regardless of physics engine support
- Do not attempt to switch to Bullet-Featherstone unless the plugin is installed

## References

- gz_ros2_control documentation: https://control.ros.org/humble/doc/gz_ros2_control/doc/index.html
- Mimic joint issue discussion: https://github.com/ros-controls/gz_ros2_control/issues/340
