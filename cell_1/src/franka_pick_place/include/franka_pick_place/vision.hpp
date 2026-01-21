#ifndef VISION_HPP
#define VISION_HPP

#include <thread>
#include <vector>
#include <rclcpp/rclcpp.hpp>

#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <vision_msgs/msg/detection3_d.hpp>
#include <vision_msgs/msg/detection3_d_array.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

class VisionPipeline: public rclcpp::Node
{
    public:
        VisionPipeline();

        ~VisionPipeline()
        {
            RCLCPP_INFO(this->get_logger(), "Vision Pipeline Shutting down");
        }

    private:
        cv::Scalar lower_green{40, 50, 50};
        cv::Scalar upper_green{80, 255, 255};

        struct CubeDetection {
            std::string color;
            int center_x, center_y;
            double area;

            float x, y, z;
            bool has_3d_position;
        };

        struct Cube3D 
        {
            std::string color;
            int pixel_x, pixel_y;
            float x, y, z;
            bool valid;
        };

        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_subscriber_;
        rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_subscriber_;
        sensor_msgs::msg::PointCloud2::SharedPtr latest_pointcloud_;
        std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
        std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

        void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg);
        std::vector<CubeDetection> detectCubes(const cv::Mat* msg);
        void pointcloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg);
        Cube3D get3DPosition(const sensor_msgs::msg::PointCloud2::SharedPtr cloud,
                             const CubeDetection& cube_2d);
        geometry_msgs::msg::PoseStamped transformToBaseFrame(float x_camera, float y_camera, float z_camera);
        bool isOnTable(const geometry_msgs::msg::Pose& pose);
        rclcpp::Publisher<vision_msgs::msg::Detection3DArray>::SharedPtr cube_pose_publisher_;
};

#endif // VISION_HPP