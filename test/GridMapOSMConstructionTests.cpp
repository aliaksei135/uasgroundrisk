/*
 * GridMapOSMConstructionTests.cpp
 *
 *  Created by A.Pilko on 21/04/2021.
 */

#include <grid_map_core/GridMap.hpp>
#include <gtest/gtest.h>
#include <osmium/geom/coordinates.hpp>
#include <osmium/handler.hpp>
#include <osmium/index/map.hpp>

#include <geos_c.h>

#include <matplotlibcpp.h>

#include "../src/map_gen/GridMapOSMHandler.h"
#include "../src/map_gen/osm/DefaultNodeLocationsForWaysHandler.h"
#include "../src/map_gen/osm/builder/OSMOverpassQueryBuilder.h"
#include "../src/utils/DefaultGEOSMessageHandlers.h"

namespace plt = matplotlibcpp;

class GridMapOSMConstructionTests : public ::testing::Test {
protected:
  void SetUp() override {
    // Test bounds for Southampton, UK
    southWestCoords = osmium::geom::Coordinates{-1.4500237, 50.9065510};
    northEastCoords = osmium::geom::Coordinates{-1.3419628, 50.9517765};

    n2wHandler.ignore_errors();
  }
  osmium::geom::Coordinates southWestCoords;
  osmium::geom::Coordinates northEastCoords;

  grid_map::GridMap gridMap;
  std::map<OSMTag, std::string> tagLayerMap;
  std::map<GEOSGeometry *, float> popDensityGeomMap;
  std::map<OSMTag, float> densityTagMap;

  DefaultNodeLocationsForWaysHandler n2wHandler;
};

TEST_F(GridMapOSMConstructionTests, OSMDataTest) {
  using namespace osmium::handler;
  using namespace osmium::index::map;

  // Setup an empty OSM query
  OSMOverpassQueryBuilder builder =
      OSMOverpassQuery::create(southWestCoords, northEastCoords);
  builder.withNodeTag("amenity", "school").withWayTag("amenity", "school");
  OSMOverpassQuery query = builder.build();

  gridMap.setGeometry(
      grid_map::Length(612435.55 + 90619.29, 1234954.16 + 10097.13), 60,
      grid_map::Position(308188.48, 608846.16));
  gridMap.add("Schools", 0.0);
  gridMap.setFrameId("map");
  gridMap["Schools"].setZero();

  tagLayerMap.emplace(OSMTag("amenity", "school"), "Schools");

  densityTagMap.emplace(OSMTag("amenity", "school"), 10);

  GridMapOSMHandler handler(&gridMap, tagLayerMap, popDensityGeomMap,
                            densityTagMap, "EPSG:27700");

  query.makeQuery(n2wHandler, handler);

  auto value = gridMap.at("Schools", grid_map::Index(3588, 18612));

  EXPECT_EQ(value, 10);

  auto size = gridMap.getSize();
  plt::imshow(gridMap.get("Schools").data(), size.x(), size.y(), 1);
  plt::save("gridmap_osm_test.png");
}

int main(int argc, char **argv) {
  initGEOS(notice, log_and_exit);

  ::testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();

  finishGEOS();

  return res;
}