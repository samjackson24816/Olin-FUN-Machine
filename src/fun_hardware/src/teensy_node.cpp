// teensy_node: connects ROS to the Teensy 4.1.
// Protocol: docs/pi-teensy-protocol.md (version 2).
//
// Topics (N = the device ID, 0-based, the same ID as on the wire):
//   publishes  /teensy/btn_N     (fun_interfaces/ButtonEvent)    button pressed / released
//   publishes  /teensy/slider_N  (fun_interfaces/AnalogReading)  slider position 0..1023
//   publishes  /teensy/prox_N    (fun_interfaces/AnalogReading)  distance reading 0..1023, bigger = closer
//   subscribes /teensy/led_N     (fun_interfaces/LedCommand)     button LED: solid colour or pulse
//   subscribes /teensy/strip     (fun_interfaces/StripCommand)   whole strip to one colour
//   subscribes /teensy/snapshot  (std_msgs/Empty)                ask the Teensy to report every input
//
// Parameters:
//   use_sim  (bool, default true)
//            true:  no Teensy needed. Fake a line from the Teensy by publishing it to
//                   /teensy/sim_line (std_msgs/String), e.g. "SLIDER 0 900".
//                   Commands to the Teensy are only logged.
//            false: talk to the real Teensy over USB.
//   port     (string, default "/dev/ttyACM0")  the Teensy's serial port
//   num_buttons, num_sliders, num_prox, num_leds: how many of each; must match pins.h
//
// The node has no logic of its own: it only translates between ROS and the serial lines.

#include <chrono>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "fun_hardware/serial_port.hpp"
#include "fun_interfaces/msg/analog_reading.hpp"
#include "fun_interfaces/msg/button_event.hpp"
#include "fun_interfaces/msg/led_command.hpp"
#include "fun_interfaces/msg/strip_command.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/empty.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;
using fun_interfaces::msg::AnalogReading;
using fun_interfaces::msg::ButtonEvent;
using fun_interfaces::msg::LedCommand;
using fun_interfaces::msg::StripCommand;

class TeensyNode : public rclcpp::Node
{
public:
  TeensyNode()
  : Node("teensy")
  {
    use_sim_ = declare_parameter<bool>("use_sim", true);
    port_ = declare_parameter<std::string>("port", "/dev/ttyACM0");
    const int num_buttons = declare_parameter<int>("num_buttons", 4);
    const int num_sliders = declare_parameter<int>("num_sliders", 3);
    const int num_prox = declare_parameter<int>("num_prox", 1);
    const int num_leds = declare_parameter<int>("num_leds", 4);

    // One publisher per input, named by its ID.
    for (int i = 0; i < num_buttons; i++) {
      btn_pubs_.push_back(create_publisher<ButtonEvent>("~/btn_" + std::to_string(i), 10));
    }
    for (int i = 0; i < num_sliders; i++) {
      slider_pubs_.push_back(create_publisher<AnalogReading>("~/slider_" + std::to_string(i), 10));
    }
    for (int i = 0; i < num_prox; i++) {
      prox_pubs_.push_back(create_publisher<AnalogReading>("~/prox_" + std::to_string(i), 10));
    }

    // One subscription per button LED.
    for (int i = 0; i < num_leds; i++) {
      led_subs_.push_back(create_subscription<LedCommand>(
        "~/led_" + std::to_string(i), 10, [this, i](const LedCommand & cmd) {
          std::string line;
          if (cmd.mode == LedCommand::MODE_PULSE) {
            line = "PULSE " + std::to_string(i) + " " + rgb(cmd.r, cmd.g, cmd.b) + " " +
              std::to_string(cmd.period_ms);
          } else {
            line = "LED " + std::to_string(i) + " " + rgb(cmd.r, cmd.g, cmd.b);
          }
          set_state("LED " + std::to_string(i), line);
        }));
    }

    strip_sub_ = create_subscription<StripCommand>(
      "~/strip", 10, [this](const StripCommand & cmd) {
        set_state("STRIP", "STRIP " + rgb(cmd.r, cmd.g, cmd.b));
      });

    snapshot_sub_ = create_subscription<std_msgs::msg::Empty>(
      "~/snapshot", 10, [this](const std_msgs::msg::Empty &) {send("SNAPSHOT");});

    if (use_sim_) {
      // Simulation: a ROS topic stands in for the Teensy's messages.
      sim_line_sub_ = create_subscription<std_msgs::msg::String>(
        "~/sim_line", 10, [this](const std_msgs::msg::String & msg) {handle_line(msg.data);});
      RCLCPP_INFO(
        get_logger(), "SIM mode: fake Teensy lines on %s/sim_line, e.g. \"SLIDER 0 900\"",
        get_fully_qualified_name());
    } else {
      // Hardware: check the serial port for new lines 100 times per second.
      poll_timer_ = create_wall_timer(10ms, [this] {poll_serial();});
      RCLCPP_INFO(get_logger(), "HARDWARE mode: Teensy on %s", port_.c_str());
    }
  }

private:
  static std::string rgb(int r, int g, int b)
  {
    return std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b);
  }

  // Remember the latest command for an output and send it.
  // The Teensy forgets everything when it restarts, so these are resent after READY.
  void set_state(const std::string & output, const std::string & line)
  {
    state_[output] = line;
    send(line);
  }

  // Send every remembered output state again, then ask for every input.
  void resync()
  {
    for (const auto & [output, line] : state_) {
      send(line);
    }
    send("SNAPSHOT");
  }

  // Read new lines from the Teensy, or (re)connect if it is not connected.
  void poll_serial()
  {
    if (serial_.is_open()) {
      for (const std::string & line : serial_.read_lines()) {
        handle_line(line);
      }
      if (!serial_.is_open()) {
        RCLCPP_ERROR(get_logger(), "Teensy disconnected");
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
      // Unlike an Arduino Uno, a Teensy does not restart when the port opens,
      // so it may have sent READY long ago: resync right away.
      RCLCPP_INFO(get_logger(), "Connected to %s", port_.c_str());
      resync();
    } else {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000, "Waiting for Teensy on %s ...", port_.c_str());
    }
  }

  // Handle one line from the Teensy (or from /teensy/sim_line).
  void handle_line(const std::string & line)
  {
    std::istringstream in(line);
    std::string kind;
    int id = -1;
    int value = -1;
    in >> kind >> id >> value;

    if (kind == "BTN" && valid(id, btn_pubs_.size()) && (value == 0 || value == 1)) {
      ButtonEvent event;
      event.stamp = now();
      event.pressed = (value == 1);
      btn_pubs_[id]->publish(event);
      RCLCPP_INFO(get_logger(), "Button %d %s", id, event.pressed ? "PRESSED" : "released");
    } else if (kind == "SLIDER" && valid(id, slider_pubs_.size()) && value >= 0 && value <= 1023) {
      slider_pubs_[id]->publish(reading(value));
      RCLCPP_DEBUG(get_logger(), "Slider %d = %d", id, value);
    } else if (kind == "PROX" && valid(id, prox_pubs_.size()) && value >= 0 && value <= 1023) {
      prox_pubs_[id]->publish(reading(value));
      RCLCPP_DEBUG(get_logger(), "Proximity %d = %d", id, value);
    } else if (kind == "READY") {
      RCLCPP_INFO(get_logger(), "Teensy: %s (protocol version %d)", line.c_str(), id);
      resync();
    } else if (kind == "ERR") {
      RCLCPP_WARN(get_logger(), "Teensy: %s", line.c_str());
    } else if (kind == "PONG" || kind == "OK") {
      RCLCPP_INFO(get_logger(), "Teensy: %s", line.c_str());
    } else {
      RCLCPP_WARN(get_logger(), "Teensy sent a line this node does not understand: %s", line.c_str());
    }
  }

  static bool valid(int id, size_t count)
  {
    return id >= 0 && static_cast<size_t>(id) < count;
  }

  AnalogReading reading(int value)
  {
    AnalogReading msg;
    msg.stamp = now();
    msg.value = static_cast<uint16_t>(value);
    return msg;
  }

  // Send one command line to the Teensy (in SIM mode: only log it).
  void send(const std::string & line)
  {
    if (use_sim_) {
      RCLCPP_INFO(get_logger(), "[SIM] would send to Teensy: %s", line.c_str());
    } else if (!serial_.write_line(line)) {
      // Not lost for good: the state is resent when the Teensy connects.
      RCLCPP_WARN(get_logger(), "Teensy not connected, will resend later: %s", line.c_str());
    }
  }

  bool use_sim_;
  std::string port_;
  fun_hardware::SerialPort serial_;
  std::chrono::steady_clock::time_point last_connect_try_{};
  std::map<std::string, std::string> state_;  // output name -> last command line

  std::vector<rclcpp::Publisher<ButtonEvent>::SharedPtr> btn_pubs_;
  std::vector<rclcpp::Publisher<AnalogReading>::SharedPtr> slider_pubs_;
  std::vector<rclcpp::Publisher<AnalogReading>::SharedPtr> prox_pubs_;
  std::vector<rclcpp::Subscription<LedCommand>::SharedPtr> led_subs_;
  rclcpp::Subscription<StripCommand>::SharedPtr strip_sub_;
  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr snapshot_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sim_line_sub_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TeensyNode>());
  rclcpp::shutdown();
  return 0;
}
