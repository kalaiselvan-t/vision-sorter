#include "franka_pick_place/franka_pick_and_place.hpp"
#include <yaml-cpp/yaml.h>
#include <ament_index_cpp/get_package_share_directory.hpp>

void FrankaPickAndPlace::loadPlanningSceneFromConfig()
{
    RCLCPP_INFO(this->get_logger(), "Loading planning scene from config file...");

    try {
        // Get path to config file
        std::string package_share_directory = ament_index_cpp::get_package_share_directory("franka_pick_place");
        std::string config_file = package_share_directory + "/config/planning_scene.yaml";

        RCLCPP_INFO(this->get_logger(), "Loading config from: %s", config_file.c_str());

        // Load YAML file
        YAML::Node config = YAML::LoadFile(config_file);

        if (!config["planning_scene"] || !config["planning_scene"]["world"] ||
            !config["planning_scene"]["world"]["collision_objects"]) {
            RCLCPP_ERROR(this->get_logger(), "Invalid config file structure");
            return;
        }

        YAML::Node collision_objects = config["planning_scene"]["world"]["collision_objects"];

        std::vector<moveit_msgs::msg::CollisionObject> objects;

        // Parse each collision object
        for (const auto& obj_node : collision_objects) {
            moveit_msgs::msg::CollisionObject collision_object;

            // Get ID
            collision_object.id = obj_node["id"].as<std::string>();
            collision_object.header.frame_id = "world";

            // Get type and dimensions
            std::string type = obj_node["type"].as<std::string>();
            if (type == "box") {
                shape_msgs::msg::SolidPrimitive primitive;
                primitive.type = primitive.BOX;

                auto dims = obj_node["dimensions"].as<std::vector<double>>();
                primitive.dimensions.resize(dims.size());
                for (size_t i = 0; i < dims.size(); ++i) {
                    primitive.dimensions[i] = dims[i];
                }

                collision_object.primitives.push_back(primitive);
            }

            // Get pose
            geometry_msgs::msg::Pose pose;
            pose.position.x = obj_node["pose"]["position"]["x"].as<double>();
            pose.position.y = obj_node["pose"]["position"]["y"].as<double>();
            pose.position.z = obj_node["pose"]["position"]["z"].as<double>();

            pose.orientation.x = obj_node["pose"]["orientation"]["x"].as<double>();
            pose.orientation.y = obj_node["pose"]["orientation"]["y"].as<double>();
            pose.orientation.z = obj_node["pose"]["orientation"]["z"].as<double>();
            pose.orientation.w = obj_node["pose"]["orientation"]["w"].as<double>();

            collision_object.primitive_poses.push_back(pose);
            collision_object.operation = collision_object.ADD;

            objects.push_back(collision_object);

            RCLCPP_INFO(this->get_logger(), "  Loaded: %s", collision_object.id.c_str());
        }

        // Apply all collision objects to planning scene
        planning_scene_interface.applyCollisionObjects(objects);

        RCLCPP_INFO(this->get_logger(), "✓ Successfully added %zu collision objects to planning scene", objects.size());

    } catch (const YAML::Exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Error loading planning scene config: %s", e.what());
    } catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Error: %s", e.what());
    }
}
