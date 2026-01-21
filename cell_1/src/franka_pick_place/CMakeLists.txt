cmake_minimum_required(VERSION 3.8)
project(franka_pick_place)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(moveit_core REQUIRED)
find_package(geometric_shapes REQUIRED)
find_package(shape_msgs REQUIRED)
find_package(franka_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(control_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(eigen3_cmake_module REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(moveit_task_constructor_core REQUIRED)
find_package(moveit_task_constructor_msgs REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(OpenCV REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(vision_msgs REQUIRED)

# Create library for franka_pick_and_place
add_library(franka_pick_place_lib
  src/mtc/franka_pick_and_place.cpp
  src/mtc/cube_tracking.cpp
  src/mtc/mtc.cpp
  src/utilities/test_utilities.cpp
  src/mtc/planning_scene_loader.cpp
)

add_library(vision_pipeline_lib
  src/vision/vision.cpp
)

target_include_directories(franka_pick_place_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(vision_pipeline_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(franka_pick_place_lib
  rclcpp
  geometry_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  shape_msgs
  vision_msgs
  moveit_msgs
  moveit_ros_planning_interface
  moveit_task_constructor_core
  ament_index_cpp
)

ament_target_dependencies(vision_pipeline_lib
  rclcpp
  geometry_msgs
  sensor_msgs
  vision_msgs
  cv_bridge
  tf2
  tf2_ros
  tf2_geometry_msgs
  shape_msgs
  ament_index_cpp
)

target_link_libraries(franka_pick_place_lib
  yaml-cpp
)

target_link_libraries(vision_pipeline_lib
  ${OpenCV_LIBS}
)

# Add executables
add_executable(franka_pick_place_node src/main.cpp)
add_executable(vision_pipeline_node src/vision_main.cpp)

# Link library to franka_pick_and_place_node
target_link_libraries(franka_pick_place_node franka_pick_place_lib)
target_link_libraries(vision_pipeline_node vision_pipeline_lib)

ament_target_dependencies(franka_pick_place_node
  rclcpp
  moveit_ros_planning_interface
  moveit_msgs
  moveit_core
  moveit_task_constructor_core
  moveit_task_constructor_msgs
  geometric_shapes
  shape_msgs
  franka_msgs
  geometry_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  sensor_msgs
  std_msgs
  control_msgs
  trajectory_msgs
  Eigen3
)

ament_target_dependencies(vision_pipeline_node
  rclcpp
  sensor_msgs
  geometry_msgs
  cv_bridge
)

# Install library
install(TARGETS
  franka_pick_place_lib
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS
  vision_pipeline_lib
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install header files
install(DIRECTORY include/
  DESTINATION include
)

# Install executables
install(TARGETS
  franka_pick_place_node
  vision_pipeline_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

# Install config files
install(DIRECTORY
  config
  DESTINATION share/${PROJECT_NAME}/
)

# Install world files
install(DIRECTORY
  worlds
  DESTINATION share/${PROJECT_NAME}/
)

# Install model files
install(DIRECTORY
  models
  DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
