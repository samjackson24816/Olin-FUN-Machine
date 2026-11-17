// clip_player: plays voice lines, videos and sound effects for the brain.
//
// For now there is only a SIM backend: it logs what it would play and pretends every
// clip lasts `sim_clip_sec` seconds. The real audio/video backend comes next; it must
// keep exactly the same action and topic so the brain does not change.
//
// Action  /play_clip   (fun_interfaces/action/PlayClip)  one clip at a time
//   - A new goal while one is playing: the old one ends (STATUS_CANCELED), the new one plays.
//   - Cancel: stop now (STATUS_CANCELED).
//   - loop == true: repeat until canceled or replaced.
// Topic   /audio/loop  (fun_interfaces/SoundLoop)  looping sounds with a live volume,
//   played on top of the clip (e.g. the Sliders scene's "noooo" / "yeesss" drones)
//
// Parameters:
//   sim_clip_sec  (double, default 2.0)  how long each simulated clip "plays"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <memory>
#include <string>

#include "fun_interfaces/action/play_clip.hpp"
#include "fun_interfaces/msg/sound_loop.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

using namespace std::chrono_literals;
using fun_interfaces::action::PlayClip;
using fun_interfaces::msg::SoundLoop;
using GoalHandle = rclcpp_action::ServerGoalHandle<PlayClip>;

class ClipPlayerNode : public rclcpp::Node
{
public:
  ClipPlayerNode()
  : Node("clip_player")
  {
    sim_clip_sec_ = declare_parameter<double>("sim_clip_sec", 2.0);

    server_ = rclcpp_action::create_server<PlayClip>(
      this, "/play_clip",
      [](const rclcpp_action::GoalUUID &, std::shared_ptr<const PlayClip::Goal>) {
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
      },
      [](const std::shared_ptr<GoalHandle>) {
        return rclcpp_action::CancelResponse::ACCEPT;
      },
      [this](const std::shared_ptr<GoalHandle> goal) {start(goal);});

    loop_sub_ = create_subscription<SoundLoop>(
      "/audio/loop", 10, [this](const SoundLoop & msg) {set_loop(msg);});

    timer_ = create_wall_timer(100ms, [this] {update();});
    RCLCPP_INFO(get_logger(), "SIM mode: clips are only logged, each lasts %.1f s", sim_clip_sec_);
  }

private:
  void start(const std::shared_ptr<GoalHandle> goal)
  {
    if (current_ && current_->is_active()) {
      RCLCPP_INFO(get_logger(), "[SIM] interrupted: %s", current_->get_goal()->clip_id.c_str());
      current_->abort(result(PlayClip::Result::STATUS_CANCELED));
    }
    current_ = goal;
    started_ = now();
    last_feedback_sec_ = 0.0;
    RCLCPP_INFO(
      get_logger(), "[SIM] playing: %s%s", goal->get_goal()->clip_id.c_str(),
      goal->get_goal()->loop ? " (loop)" : "");
  }

  // Finish, cancel or send feedback for the clip that is playing.
  void update()
  {
    if (!current_ || !current_->is_active()) {
      return;
    }
    const std::string & clip = current_->get_goal()->clip_id;

    if (current_->is_canceling()) {
      RCLCPP_INFO(get_logger(), "[SIM] stopped: %s", clip.c_str());
      current_->canceled(result(PlayClip::Result::STATUS_CANCELED));
      return;
    }

    const double elapsed = (now() - started_).seconds();
    if (!current_->get_goal()->loop && elapsed >= sim_clip_sec_) {
      RCLCPP_INFO(get_logger(), "[SIM] finished: %s", clip.c_str());
      current_->succeed(result(PlayClip::Result::STATUS_COMPLETED));
      return;
    }

    if (elapsed - last_feedback_sec_ >= 0.5) {
      last_feedback_sec_ = elapsed;
      auto feedback = std::make_shared<PlayClip::Feedback>();
      feedback->elapsed_sec = static_cast<float>(elapsed);
      current_->publish_feedback(feedback);
    }
  }

  void set_loop(const SoundLoop & msg)
  {
    const float volume = std::clamp(msg.volume, 0.0f, 1.0f);
    auto playing = loops_.find(msg.clip_id);

    if (volume == 0.0f) {
      if (playing != loops_.end()) {
        RCLCPP_INFO(get_logger(), "[SIM] loop stopped: %s", msg.clip_id.c_str());
        loops_.erase(playing);
      }
      return;
    }
    if (playing == loops_.end()) {
      RCLCPP_INFO(get_logger(), "[SIM] loop started: %s at volume %.2f", msg.clip_id.c_str(), volume);
      loops_[msg.clip_id] = volume;
      return;
    }
    // Volume changes come many times per second; only log steps of 0.1 or more.
    if (std::fabs(volume - playing->second) >= 0.1f) {
      RCLCPP_INFO(get_logger(), "[SIM] loop %s volume %.2f", msg.clip_id.c_str(), volume);
      playing->second = volume;
    }
  }

  static std::shared_ptr<PlayClip::Result> result(uint8_t status)
  {
    auto r = std::make_shared<PlayClip::Result>();
    r->status = status;
    return r;
  }

  double sim_clip_sec_;
  rclcpp_action::Server<PlayClip>::SharedPtr server_;
  std::shared_ptr<GoalHandle> current_;
  rclcpp::Time started_;
  double last_feedback_sec_ = 0.0;
  std::map<std::string, float> loops_;  // looping sound -> last logged volume
  rclcpp::Subscription<SoundLoop>::SharedPtr loop_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ClipPlayerNode>());
  rclcpp::shutdown();
  return 0;
}
