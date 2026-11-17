#include "fun_core/scenes/intro_scene.hpp"

#include "rclcpp/logging.hpp"

namespace fun_core
{

namespace
{
// Clip IDs (voice line + video). The clip player maps each ID to its files.
const std::vector<std::string> IDLE_LINES = {
  "intro_hello",         // "Hello?"
  "intro_ta_ta_ta",      // "ta ta ta"
  "intro_tsk_hmm",       // "tsk hmm hm hmm"
  "intro_whistle",       // *whistle*
};
const std::vector<std::string> CALLOUT_LINES = {
  "intro_hey_you_there",     // "Hey you there!"
  "intro_person_detected",   // "Person... thing... detected movement"
  "intro_help_me",           // "Help me"
};
}  // namespace

IntroScene::IntroScene(Machine & machine, const IntroConfig & config)
: Scene(machine),
  cfg_(config),
  presence_(config.nearby_threshold, config.close_threshold, config.hysteresis)
{
}

void IntroScene::enter()
{
  m_.leds_off();
  show_lights();
  schedule_next_line();
  m_.request_snapshot();  // get the distance reading now instead of waiting for it to change
}

void IntroScene::tick()
{
  if (!speaking_ && next_line_at_ >= 0.0 && m_.now_sec() >= next_line_at_) {
    say_line();
  }
}

void IntroScene::on_prox(int id, int value)
{
  if (id != cfg_.prox_sensor) {
    return;
  }
  const int level = presence_.update(value);
  if (level == level_) {
    return;
  }
  const int old_level = level_;
  level_ = level;

  if (level == 2) {
    RCLCPP_INFO(m_.logger(), "Intro: someone is at the machine (reading %d)", value);
    m_.go_to("press_the_button");
    return;
  }

  show_lights();
  if (level == 1 && old_level == 0) {
    RCLCPP_INFO(m_.logger(), "Intro: someone nearby (reading %d), calling out", value);
    say_line();  // call out right away, even over an idle line
  } else if (level == 0) {
    RCLCPP_INFO(m_.logger(), "Intro: nobody nearby any more (reading %d)", value);
    if (!speaking_) {
      schedule_next_line();  // a call-out still playing schedules the next line itself
    }
  }
}

void IntroScene::show_lights()
{
  if (level_ == 0) {
    m_.led_solid(cfg_.big_red_led, cfg_.dim_red, 0, 0);
    m_.strip(0, 0, 0);
  } else {
    m_.led_pulse(cfg_.big_red_led, cfg_.pulse_red, 0, 0, cfg_.pulse_period_ms);
    m_.strip(cfg_.faint_light, cfg_.faint_light, cfg_.faint_light);
  }
}

void IntroScene::say_line()
{
  const std::vector<std::string> & lines = (level_ == 0) ? IDLE_LINES : CALLOUT_LINES;
  last_line_ = pick_other(lines.size(), last_line_, m_.rng());
  speaking_ = true;
  next_line_at_ = -1.0;
  m_.play(lines[last_line_], [this] {
    speaking_ = false;
    schedule_next_line();
  });
}

void IntroScene::schedule_next_line()
{
  const double lo = (level_ == 0) ? cfg_.idle_gap_min_sec : cfg_.callout_gap_min_sec;
  const double hi = (level_ == 0) ? cfg_.idle_gap_max_sec : cfg_.callout_gap_max_sec;
  const double gap = std::uniform_real_distribution<double>(lo, hi)(m_.rng());
  next_line_at_ = m_.now_sec() + gap;
}

}  // namespace fun_core
