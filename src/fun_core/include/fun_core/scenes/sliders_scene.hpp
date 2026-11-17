#pragma once
// docs/Script.md, "Sliders":
//   1. Voice line + video: "You know you haven't really been all that helpful..." (character walks off).
//   2. Each of the 3 sliders gets a random target, somewhat far from where it is now.
//   3. As soon as a slider moves, a droning "noooo" fades in. As the sliders get closer to
//      their targets, it crossfades into a droning "yeeessss".
//   4. All 3 sliders on target (plus or minus a margin): ding, "Self Destruct Sequence Cancelled",
//      LED matrix shows "STATUS GOOD".
//   5. Character comes back: "I think I figured out how to disab- oh..." Then the LED matrix shows
//      "STATUS :(", a sad idle animation loops, and the next scene is "reconciliation".

#include <string>
#include <vector>

#include "fun_core/scene.hpp"
#include "fun_core/scene_logic.hpp"

namespace fun_core
{

struct SlidersConfig
{
  int num_sliders = 3;
  int min_target_distance = 350;  // a target is at least this far from the slider's start (0..1023)
  int edge_margin = 80;           // targets stay this far from both ends
  int move_threshold = 20;        // a slider counts as "moved" after this much
  int target_margin = 35;         // "on target" = within this much of the target
  double hold_sec = 0.5;          // all sliders must stay on target this long
  double fade_in_sec = 1.5;       // how long the drone takes to fade in after the first move
};

class SlidersScene : public Scene
{
public:
  SlidersScene(Machine & machine, const SlidersConfig & config);

  void enter() override;
  void tick() override;
  void on_slider(int id, int value) override;

private:
  enum class State { INTRO_LINE, WAIT_POSITIONS, PLAYING, FINISHED };

  void start_game();       // after the intro line
  void choose_targets();   // needs every slider's position
  void update_drones();    // set the noooo / yeesss volumes
  bool all_on_target() const;
  void finish();           // the ending: ding, lines, matrix text, next scene

  SlidersConfig cfg_;
  State state_ = State::INTRO_LINE;
  std::vector<int> position_;   // latest reading per slider, -1 = not known yet
  std::vector<int> start_;      // position when the targets were chosen
  std::vector<int> target_;
  bool moved_ = false;          // has any slider moved since the targets were chosen?
  double fade_start_ = 0.0;
  double on_target_since_ = -1.0;  // < 0 = not all on target right now
  double last_volume_time_ = -1.0;
  float sent_no_ = -1.0f;       // last volumes sent, to send only real changes
  float sent_yes_ = -1.0f;
};

}  // namespace fun_core
