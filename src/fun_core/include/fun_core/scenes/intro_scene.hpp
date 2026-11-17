#pragma once
// docs/Script.md, "Intro":
//   Nobody around:   all lights off, big red button dimly lit, now and then an idle line
//                    ("Hello?", "ta ta ta", "tsk hmm hm hmm", *whistle*).
//   Someone nearby:  call out to them ("Hey you there!", ...), the big red button pulses
//                    brighter, some lights faintly on.
//   Someone at the machine: go to the next scene, "press_the_button".

#include <string>
#include <vector>

#include "fun_core/scene.hpp"
#include "fun_core/scene_logic.hpp"

namespace fun_core
{

struct IntroConfig
{
  int big_red_led = 0;          // LED ID of the big red button
  int prox_sensor = 0;          // distance sensor ID
  int dim_red = 30;             // big red button brightness when nobody is around (0..255)
  int pulse_red = 150;          // its peak brightness when it pulses
  int pulse_period_ms = 2000;   // one breath
  int faint_light = 15;         // strip brightness when someone is nearby (0 = stay off)
  int nearby_threshold = 150;   // distance reading for "someone nearby" (bigger = closer)
  int close_threshold = 350;    // distance reading for "someone at the machine"
  int hysteresis = 30;          // how far below a threshold a reading must fall to leave that level
  double idle_gap_min_sec = 15.0;    // pause between idle lines (random in min..max)
  double idle_gap_max_sec = 40.0;
  double callout_gap_min_sec = 5.0;  // pause between call-out lines
  double callout_gap_max_sec = 10.0;
};

class IntroScene : public Scene
{
public:
  IntroScene(Machine & machine, const IntroConfig & config);

  void enter() override;
  void tick() override;
  void on_prox(int id, int value) override;

private:
  void show_lights();            // lights for the current presence level
  void say_line();               // play a line for the current presence level
  void schedule_next_line();     // wait a random pause, then say_line()

  IntroConfig cfg_;
  Presence presence_;
  int level_ = 0;                // 0 nobody, 1 nearby, 2 at the machine
  bool speaking_ = false;        // a line is playing
  double next_line_at_ = -1.0;   // when to say the next line; < 0 = nothing planned
  size_t last_line_ = 99;        // index of the last line, to avoid repeats
};

}  // namespace fun_core
