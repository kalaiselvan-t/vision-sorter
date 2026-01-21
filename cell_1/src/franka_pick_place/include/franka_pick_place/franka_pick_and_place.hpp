#ifndef FRANKA_PICK_PLACE_FRANKA_PICK_AND_PLACE_HPP
#define FRANKA_PICK_PLACE_FRANKA_PICK_AND_PLACE_HPP

#include <string>
#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <shape_msgs/msg/solid_primitive.hpp>
#include <vision_msgs/msg/detection3_d_array.hpp>

#include <moveit_msgs/msg/planning_scene.hpp>
#include <moveit_msgs/msg/collision_object.hpp>
#include <moveit/planning_scene_interface/planning_scene_interface.h>

#include <moveit/task_constructor/task.h>
#include <moveit/task_constructor/stages.h>
#include <moveit/task_constructor/solvers.h>
#include <moveit/task_constructor/properties.h>

#include <tf2/utils.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

class FrankaPickAndPlace : public rclcpp::Node
{
public:
    FrankaPickAndPlace();

    static rclcpp::NodeOptions get_node_options();
    void initializeMTC();
    void testFunctionalities();

    ~FrankaPickAndPlace() 
    {
      shutdown_requested_ = true;
      if (mtc_thread_.joinable()) 
      {
          mtc_thread_.join();
      }
    }

private:
    // Atomic, mutex
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<bool> new_cube_available_{false};
    std::atomic<bool> pipeline_executing_{false};
    std::mutex cube_mutex_;

    // threads
    std::thread mtc_thread_;

    // Class variables
    bool cube_in_scene {false};

    std::string cube_subscription_topic;
    std::string planning_scene_topic;

    double cube_size;
    double cube_position_change_tolerance;
    double cube_orientation_change_tolerance;
    double APPROACH_DISTANCE;
    double PRE_GRASP_DISTANCE;
    double LIFT_DISTANCE;
    double PLACE_APPROACH_DISTANCE;

    geometry_msgs::msg::Pose target_cube_pose;
    std::string target_cube_color;
    rclcpp::Time last_pick_time;

    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    rclcpp::Subscription<vision_msgs::msg::Detection3DArray>::SharedPtr cube_subscriber_;
    moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
    std::unique_ptr<moveit::task_constructor::Task> mtc_task_;
    std::unique_ptr<moveit::task_constructor::Task> test_task_;

    std::map<std::string, geometry_msgs::msg::Pose> bin_poses;

    // Class structs
    struct Cube
    {
        geometry_msgs::msg::Pose pose;
        rclcpp::Time last_update_time;
    } cube;

    // Class members
    void cubePoseCallback(const vision_msgs::msg::Detection3DArray::SharedPtr msg);
    bool cubePoseChanged(const geometry_msgs::msg::PoseStamped::SharedPtr msg);
    void initializeBinPoses();
    geometry_msgs::msg::Pose getBinPoseForColor(const std::string& color);
    bool updatePlanningScene(const geometry_msgs::msg::Pose msg);

    void addCurrentStateStage();
    void addApproachStage();
    void setApproachStage(const geometry_msgs::msg::Pose& pose);
    void addGripperOpenStage();
    void planAndExecute();
    void addPreGraspStage();
    void addAllowCollisionStage();
    void addGripperCloseStage();
    void addAttachObjectStage();
    void addLiftStage();
    void addPlaceApproachStage();
    void setPlaceApproachStage();
    void addPlaceDescentStage();
    void addDetachObjectStage();
    void addGripperReleaseStage();
    void addPostPlaceRetreatStage();
    void addDisallowCollisionStage();
    void returnHomeStage();

    bool isOnTable(const geometry_msgs::msg::Pose& pose);
    bool shouldPickCube(const geometry_msgs::msg::Pose& pose);
    void loadPlanningSceneFromConfig();

    geometry_msgs::msg::Pose getCurrentEndEffectorPose();
    void addRotateGripperStage(double yaw_deg=44.50);
    void testGripperRotations();
};

#endif  // FRANKA_PICK_PLACE_FRANKA_PICK_AND_PLACE_HPP
