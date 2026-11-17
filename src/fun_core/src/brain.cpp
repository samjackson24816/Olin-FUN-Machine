// brain: runs the scenes of docs/Script.md, one at a time.
//
// It implements fun_core::Machine for the scenes: scenes never touch ROS directly.
// Written so far: "intro", "sliders". Going to a scene that is not written yet
// logs a warning and the brain stops there.
//
// Topics and actions (N = device ID):
//   subscribes /teensy/btn_N, /teensy/slider_N, /teensy/prox_N   inputs from teensy_node
//   publishes  /teensy/led_N, /teensy/strip, /teensy/snapshot     outputs to teensy_node
//   publishes  /audio/loop (SoundLoop), /matrix/command (MatrixCommand)
//   action client /play_clip (PlayClip)                           clip_player
//
// Parameters:
//   start_scene      (string, "intro")  the scene to start with, e.g. "sliders" to test it alone
//   start_delay_sec  (double, 1.0)      wait for the other nodes to connect before starting
//   num_buttons, num_sliders, num_prox, num_leds: must match teensy_node
//   big_red_led, prox_sensor, intro.*, sliders.*: see IntroConfig and SlidersConfig

#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "fun_core/scenes/intro_scene.hpp"
#include "fun_core/scene.hpp"
#include "fun_core/scenes/sliders_scene.hpp"
#include "fun_interfaces/action/play_clip.hpp"
#include "fun_interfaces/msg/analog_reading.hpp"
#include "fun_interfaces/msg/button_event.hpp"
#include "fun_interfaces/msg/led_command.hpp"
#include "fun_interfaces/msg/matrix_command.hpp"
#include "fun_interfaces/msg/sound_loop.hpp"
#include "fun_interfaces/msg/strip_command.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/empty.hpp"

using namespace std::chrono_literals;
using fun_interfaces::action::PlayClip;
using fun_interfaces::msg::AnalogReading;
using fun_interfaces::msg::ButtonEvent;
using fun_interfaces::msg::LedCommand;
using fun_interfaces::msg::MatrixCommand;
using fun_interfaces::msg::SoundLoop;
using fun_interfaces::msg::StripCommand;

class Brain : public rclcpp::Node, public fun_core::Machine
{
public:
  Brain()
  : Node("brain"), rng_(std::random_device{}())
  {
    const std::string start_scene = declare_parameter<std::string>("start_scene", "intro");
    const double start_delay = declare_parameter<double>("start_delay_sec", 1.0);
    const int num_buttons = declare_parameter<int>("num_buttons", 4);
    const int num_sliders = declare_parameter<int>("num_sliders", 3);
    const int num_prox = declare_parameter<int>("num_prox", 1);
    const int num_leds = declare_parameter<int>("num_leds", 4);
    read_scene_configs(num_sliders);

    // Inputs from the Teensy: forward each to the scene that is running.
    for (int i = 0; i < num_buttons; i++) {
      button_subs_.push_back(create_subscription<ButtonEvent>(
        "/teensy/btn_" + std::to_string(i), 10, [this, i](const ButtonEvent & msg) {
          if (scene_) {scene_->on_button(i, msg.pressed);}
        }));
    }
    for (int i = 0; i < num_sliders; i++) {
      analog_subs_.push_back(create_subscription<AnalogReading>(
        "/teensy/slider_" + std::to_string(i), 10, [this, i](const AnalogReading & msg) {
          if (scene_) {scene_->on_slider(i, msg.value);}
        }));
    }
    for (int i = 0; i < num_prox; i++) {
      analog_subs_.push_back(create_subscription<AnalogReading>(
        "/teensy/prox_" + std::to_string(i), 10, [this, i](const AnalogReading & msg) {
          if (scene_) {scene_->on_prox(i, msg.value);}
        }));
    }

    // Outputs.
    for (int i = 0; i < num_leds; i++) {
      led_pubs_.push_back(create_publisher<LedCommand>("/teensy/led_" + std::to_string(i), 10));
    }
    strip_pub_ = create_publisher<StripCommand>("/teensy/strip", 10);
    snapshot_pub_ = create_publisher<std_msgs::msg::Empty>("/teensy/snapshot", 10);
    loop_pub_ = create_publisher<SoundLoop>("/audio/loop", 10);
    matrix_pub_ = create_publisher<MatrixCommand>("/matrix/command", 10);
    clip_client_ = rclcpp_action::create_client<PlayClip>(this, "/play_clip");

    pending_scene_ = start_scene;
    start_at_ = now_sec() + start_delay;
    tick_timer_ = create_wall_timer(20ms, [this] {tick();});
    RCLCPP_INFO(get_logger(), "Starting with scene \"%s\" in %.1f s", start_scene.c_str(), start_delay);
  }

  // ---------- fun_core::Machine ----------

  double now_sec() override
  {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  }

  std::mt19937 & rng() override {return rng_;}

  rclcpp::Logger logger() override {return get_logger();}

  void play(const std::string & clip_id, std::function<void()> on_done, bool loop) override
  {
    const int token = ++clip_token_;
    // Runs on_done only if this is still the newest clip and the scene has not changed.
    auto finished = [this, token, generation = generation_, on_done, clip_id](const char * how) {
        if (token != clip_token_ || generation != generation_) {
          return;
        }
        RCLCPP_DEBUG(get_logger(), "Clip %s %s", clip_id.c_str(), how);
        if (on_done) {
          posted_.push_back(on_done);  // run from tick(), not from inside a ROS callback
        }
      };

    if (!clip_client_->action_server_is_ready()) {
      RCLCPP_WARN(get_logger(), "clip_player is not running, skipping clip %s", clip_id.c_str());
      finished("skipped");
      return;
    }

    PlayClip::Goal goal;
    goal.clip_id = clip_id;
    goal.loop = loop;
    auto options = rclcpp_action::Client<PlayClip>::SendGoalOptions();
    options.goal_response_callback = [this, finished, clip_id](auto goal_handle) {
        if (!goal_handle) {
          RCLCPP_WARN(get_logger(), "clip_player refused clip %s, skipping it", clip_id.c_str());
          finished("refused");
        }
      };
    options.result_callback = [finished](const auto & result) {
        // Completed, failed (missing file) or canceled: in every case the scene moves on.
        finished(result.result && result.result->status == PlayClip::Result::STATUS_COMPLETED ?
          "completed" : "ended early");
      };
    RCLCPP_INFO(get_logger(), "Play clip: %s%s", clip_id.c_str(), loop ? " (loop)" : "");
    clip_client_->async_send_goal(goal, options);
  }

  void sound_loop(const std::string & clip_id, float volume) override
  {
    SoundLoop msg;
    msg.clip_id = clip_id;
    msg.volume = volume;
    loop_pub_->publish(msg);
  }

  void led_solid(int id, int r, int g, int b) override
  {
    publish_led(id, LedCommand::MODE_SOLID, r, g, b, 0);
  }

  void led_pulse(int id, int r, int g, int b, int period_ms) override
  {
    publish_led(id, LedCommand::MODE_PULSE, r, g, b, period_ms);
  }

  void leds_off() override
  {
    for (size_t i = 0; i < led_pubs_.size(); i++) {
      led_solid(static_cast<int>(i), 0, 0, 0);
    }
  }

  void strip(int r, int g, int b) override
  {
    StripCommand msg;
    msg.r = static_cast<uint8_t>(r);
    msg.g = static_cast<uint8_t>(g);
    msg.b = static_cast<uint8_t>(b);
    strip_pub_->publish(msg);
  }

  void matrix_text(const std::string & text) override
  {
    MatrixCommand msg;
    msg.mode = MatrixCommand::MODE_TEXT;
    msg.text = text;
    matrix_pub_->publish(msg);
    RCLCPP_INFO(get_logger(), "LED matrix: %s", text.c_str());
  }

  void request_snapshot() override
  {
    snapshot_pub_->publish(std_msgs::msg::Empty());
  }

  void go_to(const std::string & scene) override
  {
    pending_scene_ = scene;
  }

private:
  void read_scene_configs(int num_sliders)
  {
    intro_.big_red_led = declare_parameter<int>("big_red_led", intro_.big_red_led);
    intro_.prox_sensor = declare_parameter<int>("prox_sensor", intro_.prox_sensor);
    intro_.dim_red = declare_parameter<int>("intro.dim_red", intro_.dim_red);
    intro_.pulse_red = declare_parameter<int>("intro.pulse_red", intro_.pulse_red);
    intro_.pulse_period_ms = declare_parameter<int>("intro.pulse_period_ms", intro_.pulse_period_ms);
    intro_.faint_light = declare_parameter<int>("intro.faint_light", intro_.faint_light);
    intro_.nearby_threshold = declare_parameter<int>("intro.nearby_threshold", intro_.nearby_threshold);
    intro_.close_threshold = declare_parameter<int>("intro.close_threshold", intro_.close_threshold);
    intro_.hysteresis = declare_parameter<int>("intro.hysteresis", intro_.hysteresis);
    intro_.idle_gap_min_sec = declare_parameter<double>("intro.idle_gap_min_sec", intro_.idle_gap_min_sec);
    intro_.idle_gap_max_sec = declare_parameter<double>("intro.idle_gap_max_sec", intro_.idle_gap_max_sec);
    intro_.callout_gap_min_sec =
      declare_parameter<double>("intro.callout_gap_min_sec", intro_.callout_gap_min_sec);
    intro_.callout_gap_max_sec =
      declare_parameter<double>("intro.callout_gap_max_sec", intro_.callout_gap_max_sec);

    sliders_.num_sliders = num_sliders;
    sliders_.min_target_distance =
      declare_parameter<int>("sliders.min_target_distance", sliders_.min_target_distance);
    sliders_.edge_margin = declare_parameter<int>("sliders.edge_margin", sliders_.edge_margin);
    sliders_.move_threshold = declare_parameter<int>("sliders.move_threshold", sliders_.move_threshold);
    sliders_.target_margin = declare_parameter<int>("sliders.target_margin", sliders_.target_margin);
    sliders_.hold_sec = declare_parameter<double>("sliders.hold_sec", sliders_.hold_sec);
    sliders_.fade_in_sec = declare_parameter<double>("sliders.fade_in_sec", sliders_.fade_in_sec);
  }

  void publish_led(int id, uint8_t mode, int r, int g, int b, int period_ms)
  {
    if (id < 0 || static_cast<size_t>(id) >= led_pubs_.size()) {
      RCLCPP_WARN(get_logger(), "No LED with ID %d", id);
      return;
    }
    LedCommand msg;
    msg.mode = mode;
    msg.r = static_cast<uint8_t>(r);
    msg.g = static_cast<uint8_t>(g);
    msg.b = static_cast<uint8_t>(b);
    msg.period_ms = static_cast<uint16_t>(period_ms);
    led_pubs_[id]->publish(msg);
  }

  std::unique_ptr<fun_core::Scene> make_scene(const std::string & name)
  {
    if (name == "intro") {
      return std::make_unique<fun_core::IntroScene>(*this, intro_);
    }
    if (name == "sliders") {
      return std::make_unique<fun_core::SlidersScene>(*this, sliders_);
    }
    return nullptr;
  }

  void tick()
  {
    if (!started_) {
      if (now_sec() < start_at_) {
        return;
      }
      started_ = true;
    }
    if (!pending_scene_.empty()) {
      switch_scene(pending_scene_);
    }

    // Run callbacks the scenes asked for (finished clips). One may post more, so take a copy.
    std::vector<std::function<void()>> ready;
    ready.swap(posted_);
    for (auto & callback : ready) {
      callback();
      if (!pending_scene_.empty()) {
        break;  // the scene changed; the rest belonged to the old scene
      }
    }
    if (!pending_scene_.empty()) {
      switch_scene(pending_scene_);
    }

    if (scene_) {
      scene_->tick();
    }
  }

  void switch_scene(std::string name)
  {
    pending_scene_.clear();
    posted_.clear();
    generation_++;  // callbacks of the old scene are ignored from now on
    scene_ = make_scene(name);
    if (!scene_) {
      RCLCPP_WARN(
        get_logger(), "Scene \"%s\" is not written yet. The brain stops here.", name.c_str());
      return;
    }
    RCLCPP_INFO(get_logger(), "===== Scene: %s =====", name.c_str());
    scene_->enter();
  }

  std::mt19937 rng_;
  fun_core::IntroConfig intro_;
  fun_core::SlidersConfig sliders_;

  std::unique_ptr<fun_core::Scene> scene_;
  std::string pending_scene_;
  double start_at_ = 0.0;
  bool started_ = false;
  int generation_ = 0;   // goes up on every scene change
  int clip_token_ = 0;   // goes up on every play()
  std::vector<std::function<void()>> posted_;

  std::vector<rclcpp::Subscription<ButtonEvent>::SharedPtr> button_subs_;
  std::vector<rclcpp::Subscription<AnalogReading>::SharedPtr> analog_subs_;
  std::vector<rclcpp::Publisher<LedCommand>::SharedPtr> led_pubs_;
  rclcpp::Publisher<StripCommand>::SharedPtr strip_pub_;
  rclcpp::Publisher<std_msgs::msg::Empty>::SharedPtr snapshot_pub_;
  rclcpp::Publisher<SoundLoop>::SharedPtr loop_pub_;
  rclcpp::Publisher<MatrixCommand>::SharedPtr matrix_pub_;
  rclcpp_action::Client<PlayClip>::SharedPtr clip_client_;
  rclcpp::TimerBase::SharedPtr tick_timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Brain>());
  rclcpp::shutdown();
  return 0;
}
