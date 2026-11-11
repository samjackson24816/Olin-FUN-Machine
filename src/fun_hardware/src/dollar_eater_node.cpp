// dollar_eater_node: connects ROS to the Dollar Eater Arduino.
//
// Topics:
//   publishes  /dollar_eater/bill_switch  (fun_interfaces/ButtonEvent)   switch pressed / released
//   subscribes /dollar_eater/motor        (fun_interfaces/MotorCommand)  run the motor
//
// Parameters:
//   use_sim  (bool, default true)
//            true:  no Arduino needed. Fake the switch by publishing to
//                   /dollar_eater/sim_switch (std_msgs/Bool); motor commands are only logged.
//            false: talk to the real Arduino over USB.
//   port     (string, default "/dev/ttyACM0")  the Arduino's serial port
//
// The node has no logic of its own: the decision maker decides when to eat.

#include <chrono>
#include <memory>
#include <string>

#include "fun_hardware/serial_port.hpp"
#include "fun_interfaces/msg/button_event.hpp"
#include "fun_interfaces/msg/motor_command.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"

using namespace std::chrono_literals;
using fun_interfaces::msg::ButtonEvent;
using fun_interfaces::msg::MotorCommand;

class DollarEaterNode : public rclcpp::Node
{
public:
  DollarEaterNode()
  : Node("dollar_eater")
  {
    use_sim_ = declare_parameter<bool>("use_sim", true);
    port_ = declare_parameter<std::string>("port", "/dev/ttyACM0");

    switch_pub_ = create_publisher<ButtonEvent>("~/bill_switch", 10);

    motor_sub_ = create_subscription<MotorCommand>(
      "~/motor", 10, [this](const MotorCommand & cmd) {
        send("RUN " + std::to_string(cmd.speed) + " " + std::to_string(cmd.duration_ms));
      });

    if (use_sim_) {
      // Simulation: a ROS topic stands in for the physical switch.
      sim_switch_sub_ = create_subscription<std_msgs::msg::Bool>(
        "~/sim_switch", 10, [this](const std_msgs::msg::Bool & msg) {
          handle_line(msg.data ? "SWITCH 1" : "SWITCH 0");
        });
      RCLCPP_INFO(get_logger(), "SIM mode: fake the switch on %s/sim_switch", get_fully_qualified_name());
    } else {
      // Hardware: check the serial port for new lines 100 times per second.
      poll_timer_ = create_wall_timer(10ms, [this] { poll_serial(); });
      RCLCPP_INFO(get_logger(), "HARDWARE mode: Arduino on %s", port_.c_str());
    }
  }

private:
  // Read new lines from the Arduino, or (re)connect if it is not connected.
  void poll_serial()
  {
    if (serial_.is_open()) {
      for (const std::string & line : serial_.read_lines()) {
        handle_line(line);
      }
      if (!serial_.is_open()) {
        RCLCPP_ERROR(get_logger(), "Arduino disconnected");
      }
      return;
    }

    // Not connected: try again once per second.
    const auto now = std::chrono::steady_clock::now();
    if (now - last_connect_try_ < 1s) {
      return;
    }
    last_connect_try_ = now;

    if (serial_.open(port_)) {
      RCLCPP_INFO(get_logger(), "Connected to %s (the Arduino restarts, wait for READY)", port_.c_str());
    } else {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000, "Waiting for Arduino on %s ...", port_.c_str());
    }
  }

  // Handle one line from the Arduino (or from the simulated switch).
  void handle_line(const std::string & line)
  {
    if (line == "SWITCH 1" || line == "SWITCH 0") {
      ButtonEvent event;
      event.stamp = now();
      event.pressed = (line == "SWITCH 1");
      switch_pub_->publish(event);
      RCLCPP_INFO(get_logger(), "Bill switch %s", event.pressed ? "PRESSED" : "released");
    } else if (line.rfind("ERR", 0) == 0) {
      RCLCPP_WARN(get_logger(), "Arduino: %s", line.c_str());
    } else {
      RCLCPP_INFO(get_logger(), "Arduino: %s", line.c_str());  // READY, OK ..., DONE
    }
  }

  // Send one command line to the Arduino (in SIM mode: only log it).
  void send(const std::string & line)
  {
    if (use_sim_) {
      RCLCPP_INFO(get_logger(), "[SIM] would send to Arduino: %s", line.c_str());
    } else if (!serial_.write_line(line)) {
      RCLCPP_ERROR(get_logger(), "Arduino not connected, dropped command: %s", line.c_str());
    }
  }

  bool use_sim_;
  std::string port_;
  fun_hardware::SerialPort serial_;
  std::chrono::steady_clock::time_point last_connect_try_{};

  rclcpp::Publisher<ButtonEvent>::SharedPtr switch_pub_;
  rclcpp::Subscription<MotorCommand>::SharedPtr motor_sub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sim_switch_sub_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DollarEaterNode>());
  rclcpp::shutdown();
  return 0;
}
