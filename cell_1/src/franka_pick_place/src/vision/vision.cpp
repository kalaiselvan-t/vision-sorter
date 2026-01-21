#include "franka_pick_place/vision.hpp"

VisionPipeline::VisionPipeline() : Node("vision_pipeline_node")
{
    RCLCPP_INFO(this->get_logger(), "Vision Pipeline Started");

    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "/rgbd_camera/image",
        10,
        std::bind(&VisionPipeline::imageCallback, this, std::placeholders::_1)
    );

    pointcloud_subscriber_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
        "/rgbd_camera/points",
        10,
        std::bind(&VisionPipeline::pointcloudCallback, this, std::placeholders::_1)
    );

    cube_pose_publisher_ = this->create_publisher<vision_msgs::msg::Detection3DArray>(
        "vision/detected_cube_pose",
        10
    );

    RCLCPP_INFO(this->get_logger(), "Subscribed to /rgbd_camera/image");
}

void VisionPipeline::imageCallback(const sensor_msgs::msg::Image::SharedPtr msg)
{
    RCLCPP_INFO(this->get_logger(), "========== Image Received ==========");

    try 
    {
        cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::RGB8);
        cv::Mat rgb_image = cv_ptr->image;

        cv::Mat hsv_image;
        cv::cvtColor(rgb_image, hsv_image, cv::COLOR_RGB2HSV);

        std::vector<VisionPipeline::CubeDetection> cubes = VisionPipeline::detectCubes(&hsv_image);

        vision_msgs::msg::Detection3DArray Cubes;

        for (auto& cube: cubes)
        {
            if(latest_pointcloud_)
            {
                Cube3D cube_3d = VisionPipeline::get3DPosition(latest_pointcloud_, cube);

                if(cube_3d.valid)
                {
                    cube.x = cube_3d.x;
                    cube.y = cube_3d.y;
                    cube.z = cube_3d.z;
                    cube.has_3d_position = true;

                    geometry_msgs::msg::PoseStamped pose_base = VisionPipeline::transformToBaseFrame(cube.x, cube.y, cube.z);

                    if (!isOnTable(pose_base.pose))
                    {
                        RCLCPP_DEBUG(this->get_logger(), "Cube %s not on table - skipping (x=%.2f, y=%.2f, z=%.2f)",
                                    cube.color.c_str(),
                                    pose_base.pose.position.x,
                                    pose_base.pose.position.y,
                                    pose_base.pose.position.z);
                        continue;
                    }

                    vision_msgs::msg::Detection3D detection;
                    vision_msgs::msg::ObjectHypothesisWithPose result;
                    detection.header = pose_base.header;
                    result.hypothesis.class_id = cube.color;
                    result.hypothesis.score = 1.0;
                    result.pose.pose = pose_base.pose;
                    detection.results.push_back(result);

                    detection.id = cube.color + "_cube";

                    Cubes.detections.push_back(detection);

                    RCLCPP_INFO(this->get_logger(), "Cube detected:");
                    RCLCPP_INFO(this->get_logger(), "  Color: %s", cube.color.c_str());
                    RCLCPP_INFO(this->get_logger(), "X: %f, Y: %f, Z: %f, OX: %f, OY: %f, OZ: %f, OW: %f",
                        pose_base.pose.position.x,
                        pose_base.pose.position.y,
                        pose_base.pose.position.z,
                        pose_base.pose.orientation.x,
                        pose_base.pose.orientation.y,
                        pose_base.pose.orientation.z,
                        pose_base.pose.orientation.w
                    );
                }
                else 
                {
                    RCLCPP_WARN(this->get_logger(), "Could not get 3D position for %s cube",cube.color.c_str());
                }
            }
        }
        cube_pose_publisher_->publish(Cubes);

        RCLCPP_INFO(this->get_logger(), "====================================\n");

    } 
    catch (cv_bridge::Exception& e) 
    {
        RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    }
}

std::vector<VisionPipeline::CubeDetection> VisionPipeline::detectCubes(const cv::Mat* msg)
{
    std::vector<VisionPipeline::CubeDetection> cubes;

    struct ColorRange {
        std::string name;
        cv::Scalar lower;
        cv::Scalar upper;
    };

    std::vector<ColorRange> colors = {
        {"green", cv::Scalar{40, 50, 50}, cv::Scalar{80, 255, 255}},
        {"red_low", cv::Scalar{0, 100, 100}, cv::Scalar{10, 255, 255}},
        {"red_high", cv::Scalar{170, 100, 100}, cv::Scalar{180, 255, 255}},
        {"blue", cv::Scalar{100, 100, 100}, cv::Scalar{130, 255, 255}},
        {"yellow", cv::Scalar{20, 100, 100}, cv::Scalar{30, 255, 255}},
        {"orange", cv::Scalar{10, 100, 100}, cv::Scalar{20, 255, 255}},
        {"purple", cv::Scalar{130, 50, 50}, cv::Scalar{160, 255, 255}}
    };

    for (const auto& color_range: colors)
    {
        cv::Mat mask;
        cv::inRange(*msg, color_range.lower, color_range.upper, mask);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& contour: contours)
        {
            double area = cv::contourArea(contour);
            if (area > 100 && area < 3000)
            {
                cv::Moments m = cv::moments(contour);
                if (m.m00 != 0)
                {
                    VisionPipeline::CubeDetection cube;
                    cube.color = color_range.name;
                    cube.center_x = m.m10 / m.m00;
                    cube.center_y = m.m01 / m.m00;
                    cube.area = area;
                    cubes.push_back(cube);
                }
            }
        }
    }

    return cubes;
}

void VisionPipeline::pointcloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
{
    latest_pointcloud_ = msg;
}

VisionPipeline::Cube3D VisionPipeline::get3DPosition(const sensor_msgs::msg::PointCloud2::SharedPtr cloud,
                             const VisionPipeline::CubeDetection& cube_2d)
{
    VisionPipeline::Cube3D cube_3d;
    cube_3d.color = cube_2d.color;
    cube_3d.pixel_x = cube_2d.center_x;
    cube_3d.pixel_y = cube_2d.center_y;
    cube_3d.valid = false;

    if (!cloud)
    {
        RCLCPP_WARN(this->get_logger(), "No point cloud available");
        return cube_3d;
    }

    int index = cube_2d.center_y * cloud->width + cube_2d.center_x;

    if (index < 0 || index >= (int)(cloud->width * cloud->height)) 
    {
        RCLCPP_ERROR(this->get_logger(), "Index out of bounds: %d", index);
        return cube_3d;
    }

    sensor_msgs::PointCloud2ConstIterator<float> iter_x(*cloud, "x");
    sensor_msgs::PointCloud2ConstIterator<float> iter_y(*cloud, "y");
    sensor_msgs::PointCloud2ConstIterator<float> iter_z(*cloud, "z");

    iter_x += index;
    iter_y += index;
    iter_z += index;

    cube_3d.x = *iter_x;
    cube_3d.y = *iter_y;
    cube_3d.z = *iter_z;

    if (std::isfinite(cube_3d.x) && std::isfinite(cube_3d.y) && std::isfinite(cube_3d.z))
    {
        cube_3d.valid = true;
    }
    else
    {
        RCLCPP_WARN(this->get_logger(), "Invalid 3D point (NaN or Inf)");
    }

    return cube_3d;
}

geometry_msgs::msg::PoseStamped VisionPipeline::transformToBaseFrame(float x_camera, float y_camera, float z_camera)
{
    geometry_msgs::msg::PoseStamped pose_camera;
    pose_camera.header.frame_id = "workspace_camera/camera_link/rgbd_camera_sensor";
    pose_camera.header.stamp = this->now();
    pose_camera.pose.position.x = x_camera;
    pose_camera.pose.position.y = y_camera;
    pose_camera.pose.position.z = z_camera;
    pose_camera.pose.orientation.w = 1.0;

    geometry_msgs::msg::PoseStamped pose_base;

    try
    {
        pose_base = tf_buffer_->transform(
            pose_camera,
            "fr3_link0",
            tf2::durationFromSec(0.5)
        );

        RCLCPP_INFO(this->get_logger(), "Transformed position:");
        RCLCPP_INFO(this->get_logger(), "  Camera frame: (%.3f, %.3f, %.3f)", x_camera, y_camera, z_camera);
        RCLCPP_INFO(this->get_logger(), "  Base frame: (%.3f, %.3f, %.3f)",
              pose_base.pose.position.x,
              pose_base.pose.position.y,
              pose_base.pose.position.z);

        return pose_base;
    }
    catch (tf2::TransformException &ex)
    {
        RCLCPP_ERROR(this->get_logger(), "TF transform failed: %s", ex.what());
        return pose_camera;
    }
}

bool VisionPipeline::isOnTable(const geometry_msgs::msg::Pose& pose)
{
    // Table bounds (must match planning_scene.yaml and MTC configuration)
    double table_x_min = 0.35;
    double table_x_max = 0.85;
    double table_y_min = -0.25;
    double table_y_max = 0.25;
    double table_z_min = 0.05;
    double table_z_max = 0.15;

    bool on_table = (pose.position.x >= table_x_min && pose.position.x <= table_x_max &&
                    pose.position.y >= table_y_min && pose.position.y <= table_y_max &&
                    pose.position.z >= table_z_min && pose.position.z <= table_z_max);

    return on_table;
}


