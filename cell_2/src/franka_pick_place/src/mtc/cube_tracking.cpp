#include "franka_pick_place/franka_pick_and_place.hpp"

void FrankaPickAndPlace::initializeBinPoses()
{
    geometry_msgs::msg::Pose pose;

    // Green bin - Left side front (from SDF line 75-76)
    pose.position.x = 0.45;
    pose.position.y = 0.45;
    pose.position.z = 0.15;  // Above bin height
    pose.orientation.w = 1.0;
    bin_poses["green"] = pose;

    // Black bin - Left side back (line 82-83)
    pose.position.x = 0.65;
    pose.position.y = 0.45;
    pose.position.z = 0.15;
    bin_poses["black"] = pose;

    // Red bin - Right side front (line 89-90)
    pose.position.x = 0.45;
    pose.position.y = -0.45;
    pose.position.z = 0.15;
    bin_poses["red_low"] = pose;

    // Blue bin - Right side back (line 96-97)
    pose.position.x = 0.65;
    pose.position.y = -0.45;
    pose.position.z = 0.15;
    bin_poses["blue"] = pose;

    RCLCPP_INFO(this->get_logger(), "Bin poses initialized for sorting");
}

geometry_msgs::msg::Pose FrankaPickAndPlace::getBinPoseForColor(const std::string& color)
{
    auto it = bin_poses.find(color);
    if (it != bin_poses.end()) {
        RCLCPP_INFO(this->get_logger(), "Placing %s cube in %s bin",
            color.c_str(), color.c_str());
        return it->second;
    }

    // Default/fallback bin if color not found
    RCLCPP_WARN(this->get_logger(), "Unknown color '%s', using default bin",
        color.c_str());
    return bin_poses["black"];
}

// void FrankaPickAndPlace::cubePoseCallback(const vision_msgs::msg::Detection3D::SharedPtr msg)
// {
//     auto pose = std::make_shared<geometry_msgs::msg::PoseStamped>();
//     pose->header = msg->header;
//     pose->pose = msg->results[0].pose.pose;
//     if (cubePoseChanged(pose))
//     {
//         if (pipeline_executing_)
//         {
//             return;
//         }

//         if (!shouldPickCube(pose->pose))
//         {
//             return;
//         }

//         {
//             std::lock_guard<std::mutex> lock(cube_mutex_);
//             target_cube_pose = pose->pose;
//             target_cube_color = msg->results[0].hypothesis.class_id;
//         }

//         cube.pose = pose->pose;
//         new_cube_available_ = true;
//         updatePlanningScene(pose);
//         last_pick_time = this->now();
//     }
// }

void FrankaPickAndPlace::cubePoseCallback(const vision_msgs::msg::Detection3DArray::SharedPtr msg)
{
    if (msg->detections.empty() || pipeline_executing_)
    {
        return;
    }

    auto& first_detection = msg->detections[0];
    auto pose = first_detection.results[0].pose.pose;
    std::string color = first_detection.results[0].hypothesis.class_id;

    if (!isOnTable(pose)) 
    {
          return;
    }

    {
        std::lock_guard<std::mutex> lock(cube_mutex_);
        target_cube_pose = pose;
        target_cube_color = color;
    }

    updatePlanningScene(pose);
    new_cube_available_ = true;
}

bool FrankaPickAndPlace::cubePoseChanged(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
{
    // Detect position change
    double dx = msg->pose.position.x - cube.pose.position.x;
    double dy = msg->pose.position.y - cube.pose.position.y;
    double dz = msg->pose.position.z - cube.pose.position.z;

    double distance_moved = std::sqrt(dx*dx + dy*dy + dz*dz);

    // Detect orientation change
    tf2::Quaternion q_old, q_new, q_diff;
    tf2::fromMsg(cube.pose.orientation, q_old);
    tf2::fromMsg(msg->pose.orientation, q_new);

    q_diff = q_new * q_old.inverse();

    double angle = q_diff.getAngle();
    double angle_degrees = angle * 180 / M_PI;

    bool position_changed = (distance_moved > cube_position_change_tolerance);
    bool orientation_changed = (angle_degrees > cube_orientation_change_tolerance);

    if (position_changed || orientation_changed)
    {
        // RCLCPP_INFO(this->get_logger(), "Cube Moved: %.2f, Orientation Changed: %.2f deg", distance_moved, angle_degrees);
        return true;
    }

    return false;
}

bool FrankaPickAndPlace::updatePlanningScene(const geometry_msgs::msg::Pose msg)
{
    RCLCPP_INFO(this->get_logger(), "Updating the Planning Scene...");

    // Create collision object
    moveit_msgs::msg::CollisionObject collision_object;
    collision_object.id = "target_cube";

    // Define the object size
    shape_msgs::msg::SolidPrimitive primitive;
    primitive.type = primitive.BOX;
    primitive.dimensions = {cube_size, cube_size, cube_size};

    // Set the pose
    geometry_msgs::msg::Pose object_pose;
    object_pose.position = msg.position;
    object_pose.orientation = msg.orientation;

    // Add to collision object
    collision_object.header.frame_id = "world";
    collision_object.primitives.push_back(primitive);
    collision_object.primitive_poses.push_back(object_pose);
    collision_object.operation = collision_object.ADD;

    // Updating planning scene via planning scene interface
    bool success = planning_scene_interface.applyCollisionObject(collision_object);
    cube_in_scene = success;
    if (success)
    {
        RCLCPP_INFO(this->get_logger(), "Scene updated via planning scene interface");
    }
    else
    {
        RCLCPP_INFO(this->get_logger(), "Scene update failed");
    }

    return true;
}

bool FrankaPickAndPlace::isOnTable(const geometry_msgs::msg::Pose& pose)
{
    double table_x_min = 0.35;
    double table_x_max = 0.85;
    double table_y_min = -0.25;
    double table_y_max = 0.25;
    double table_z_min = 0.05;
    double table_z_max = 0.15;

    bool on_table = (pose.position.x >= table_x_min && pose.position.x <= table_x_max &&
                    pose.position.y >= table_y_min && pose.position.y <= table_y_max &&
                    pose.position.z >= table_z_min && pose.position.z <= table_z_max
                );

    if (!on_table)
    {
        RCLCPP_DEBUG(this->get_logger(), "Cube not on table - position: x=%.2f, y=%.2f, z=%.2f",
                    pose.position.x, pose.position.y, pose.position.z);
    }

    return on_table;
}

bool FrankaPickAndPlace::shouldPickCube(const geometry_msgs::msg::Pose& pose)
{
    if (!isOnTable(pose))
    {
        // RCLCPP_INFO(this->get_logger(), "Cube not on table - ignoring");
        return false;
    }

    // Check 2: Has enough time passed since last pick?
    auto now = this->now();
    double time_since_last_pick = (now - last_pick_time).seconds();
    if (time_since_last_pick < 5.0)  // 5 second cooldown
    {
        // RCLCPP_INFO(this->get_logger(), "Cooldown active (%.1fs elapsed, need 5.0s) - ignoring",
                    // time_since_last_pick);
        return false;
    }

    RCLCPP_INFO(this->get_logger(), "Cube is pickable - on table and cooldown satisfied");
    return true;
}