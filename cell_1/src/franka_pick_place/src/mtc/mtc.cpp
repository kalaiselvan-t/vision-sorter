#include "franka_pick_place/franka_pick_and_place.hpp"

void FrankaPickAndPlace::initializeMTC()
{
    RCLCPP_INFO(this->get_logger(), "==========Initializing MTC==========");

    loadPlanningSceneFromConfig();

    mtc_task_ = std::make_unique<moveit::task_constructor::Task>();
    mtc_task_->setName("franka_pick_and_place_mtc_task");
    mtc_task_->loadRobotModel(shared_from_this());

    mtc_task_->setProperty("group", "fr3_arm");
    mtc_task_->setProperty("eef", "fr3_hand");
    mtc_task_->setProperty("ik_frame", "fr3_hand_tcp");

    addCurrentStateStage();
    addApproachStage();
    addGripperOpenStage();
    addPreGraspStage();
    addAllowCollisionStage();
    addGripperCloseStage();
    addAttachObjectStage();
    addLiftStage();
    addPlaceApproachStage();
    addPlaceDescentStage();
    addDetachObjectStage();
    addGripperReleaseStage();
    addPostPlaceRetreatStage();
    addDisallowCollisionStage();
    returnHomeStage();

    mtc_thread_ = std::thread([this]() {
        while(rclcpp::ok() && !shutdown_requested_)
        {   
            if(new_cube_available_)
            {
                geometry_msgs::msg::Pose cube_pose;
                std::string cube_color;
                {
                    std::lock_guard<std::mutex> lock(cube_mutex_);
                    cube_pose = target_cube_pose;
                    cube_color = target_cube_color;

                    RCLCPP_INFO(this->get_logger(), "MTC thread received new pose: x=%.2f, y=%.2f, z=%.2f, ox=%.2f, oy=%.2f, oz=%.2f, ow=%.2f", 
                        cube_pose.position.x, 
                        cube_pose.position.y, 
                        cube_pose.position.z,
                        cube_pose.orientation.x,
                        cube_pose.orientation.y,
                        cube_pose.orientation.z,
                        cube_pose.orientation.w
                    );
                }
                new_cube_available_ = false;

                pipeline_executing_ = true;
                RCLCPP_INFO(this->get_logger(), "Pipeline execution started - cube tracking disabled");
                
                setApproachStage(cube_pose);
                setPlaceApproachStage();

                RCLCPP_INFO(this->get_logger(), "Planning with new cube pose...");

                planAndExecute();

                pipeline_executing_ = false;
                RCLCPP_INFO(this->get_logger(), "Pipeline execution completed - cube tracking re-enabled");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void FrankaPickAndPlace::addCurrentStateStage()
{
    auto current_state = std::make_unique<moveit::task_constructor::stages::CurrentState>("current state");
    mtc_task_->add(std::move(current_state));
}

void FrankaPickAndPlace::addApproachStage()
{
    auto pipeline_planner = std::make_shared<moveit::task_constructor::solvers::PipelinePlanner>(shared_from_this(), "ompl");
    pipeline_planner->setPlannerId("RRTConnectConfigDefault");

    auto approach = std::make_unique<moveit::task_constructor::stages::MoveTo>("approach", pipeline_planner);
    approach->setGroup("fr3_arm");

    mtc_task_->add(std::move(approach));
}

void FrankaPickAndPlace::setApproachStage(const geometry_msgs::msg::Pose& pose)
{
    geometry_msgs::msg::PoseStamped approach_pose;
    approach_pose.header.frame_id = "world";
    approach_pose.pose = pose;
    approach_pose.pose.position.z += APPROACH_DISTANCE;

    tf2::Quaternion gripper_orientation;
    gripper_orientation.setRPY(0.0, 180.0 * M_PI/180.0, -44.75 * M_PI/180.0);
    approach_pose.pose.orientation = tf2::toMsg(gripper_orientation);

    RCLCPP_INFO(this->get_logger(), "Approach pose: x=%.3f, y=%.3f, z=%.3f, ox=%.3f, oy=%.3f, oz=%.3f, ow=%.3f",
                approach_pose.pose.position.x,
                approach_pose.pose.position.y,
                approach_pose.pose.position.z,
                approach_pose.pose.orientation.x,
                approach_pose.pose.orientation.y,
                approach_pose.pose.orientation.z,
                approach_pose.pose.orientation.w
    );

    auto stage = mtc_task_->stages()->findChild("approach");
    if (!stage) 
    {
      RCLCPP_ERROR(this->get_logger(), "Approach stage not found!");
      return;
    }

    stage->setProperty("goal", approach_pose);
}

void FrankaPickAndPlace::addRotateGripperStage(double yaw_deg)
{
    auto pipeline_planner = std::make_shared<moveit::task_constructor::solvers::PipelinePlanner>(shared_from_this(), "ompl");
    pipeline_planner->setPlannerId("RRTConnectConfigDefault");

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveTo>("test rotation", pipeline_planner);
    stage->setGroup("fr3_arm");

    std::map<std::string, double> joint_positions;
    joint_positions["fr3_joint7"] = yaw_deg * M_PI / 180.0; 

    stage->setGoal(joint_positions);
    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addGripperOpenStage()
{
    auto interpolation_planner = std::make_shared<moveit::task_constructor::solvers::JointInterpolationPlanner>();
    auto stage = std::make_unique<moveit::task_constructor::stages::MoveTo>("open gripper", interpolation_planner);
    stage->setGroup("fr3_hand");
    stage->setGoal("open");

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addPreGraspStage()
{
    auto cartesian_planner = std::make_shared<moveit::task_constructor::solvers::CartesianPath>();
    cartesian_planner->setMinFraction(0.9);
    cartesian_planner->setMaxVelocityScalingFactor(0.2);
    cartesian_planner->setMaxAccelerationScalingFactor(0.2);
    cartesian_planner->setStepSize(0.005);

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveRelative>("pre-grasp", cartesian_planner);
    stage->setGroup("fr3_arm");
    stage->setMinMaxDistance(PRE_GRASP_DISTANCE, PRE_GRASP_DISTANCE);

    geometry_msgs::msg::Vector3Stamped direction;
    direction.header.frame_id = "world";
    direction.vector.z = -PRE_GRASP_DISTANCE;

    stage->setDirection(direction);

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addAllowCollisionStage()
{
    auto stage = std::make_unique<moveit::task_constructor::stages::ModifyPlanningScene>("allow collision");

    // Allow collisions between the gripper and the cube
    stage->allowCollisions("target_cube",
        mtc_task_->getRobotModel()
            ->getJointModelGroup("fr3_hand")
            ->getLinkModelNamesWithCollisionGeometry(),
        true);

    // Allow collisions between cube and table (cube sits on table)
    stage->allowCollisions("target_cube", "work_table", true);

    // Allow collisions between cube and all bins (in case cube is near/touching bins)
    stage->allowCollisions("target_cube", "green_bin", true);
    stage->allowCollisions("target_cube", "black_bin", true);
    stage->allowCollisions("target_cube", "red_bin", true);
    stage->allowCollisions("target_cube", "blue_bin", true);

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addGripperCloseStage()
{
    auto interpolation_planner = std::make_shared<moveit::task_constructor::solvers::JointInterpolationPlanner>();
    auto stage = std::make_unique<moveit::task_constructor::stages::MoveTo>("close gripper", interpolation_planner);
    stage->setGroup("fr3_hand");

    double finger_position = (cube_size / 2.0) - 0.002;
    finger_position = std::max(0.001, std::min(finger_position, 0.04));

    std::map<std::string, double> gripper_joints;
    gripper_joints["fr3_finger_joint1"] = finger_position;

    stage->setGoal(gripper_joints);

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addAttachObjectStage()
{
    auto stage = std::make_unique<moveit::task_constructor::stages::ModifyPlanningScene>("attach object");
    stage->attachObject("target_cube", "fr3_hand");
    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addLiftStage()
{
    auto cartesian_planner = std::make_shared<moveit::task_constructor::solvers::CartesianPath>();
    cartesian_planner->setMinFraction(0.5);
    cartesian_planner->setMaxVelocityScalingFactor(0.2);
    cartesian_planner->setMaxAccelerationScalingFactor(0.2);
    cartesian_planner->setStepSize(0.005);

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveRelative>("lift", cartesian_planner);
    stage->setGroup("fr3_arm");
    stage->setMinMaxDistance(LIFT_DISTANCE, LIFT_DISTANCE);

    geometry_msgs::msg::Vector3Stamped direction;
    direction.header.frame_id = "world";
    direction.vector.z = LIFT_DISTANCE;

    stage->setDirection(direction);

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addPlaceApproachStage()
{
    auto pipeline_planner = std::make_shared<moveit::task_constructor::solvers::PipelinePlanner>(shared_from_this(), "ompl");
    pipeline_planner->setPlannerId("RRTConnectConfigDefault");
    pipeline_planner->setMaxVelocityScalingFactor(0.2);
    pipeline_planner->setMaxAccelerationScalingFactor(0.2);

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveTo>("place approach", pipeline_planner);
    stage->setGroup("fr3_arm");

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::setPlaceApproachStage()
{
    geometry_msgs::msg::PoseStamped place_approach_pose;
    place_approach_pose.header.frame_id = "world";
    place_approach_pose.pose = FrankaPickAndPlace::getBinPoseForColor(target_cube_color);
    place_approach_pose.pose.position.z += APPROACH_DISTANCE;

    Eigen::Quaterniond q(Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitX()));
    place_approach_pose.pose.orientation.x = q.x();
    place_approach_pose.pose.orientation.y = q.y();
    place_approach_pose.pose.orientation.z = q.z();
    place_approach_pose.pose.orientation.w = q.w();

    // tf2::Quaternion gripper_orientation;
    // gripper_orientation.setRPY(0.0, 180.0 * M_PI/180.0, -45.0 * M_PI/180.0);
    // place_approach_pose.pose.orientation = tf2::toMsg(gripper_orientation);

    RCLCPP_INFO(this->get_logger(), "Place approach pose: x=%.3f, y=%.3f, z=%.3f, ox=%.3f, oy=%.3f, oz=%.3f, ow=%.3f",
                place_approach_pose.pose.position.x,
                place_approach_pose.pose.position.y,
                place_approach_pose.pose.position.z,
                place_approach_pose.pose.orientation.x,
                place_approach_pose.pose.orientation.y,
                place_approach_pose.pose.orientation.z,
                place_approach_pose.pose.orientation.w
    );

    auto stage = mtc_task_->stages()->findChild("place approach");
    if (stage)
    {
        stage->setProperty("goal", place_approach_pose);
    }
}

void FrankaPickAndPlace::addPlaceDescentStage()
{
    auto cartesian_planner = std::make_shared<moveit::task_constructor::solvers::CartesianPath>();
    cartesian_planner->setMinFraction(0.5);
    cartesian_planner->setMaxVelocityScalingFactor(0.2);     // Very slow - 10% speed
    cartesian_planner->setMaxAccelerationScalingFactor(0.2); // Very slow - 10% accel
    cartesian_planner->setStepSize(0.005);                   // 5mm steps

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveRelative>("place descent", cartesian_planner);
    stage->setGroup("fr3_arm");

    double descent_distance = -PRE_GRASP_DISTANCE;

    stage->setMinMaxDistance(-descent_distance, -descent_distance);

    geometry_msgs::msg::Vector3Stamped direction;
    direction.header.frame_id = "world";
    direction.vector.z = descent_distance;

    stage->setDirection(direction);

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addDetachObjectStage()
{
    auto stage = std::make_unique<moveit::task_constructor::stages::ModifyPlanningScene>("detach object");
    stage->detachObject("target_cube", "fr3_hand");
    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addGripperReleaseStage()
{
    auto interpolation_planner = std::make_shared<moveit::task_constructor::solvers::JointInterpolationPlanner>();
    auto stage = std::make_unique<moveit::task_constructor::stages::MoveTo>("release gripper", interpolation_planner);
    stage->setGroup("fr3_hand");
    stage->setGoal("open");

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addPostPlaceRetreatStage()
{
    auto cartesian_planner = std::make_shared<moveit::task_constructor::solvers::CartesianPath>();
    cartesian_planner->setMinFraction(0.5);
    cartesian_planner->setMaxVelocityScalingFactor(0.2);
    cartesian_planner->setMaxAccelerationScalingFactor(0.2);
    cartesian_planner->setStepSize(0.005);

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveRelative>("post-place retreat", cartesian_planner);
    stage->setGroup("fr3_arm");

    double retreat_distance = APPROACH_DISTANCE;
    stage->setMinMaxDistance(retreat_distance, retreat_distance);

    geometry_msgs::msg::Vector3Stamped direction;
    direction.header.frame_id = "world";
    direction.vector.z = retreat_distance;

    stage->setDirection(direction);

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::addDisallowCollisionStage()
{
    auto stage = std::make_unique<moveit::task_constructor::stages::ModifyPlanningScene>("disallow collision");

    stage->allowCollisions("target_cube",
        mtc_task_->getRobotModel()
            ->getJointModelGroup("fr3_hand")
            ->getLinkModelNamesWithCollisionGeometry(),
        false
    );

    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::returnHomeStage()
{
    auto pipeline_planner = std::make_shared<moveit::task_constructor::solvers::PipelinePlanner>(shared_from_this(), "ompl");
    pipeline_planner->setPlannerId("RRTConnectConfigDefault");
    pipeline_planner->setMaxVelocityScalingFactor(0.5);  // Moderate speed for return
    pipeline_planner->setMaxAccelerationScalingFactor(0.5);

    auto stage = std::make_unique<moveit::task_constructor::stages::MoveTo>("return home", pipeline_planner);
    stage->setGroup("fr3_arm");

    // Define home position joint values (safe neutral pose)
    std::map<std::string, double> home_joint_positions;
    home_joint_positions["fr3_joint1"] = 0.0;
    home_joint_positions["fr3_joint2"] = -0.785;  // -45 degrees
    home_joint_positions["fr3_joint3"] = 0.0;
    home_joint_positions["fr3_joint4"] = -2.356;  // -135 degrees
    home_joint_positions["fr3_joint5"] = 0.0;
    home_joint_positions["fr3_joint6"] = 1.571;   // 90 degrees
    home_joint_positions["fr3_joint7"] = 0.785;   // 45 degrees

    stage->setGoal(home_joint_positions);
    mtc_task_->add(std::move(stage));
}

void FrankaPickAndPlace::planAndExecute()
{
    mtc_task_->reset();

    if(mtc_task_->plan(5))
    {
        RCLCPP_INFO(this->get_logger(), "Planning succeeded");

        auto solutions = mtc_task_->solutions();
        if (!solutions.empty())
        {
            RCLCPP_INFO(this->get_logger(), "Number of stages: %zu", solutions.size());
            
            auto result = mtc_task_->execute(*solutions.front());

            if (result.val == moveit_msgs::msg::MoveItErrorCodes::SUCCESS)
            {
                RCLCPP_INFO(this->get_logger(), "Execution succeeded!");
            }
            else
            {
                RCLCPP_ERROR(this->get_logger(), "Execution failed with code: %d", result.val);
            }
        }
    }
    else
    {
        RCLCPP_ERROR(this->get_logger(), "Planning failed");

        // This prints to stdout, not ROS logger
        std::cout << "\n=== Full MTC State ===" << std::endl;
        mtc_task_->printState();
        std::cout << "=====================\n" << std::endl;
    }
}

// Utilities

geometry_msgs::msg::Pose FrankaPickAndPlace::getCurrentEndEffectorPose()
{
    geometry_msgs::msg::Pose current_pose;
    
    try 
    {
        // Use TF2 to get current end effector pose
        auto transform = tf_buffer_->lookupTransform("world", "fr3_hand_tcp", tf2::TimePointZero);
        
        current_pose.position.x = transform.transform.translation.x;
        current_pose.position.y = transform.transform.translation.y;
        current_pose.position.z = transform.transform.translation.z;
        current_pose.orientation = transform.transform.rotation;
        
        RCLCPP_INFO(this->get_logger(), "Current EE pose from TF2: x=%.3f, y=%.3f, z=%.3f",
                    current_pose.position.x, current_pose.position.y, current_pose.position.z);
    }
    catch (tf2::TransformException& ex)
    {
        RCLCPP_WARN(this->get_logger(), "Could not get current EE pose from TF2: %s", ex.what());
        
        // Fallback: use default downward orientation
        tf2::Quaternion quaternion;
        quaternion.setRPY(M_PI, 0.0, 0.0);  // Point gripper downward
        current_pose.orientation = tf2::toMsg(quaternion);
    }
    
    return current_pose;
}
