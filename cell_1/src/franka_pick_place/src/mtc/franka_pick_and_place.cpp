#include "franka_pick_place/franka_pick_and_place.hpp"

FrankaPickAndPlace::FrankaPickAndPlace() : Node("franka_pick_place_node", get_node_options())
{
    RCLCPP_INFO(this->get_logger(), "Franka Pick and Place Node Started");

    // ==========Parameters setup==========
    cube_subscription_topic = this->get_parameter("cube_subscription_topic").as_string();
    cube_position_change_tolerance = this->get_parameter("cube_position_change_tolerance").as_double();
    cube_orientation_change_tolerance = this->get_parameter("cube_orientation_change_tolerance").as_double();
    cube_size = this->get_parameter("cube_size").as_double();
    APPROACH_DISTANCE = this->get_parameter("approach_distance").as_double();
    PRE_GRASP_DISTANCE = this->get_parameter("pre_grasp_distance").as_double();
    LIFT_DISTANCE = this->get_parameter("lift_distance").as_double();
    PLACE_APPROACH_DISTANCE = this->get_parameter("place_approach_distance").as_double();

    // Print Parameters
    RCLCPP_INFO(this->get_logger(), "==========Current Parameters==========");
    RCLCPP_INFO(this->get_logger(), "Cube Subscription Topic: %s", cube_subscription_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "Cube Position Change Tolerance: %.2f", cube_position_change_tolerance);
    RCLCPP_INFO(this->get_logger(), "Cube Orientation Change Tolerance: %.2f", cube_orientation_change_tolerance);
    RCLCPP_INFO(this->get_logger(), "Cube Size: %.2f", cube_size);
    RCLCPP_INFO(this->get_logger(), "Approach distance: %.2f", APPROACH_DISTANCE);
    RCLCPP_INFO(this->get_logger(), "Pre-grasp distance: %.2f", PRE_GRASP_DISTANCE);
    RCLCPP_INFO(this->get_logger(), "Lift distance: %.2f", LIFT_DISTANCE);
    RCLCPP_INFO(this->get_logger(), "Place approach distance: %.2f", PLACE_APPROACH_DISTANCE);

    // ==========Cube monitor setup==========

    // Set QoS Profile
    rclcpp::QoS qos_profile(1);
    qos_profile.reliability(rclcpp::ReliabilityPolicy::BestEffort); // For low latency
    qos_profile.durability(rclcpp::DurabilityPolicy::Volatile); // Don't store old poses

    // Subscribe to cube pose
    cube_subscriber_ = this->create_subscription<vision_msgs::msg::Detection3DArray>(
        cube_subscription_topic,
        qos_profile,
        std::bind(&FrankaPickAndPlace::cubePoseCallback, this, std::placeholders::_1)
    );

    // tf2 setup
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    last_pick_time = this->now() - rclcpp::Duration::from_seconds(10.0);

    initializeBinPoses();
}

rclcpp::NodeOptions FrankaPickAndPlace::get_node_options()
{
    rclcpp::NodeOptions node_options;
    node_options.automatically_declare_parameters_from_overrides(true);
    node_options.parameter_overrides({
        rclcpp::Parameter("use_sim_time", true),
    });

    return node_options;
}



