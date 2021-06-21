/*
 * VectorOpsTests
 *
 *  Created by A.Pilko on 18/06/2021.
 */

#include "../src/utils/VectorOperations.h"
#include <cmath>
#include <gtest/gtest.h>

using namespace ugr::util;

class VectorOpsTests : public ::testing::Test {
protected:
  double theta = DEG2RAD(45);
  double resVal = sqrt(2) / 2;
};

TEST_F(VectorOpsTests, FirstQuadRotationTest) {
  Eigen::Vector2d vect(1, 0);
  Eigen::Vector2d out = rotate2D(vect, theta);
  Eigen::Vector2d expected(resVal, resVal);
  ASSERT_NEAR(out(0), expected(0), 0.001);
  ASSERT_NEAR(out(1), expected(1), 0.001);
}
TEST_F(VectorOpsTests, SecondQuadRotationTest) {
  Eigen::Vector2d vect(0, 1);
  Eigen::Vector2d out = rotate2D(vect, theta);
  Eigen::Vector2d expected(-resVal, resVal);
  ASSERT_NEAR(out(0), expected(0), 0.001);
  ASSERT_NEAR(out(1), expected(1), 0.001);
}
TEST_F(VectorOpsTests, ThirdQuadRotationTest) {
  Eigen::Vector2d vect(-1, 0);
  Eigen::Vector2d out = rotate2D(vect, theta);
  Eigen::Vector2d expected(-resVal, -resVal);
  ASSERT_NEAR(out(0), expected(0), 0.001);
  ASSERT_NEAR(out(1), expected(1), 0.001);
}
TEST_F(VectorOpsTests, FourthQuadRotationTest) {
  Eigen::Vector2d vect(0, -1);
  Eigen::Vector2d out = rotate2D(vect, theta);
  Eigen::Vector2d expected(resVal, -resVal);
  ASSERT_NEAR(out(0), expected(0), 0.001);
  ASSERT_NEAR(out(1), expected(1), 0.001);
}

TEST(AngleTests, Bearing2AngleTest) {
  ASSERT_EQ(bearing2Angle(DEG2RAD(0)), DEG2RAD(90));
  ASSERT_EQ(bearing2Angle(DEG2RAD(90)), DEG2RAD(0));
  ASSERT_EQ(bearing2Angle(DEG2RAD(180)), DEG2RAD(270));
  ASSERT_EQ(bearing2Angle(DEG2RAD(270)), DEG2RAD(180));
  ASSERT_EQ(bearing2Angle(DEG2RAD(360)), DEG2RAD(90));
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}