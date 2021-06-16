/*
 * MappingTests
 *
 *  Created by A.Pilko on 11/04/2021.
 */

#include <gtest/gtest.h>
#include <matplotlibcpp.h>

namespace mpl = matplotlibcpp;

TEST(MappingTests, BasicPlotTest) {
  mpl::plot({1, 3, 2, 4});
  mpl::save("test_fig.png");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}