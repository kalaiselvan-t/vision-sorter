#include <thread>
#include <rclcpp/rclcpp.hpp>
#include "franka_pick_place/franka_pick_and_place.hpp"

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<FrankaPickAndPlace>();

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);

    node->initializeMTC();

    std::thread executor_thread([&executor](){
        executor.spin();
    });

    executor_thread.join();
    rclcpp::shutdown();

    return 0;
}