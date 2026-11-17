#include "fun_core/scenes/sliders_scene.hpp"

#include <cmath>
#include <cstdlib>

#include "rclcpp/logging.hpp"

namespace fun_core
{

namespace
{
// Clip IDs. The clip player maps each ID to its files.
const char * INTRO_LINE = "sliders_intro";              // "You know you haven't really been..."
const char * DRONE_NO = "sliders_nooo";                 // looping droning "noooo"
const char * DRONE_YES = "sliders_yesss";               // looping droning "yeeessss"
const char * DING = "sfx_ding";
const char * CANCELLED_LINE = "sliders_cancelled";      // "Self Destruct Sequence Cancelled"
const char * RETURN_LINE = "sliders_character_returns"; // "I think I figured out how to disab- oh..."
const char * SAD_IDLE = "sliders_sad_idle";             // sad idle animation, loops

const double VOLUME_INTERVAL_SEC = 0.05;  // send drone volumes at most 20 times per second
const float VOLUME_STEP = 0.01f;          // and only when they change at least this much
}  // namespace

SlidersScene::SlidersScene(Machine & machine, const SlidersConfig & config)
: Scene(machine),
  cfg_(config),
  position_(config.num_sliders, -1),
  start_(config.num_sliders, 0),
  target_(config.num_sliders, 0)
{
}

void SlidersScene::enter()
{
  m_.request_snapshot();  // get every slider's position while the intro line plays
  m_.play(INTRO_LINE, [this] {start_game();});
}

void SlidersScene::on_slider(int id, int value)
{
  if (id < 0 || id >= cfg_.num_sliders) {
    return;
  }
  position_[id] = value;
  if (state_ == State::WAIT_POSITIONS) {
    start_game();
  }
}

void SlidersScene::start_game()
{
  for (int p : position_) {
    if (p < 0) {
      if (state_ != State::WAIT_POSITIONS) {
        RCLCPP_WARN(m_.logger(), "Sliders: waiting for every slider's position...");
        m_.request_snapshot();
      }
      state_ = State::WAIT_POSITIONS;
      return;
    }
  }
  choose_targets();
}

void SlidersScene::choose_targets()
{
  for (int i = 0; i < cfg_.num_sliders; i++) {
    start_[i] = position_[i];
    target_[i] = pick_target(position_[i], cfg_.min_target_distance, cfg_.edge_margin, m_.rng());
    RCLCPP_INFO(
      m_.logger(), "Sliders: slider %d is at %d, target %d", i, position_[i], target_[i]);
  }
  state_ = State::PLAYING;
}

void SlidersScene::tick()
{
  if (state_ != State::PLAYING) {
    return;
  }

  if (!moved_) {
    for (int i = 0; i < cfg_.num_sliders; i++) {
      if (std::abs(position_[i] - start_[i]) > cfg_.move_threshold) {
        moved_ = true;
        fade_start_ = m_.now_sec();
        RCLCPP_INFO(m_.logger(), "Sliders: a slider moved, the drone fades in");
        break;
      }
    }
  }
  if (moved_) {
    update_drones();
  }

  if (all_on_target()) {
    if (on_target_since_ < 0.0) {
      on_target_since_ = m_.now_sec();
    } else if (m_.now_sec() - on_target_since_ >= cfg_.hold_sec) {
      finish();
    }
  } else {
    on_target_since_ = -1.0;
  }
}

void SlidersScene::update_drones()
{
  if (m_.now_sec() - last_volume_time_ < VOLUME_INTERVAL_SEC) {
    return;
  }

  // Overall progress = average closeness of the sliders to their targets.
  double progress = 0.0;
  for (int i = 0; i < cfg_.num_sliders; i++) {
    progress += closeness(position_[i], start_[i], target_[i]);
  }
  progress /= cfg_.num_sliders;

  const double fade_in = std::clamp((m_.now_sec() - fade_start_) / cfg_.fade_in_sec, 0.0, 1.0);
  const auto [no, yes] = crossfade(progress);
  const float no_volume = static_cast<float>(no * fade_in);
  const float yes_volume = static_cast<float>(yes * fade_in);

  if (std::fabs(no_volume - sent_no_) < VOLUME_STEP && std::fabs(yes_volume - sent_yes_) < VOLUME_STEP) {
    return;
  }
  m_.sound_loop(DRONE_NO, no_volume);
  m_.sound_loop(DRONE_YES, yes_volume);
  sent_no_ = no_volume;
  sent_yes_ = yes_volume;
  last_volume_time_ = m_.now_sec();
}

bool SlidersScene::all_on_target() const
{
  for (int i = 0; i < cfg_.num_sliders; i++) {
    if (std::abs(position_[i] - target_[i]) > cfg_.target_margin) {
      return false;
    }
  }
  return true;
}

void SlidersScene::finish()
{
  state_ = State::FINISHED;
  RCLCPP_INFO(m_.logger(), "Sliders: all sliders on target, self destruct cancelled");
  m_.sound_loop(DRONE_NO, 0.0f);
  m_.sound_loop(DRONE_YES, 0.0f);
  m_.matrix_text("STATUS GOOD");

  // Each clip starts when the one before it ends.
  m_.play(DING, [this] {
    m_.play(CANCELLED_LINE, [this] {
      m_.play(RETURN_LINE, [this] {
        m_.matrix_text("STATUS :(");
        m_.play(SAD_IDLE, nullptr, true);  // keeps looping into the next scene
        m_.go_to("reconciliation");
      });
    });
  });
}

}  // namespace fun_core
