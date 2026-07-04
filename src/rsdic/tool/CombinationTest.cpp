#include <gtest/gtest.h>
#include "Combination.hpp"

TEST(Combination, trivial){
  Combination cb(64);
  EXPECT_EQ(1, cb.Choose(0, 0));
  EXPECT_EQ(3, cb.Choose(3, 1));
  EXPECT_EQ(3, cb.Choose(3, 2));
  EXPECT_EQ(1, cb.Choose(3, 3));
  EXPECT_EQ(1, cb.Choose(4, 0));
  EXPECT_EQ(4, cb.Choose(4, 1));
  EXPECT_EQ(6, cb.Choose(4, 2));
  EXPECT_EQ(4, cb.Choose(4, 3));
  EXPECT_EQ(1, cb.Choose(4, 4));
}

