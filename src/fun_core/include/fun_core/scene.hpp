#pragma once
// The pieces every scene uses:
//   Machine: what a scene can do (play clips, set lights, change scene). The brain node implements it.
//   Scene:   one section of docs/Script.md. The brain runs exactly one scene at a time.

#include <functional>
#include <random>
#include <string>

#include "rclcpp/logger.hpp"

namespace fun_core
{

class Machine
{
public:
  virtual ~Machine() = default;

  virtual double now_sec() = 0;  // seconds; only differences matter
  virtual std::mt19937 & rng() = 0;
  virtual rclcpp::Logger logger() = 0;

  // Play one clip (voice line + its video, or a sound effect). It replaces the clip playing now.
  // on_done runs when this clip ends, fails or is skipped. It does NOT run if a newer clip
  // replaced it, or if the scene changed in the meantime.
  virtual void play(
    const std::string & clip_id, std::function<void()> on_done = nullptr, bool loop = false) = 0;

  // Looping sound with a live volume, on top of the clip. volume 0 = stop.
  virtual void sound_loop(const std::string & clip_id, float volume) = 0;

  virtual void led_solid(int id, int r, int g, int b) = 0;
  virtual void led_pulse(int id, int r, int g, int b, int period_ms) = 0;
  virtual void leds_off() = 0;
  virtual void strip(int r, int g, int b) = 0;
  virtual void matrix_text(const std::string & text) = 0;

  // Ask the Teensy to report every input once (button, slider and distance values).
  virtual void request_snapshot() = 0;

  // Leave this scene and start another one, right after the current callback returns.
  virtual void go_to(const std::string & scene) = 0;
};

class Scene
{
public:
  explicit Scene(Machine & machine)
  : m_(machine) {}
  virtual ~Scene() = default;

  virtual void enter() = 0;                   // the scene starts
  virtual void tick() {}                      // called 50 times per second
  virtual void on_button(int /*id*/, bool /*pressed*/) {}
  virtual void on_slider(int /*id*/, int /*value*/) {}
  virtual void on_prox(int /*id*/, int /*value*/) {}

protected:
  Machine & m_;
};

}  // namespace fun_core
