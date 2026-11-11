cmake_minimum_required(VERSION 3.20)
project(fun_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)

# Turn each definition file into C++ (and Python) code.
# Every new .msg / .action file must be added to this list.
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/ButtonEvent.msg"
  "msg/MatrixCommand.msg"
  "msg/MotorCommand.msg"
  "action/PlayClip.action"
  DEPENDENCIES builtin_interfaces
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
