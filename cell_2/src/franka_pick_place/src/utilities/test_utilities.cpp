#include "franka_pick_place/franka_pick_and_place.hpp"

void FrankaPickAndPlace::testFunctionalities()
{
    RCLCPP_INFO(this->get_logger(), "==========Testing Functionalities==========");

    test_task_ = std::make_unique<moveit::task_constructor::Task>();
    test_task_->setName("test_task");
    test_task_->loadRobotModel(shared_from_this());

    test_task_->setProperty("group", "fr3_arm");
    test_task_->setProperty("eef", "fr3_hand");
    test_task_->setProperty("ik_frame", "fr3_hand_tcp");

    testGripperRotations();

}

void FrankaPickAndPlace::testGripperRotations()
{   
    double yaw_deg = 45.0;

    RCLCPP_INFO(this->get_logger(), "-----Testing Functionalities-----");

    auto pipeline_planner = std::make_shared<moveit::task_constructor::solvers::PipelinePlanner>(shared_from_this(), "ompl");
    pipeline_planner->setPlannerId("RRTConnectConfigDefault");

    auto current_state = std::make_unique<moveit::task_constructor::stages::CurrentState>("current state");
    test_task_->add(std::move(current_state));

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveTo>("test rotation", pipeline_planner);
    stage->setGroup("fr3_arm");

    std::map<std::string, double> joint_positions;
    joint_positions["fr3_joint7"] = yaw_deg * M_PI / 180.0; 

    stage->setGoal(joint_positions);
    test_task_->add(std::move(stage));

    if (test_task_->plan(1))
    {
        RCLCPP_INFO(this->get_logger(), "SUCCESS: Yaw deg=%f", yaw_deg);

        auto solutions = test_task_->solutions();
        if (!solutions.empty())
        {
            auto result = test_task_->execute(*solutions.front());

            if (result.val == moveit_msgs::msg::MoveItErrorCodes::SUCCESS)
            {
                RCLCPP_INFO(this->get_logger(), "EXECUTED: Yaw deg=%f", yaw_deg);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            else
            {
                RCLCPP_ERROR(this->get_logger(), "EXECUTION FAILED: Yaw deg=%f", yaw_deg);
            }
        }
    }
    else
    {
        RCLCPP_WARN(this->get_logger(), "FAILED: Yaw deg=%f", yaw_deg);
    }

    test_task_->clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}