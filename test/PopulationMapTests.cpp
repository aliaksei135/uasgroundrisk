/*
 * PopulationMapTests
 *
 *  Created by A.Pilko on 15/06/2021.
 */

#include "../src/utils/GeometryProjectionUtils.h"
#include <../src/map_gen/PopulationMap.h>
#include <Eigen/Dense>
#include <gtest/gtest.h>
#include <matplotlibcpp.h>

using namespace ugr::mapping;
namespace plt = matplotlibcpp;

class PopulationMapTests : public ::testing::Test {
protected:
  std::array<float, 4> bounds{50.9065510f, -1.4500237f, 50.9517765f,
                              -1.3419628f};
  int resolution = 20;
};

TEST_F(PopulationMapTests, EmptyMapTest) {
  PopulationMap populationMap(bounds, resolution);
  auto popMap = populationMap.generateMap();

  ASSERT_EQ(popMap.getLayers().size(), 1);
  ASSERT_EQ(popMap.hasBasicLayers(), false);

  auto size = popMap.getSize();
  ASSERT_EQ(size.x(), 601);
  ASSERT_EQ(size.y(), 398);

  auto outCoord = ugr::util::reprojectCoordinate(50.933289, -1.338997);
  grid_map::Position outPos(outCoord.xy.x, outCoord.xy.y);
  ASSERT_FALSE(popMap.isInside(outPos));

  auto inCoord = ugr::util::reprojectCoordinate(50.922982, -1.404258);
  grid_map::Position inPos(inCoord.xy.x, inCoord.xy.y);
  ASSERT_TRUE(popMap.isInside(inPos));
}

TEST_F(PopulationMapTests, SingleLayerTest) {
  PopulationMap populationMap(bounds, resolution);
  populationMap.addLayer("Schools", {{"amenity", "school"}}, 10);
  auto popMap = populationMap.generateMap();

  // Inside Bitterne Park School, Southampton
  auto testCoord = ugr::util::reprojectCoordinate(50.929116, -1.369220);
  grid_map::Position testPos(testCoord.xy.x, testCoord.xy.y);

  ASSERT_EQ(popMap.getLayers().size(), 2);
  ASSERT_EQ(popMap.get("Schools").maxCoeff(), 10);
  EXPECT_EQ(popMap.atPosition("Schools", testPos), 10);

  auto size = popMap.getSize();
  plt::imshow(popMap.get("Schools").data(), size.y(), size.x(), 1);
  plt::save("single_layer_test.png");
}

TEST_F(PopulationMapTests, MultiLayerTest) {
  PopulationMap populationMap(bounds, resolution);
  populationMap.addLayer("Schools", {{"amenity", "school"}}, 10);
  populationMap.addLayer("Retail", {{"landuse", "retail"}}, 20);
  auto popMap = populationMap.generateMap();

  ASSERT_EQ(popMap.getLayers().size(), 3);

  // Inside Bitterne Park School, Southampton
  auto testSchoolCoord = ugr::util::reprojectCoordinate(50.929116, -1.369220);
  grid_map::Position testSchoolPos(testSchoolCoord.xy.x, testSchoolCoord.xy.y);

  ASSERT_EQ(popMap.get("Schools").maxCoeff(), 10);
  EXPECT_EQ(popMap.atPosition("Schools", testSchoolPos), 10);

  // Portswood shops, Southampton
  auto testRetailCoord = ugr::util::reprojectCoordinate(50.925358, -1.393254);
  grid_map::Position testRetailPos(testRetailCoord.xy.x, testRetailCoord.xy.y);

  ASSERT_EQ(popMap.get("Retail").maxCoeff(), 20);
  EXPECT_EQ(popMap.atPosition("Retail", testRetailPos), 20);

  auto size = popMap.getSize();
  plt::title("Schools");
  plt::imshow(popMap.get("Schools").data(), size.y(), size.x(), 1);
  plt::save("multi_layer_school_test.png");
  plt::title("Retail");
  plt::imshow(popMap.get("Retail").data(), size.y(), size.x(), 1);
  plt::save("multi_layer_retail_test.png");
  plt::title("Combined");
  plt::imshow(popMap.get("Population Density").data(), size.y(), size.x(), 1);
  plt::save("multi_layer_combined_test.png");
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}