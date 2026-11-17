cmake_minimum_required(VERSION 3.20)
project(fun_core)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(fun_interfaces REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(decision_maker src/decision_maker.cpp)
target_link_libraries(decision_maker PRIVATE rclcpp::rclcpp std_msgs::std_msgs ${fun_interfaces_TARGETS})
install(TARGETS decision_maker DESTINATION lib/${PROJECT_NAME})

# Brain: runs the scenes of docs/Script.md (intro, sliders so far)
add_executable(brain
  src/brain.cpp
  src/scenes/intro_scene.cpp
  src/scenes/sliders_scene.cpp
)
target_include_directories(brain PRIVATE include)
target_link_libraries(brain PRIVATE
  rclcpp::rclcpp
  rclcpp_action::rclcpp_action
  std_msgs::std_msgs
  ${fun_interfaces_TARGETS}
)
install(TARGETS brain DESTINATION lib/${PROJECT_NAME})


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

  # Unit tests for the scene logic (no ROS needed)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_scene_logic test/test_scene_logic.cpp)
  target_include_directories(test_scene_logic PRIVATE include)
endif()

ament_package()
