#include <memory>

#include "fun_interfaces/msg/button_event.hpp"
#include "fun_interfaces/msg/motor_command.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/string.hpp"

using fun_interfaces::msg::ButtonEvent;
using fun_interfaces::msg::MotorCommand;

class DecisionMaker : public rclcpp::Node
{
public:
  DecisionMaker()
  : Node("decision_maker")
  {
    voice_pub_ = create_publisher<std_msgs::msg::String>("voice_line_string", 10);

    lever_sub_ = create_subscription<std_msgs::msg::Bool>(
      "big_lever_state", 10, [this](const std_msgs::msg::Bool & msg) {
        if (msg.data) {
          std_msgs::msg::String voice;
          voice.data = "press_the_button";
          voice_pub_->publish(voice);
          RCLCPP_INFO(this->get_logger(), "Lever pulled, playing: %s", voice.data.c_str());
        }
      });

    // Rule: a bill goes into the Dollar Eater -> run the motor to eat it.
    // How fast and how long can be tuned without recompiling:
    //   ros2 launch fun_bringup dollar_eater.launch.xml eat_speed:=200 eat_ms:=2500
    eat_speed_ = declare_parameter<int>("eat_speed", 255);  // -255..255, negative = other direction
    eat_ms_ = declare_parameter<int>("eat_ms", 3000);

    motor_pub_ = create_publisher<MotorCommand>("/dollar_eater/motor", 10);

    bill_sub_ = create_subscription<ButtonEvent>(
      "/dollar_eater/bill_switch", 10, [this](const ButtonEvent & event) {
        if (event.pressed) {
          MotorCommand eat;
          eat.speed = static_cast<int16_t>(eat_speed_);
          eat.duration_ms = static_cast<uint32_t>(eat_ms_);
          motor_pub_->publish(eat);
          RCLCPP_INFO(get_logger(), "Bill inserted, eating it (speed %d for %d ms)", eat_speed_, eat_ms_);
        }
      });
  }

private:
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr voice_pub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr lever_sub_;

  int eat_speed_;
  int eat_ms_;
  rclcpp::Publisher<MotorCommand>::SharedPtr motor_pub_;
  rclcpp::Subscription<ButtonEvent>::SharedPtr bill_sub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DecisionMaker>());
  rclcpp::shutdown();
  return 0;
}
