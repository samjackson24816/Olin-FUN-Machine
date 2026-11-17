// Unit tests for the scene logic. Run: colcon test --packages-select fun_core

#include <gtest/gtest.h>

#include <cstdlib>
#include <random>

#include "fun_core/scene_logic.hpp"

using fun_core::closeness;
using fun_core::crossfade;
using fun_core::pick_other;
using fun_core::pick_target;
using fun_core::Presence;

TEST(Presence, GoesUpThroughTheLevels)
{
  Presence p(150, 350, 30);
  EXPECT_EQ(p.update(50), 0);
  EXPECT_EQ(p.update(150), 1);
  EXPECT_EQ(p.update(349), 1);
  EXPECT_EQ(p.update(350), 2);
}

TEST(Presence, HysteresisStopsFlicker)
{
  Presence p(150, 350, 30);
  p.update(160);                 // nearby
  EXPECT_EQ(p.update(140), 1);   // a bit below the threshold: still nearby
  EXPECT_EQ(p.update(121), 1);
  EXPECT_EQ(p.update(119), 0);   // more than 30 below: gone
  EXPECT_EQ(p.update(140), 0);   // must reach 150 again to count as nearby
}

TEST(Presence, CanJumpStraightToClose)
{
  Presence p(150, 350, 30);
  EXPECT_EQ(p.update(500), 2);
  EXPECT_EQ(p.update(200), 1);   // stepped back but still nearby
}

TEST(PickTarget, AlwaysFarEnoughAndInsideTheEdges)
{
  std::mt19937 rng(42);
  for (int current = 0; current <= 1023; current += 7) {
    for (int n = 0; n < 50; n++) {
      const int t = pick_target(current, 350, 80, rng);
      EXPECT_GE(t, 80);
      EXPECT_LE(t, 1023 - 80);
      EXPECT_GE(std::abs(t - current), 350) << "current " << current;
    }
  }
}

TEST(PickTarget, ImpossibleDistanceFallsBackToFarthestEdge)
{
  std::mt19937 rng(1);
  EXPECT_EQ(pick_target(400, 2000, 80, rng), 943);  // farther from 400 than 80 is
  EXPECT_EQ(pick_target(700, 2000, 80, rng), 80);
}

TEST(Closeness, ZeroAtStartOneOnTarget)
{
  EXPECT_DOUBLE_EQ(closeness(100, 100, 600), 0.0);
  EXPECT_DOUBLE_EQ(closeness(350, 100, 600), 0.5);
  EXPECT_DOUBLE_EQ(closeness(600, 100, 600), 1.0);
  EXPECT_DOUBLE_EQ(closeness(0, 100, 600), 0.0);     // moved the wrong way: clamped
  EXPECT_DOUBLE_EQ(closeness(650, 100, 600), 0.9);   // overshoot counts as distance too
}

TEST(Crossfade, EndsAndEqualPowerMiddle)
{
  auto [no0, yes0] = crossfade(0.0);
  EXPECT_FLOAT_EQ(no0, 1.0f);
  EXPECT_NEAR(yes0, 0.0f, 1e-6);
  auto [no1, yes1] = crossfade(1.0);
  EXPECT_NEAR(no1, 0.0f, 1e-6);
  EXPECT_FLOAT_EQ(yes1, 1.0f);
  auto [no_mid, yes_mid] = crossfade(0.5);
  EXPECT_NEAR(no_mid * no_mid + yes_mid * yes_mid, 1.0f, 1e-5);
}

TEST(PickOther, NeverRepeatsTheLastOne)
{
  std::mt19937 rng(7);
  size_t last = 0;
  for (int n = 0; n < 200; n++) {
    const size_t next = pick_other(4, last, rng);
    EXPECT_LT(next, 4u);
    EXPECT_NE(next, last);
    last = next;
  }
  EXPECT_EQ(pick_other(1, 0, rng), 0u);
}
