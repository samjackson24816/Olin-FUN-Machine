#pragma once
// Small pure functions used by the scenes. No ROS here, so they can be unit tested
// (see test/test_scene_logic.cpp).

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include <utility>

namespace fun_core
{

// Turns raw distance-sensor readings (bigger = closer) into how present a person is:
//   0 = nobody, 1 = someone nearby, 2 = someone right at the machine.
// A level is entered at its threshold and left only `hysteresis` below it,
// so a reading that wobbles around a threshold does not flicker between levels.
class Presence
{
public:
  Presence(int nearby, int close, int hysteresis)
  : nearby_(nearby), close_(close), hysteresis_(hysteresis) {}

  int update(int value)
  {
    if (level_ < 2 && value >= close_) {
      level_ = 2;
    } else if (level_ == 2 && value < close_ - hysteresis_) {
      level_ = (value >= nearby_) ? 1 : 0;
    } else if (level_ == 0 && value >= nearby_) {
      level_ = 1;
    } else if (level_ == 1 && value < nearby_ - hysteresis_) {
      level_ = 0;
    }
    return level_;
  }

  int level() const {return level_;}

private:
  int nearby_;
  int close_;
  int hysteresis_;
  int level_ = 0;
};

// Pick a random slider target that is at least `min_distance` away from `current`
// and at least `edge_margin` away from both ends of 0..1023.
// If no such spot exists, returns the allowed end farthest from `current`.
inline int pick_target(int current, int min_distance, int edge_margin, std::mt19937 & rng)
{
  const int lo = edge_margin;
  const int hi = 1023 - edge_margin;
  // Two allowed ranges: [lo, below_hi] under the slider and [above_lo, hi] over it.
  const int below_hi = std::min(hi, current - min_distance);
  const int above_lo = std::max(lo, current + min_distance);
  const int below_len = std::max(0, below_hi - lo + 1);
  const int above_len = std::max(0, hi - above_lo + 1);

  if (below_len + above_len == 0) {
    return (std::abs(current - lo) > std::abs(current - hi)) ? lo : hi;
  }
  const int pick = std::uniform_int_distribution<int>(0, below_len + above_len - 1)(rng);
  return (pick < below_len) ? lo + pick : above_lo + (pick - below_len);
}

// How close a slider is to its target: 0.0 at its starting distance (or farther), 1.0 on target.
inline double closeness(int position, int start, int target)
{
  const double start_distance = std::max(1, std::abs(start - target));
  const double distance = std::abs(position - target);
  return std::clamp(1.0 - distance / start_distance, 0.0, 1.0);
}

struct Volumes
{
  float no;
  float yes;
};

// Equal-power crossfade. progress 0 -> {no = 1, yes = 0}; progress 1 -> {no = 0, yes = 1}.
// Equal power keeps the total loudness steady in the middle (a straight line would dip).
inline Volumes crossfade(double progress)
{
  const double p = std::clamp(progress, 0.0, 1.0);
  return {static_cast<float>(std::cos(p * M_PI / 2)), static_cast<float>(std::sin(p * M_PI / 2))};
}

// Random index in 0..n-1 that is not `last` (when n > 1), so a line is never repeated twice in a row.
inline size_t pick_other(size_t n, size_t last, std::mt19937 & rng)
{
  if (n <= 1) {
    return 0;
  }
  size_t i = std::uniform_int_distribution<size_t>(0, n - 2)(rng);
  return (last < n && i >= last) ? i + 1 : i;
}

}  // namespace fun_core
