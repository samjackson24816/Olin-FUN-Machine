#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/string.hpp"

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
  }

private:
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr voice_pub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr lever_sub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DecisionMaker>());
  rclcpp::shutdown();
  return 0;
}
