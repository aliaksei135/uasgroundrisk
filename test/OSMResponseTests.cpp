/*
 * OSMResponseTests.cpp
 *
 *  Created by A.Pilko on 06/04/2021.
 */

#include "OSMTestHandlers.h"
#include <bitset>
#include <gtest/gtest.h>
#include <iostream>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map.hpp>
#include <osmium/index/map/sparse_mem_map.hpp>
#include <osmium/memory/buffer.hpp>

#include "../src/map_gen/osm/DefaultNodeLocationsForWaysHandler.h"
#include "../src/map_gen/osm/builder/OSMOverpassQueryBuilder.h"

using namespace osmium::memory;

class OSMResponseTests : public ::testing::Test {
protected:
  void SetUp() override {
    // Test bounds for Southampton, UK
    southWestCoords = osmium::geom::Coordinates{-1.4500237, 50.9065510};
    northEastCoords = osmium::geom::Coordinates{-1.3419628, 50.9517765};
  }
  osmium::geom::Coordinates southWestCoords;
  osmium::geom::Coordinates northEastCoords;
};

TEST_F(OSMResponseTests, RawResponseIntegrationTest) {
  OSMOverpassQueryBuilder builder =
      OSMOverpassQuery::create(southWestCoords, northEastCoords);
  OSMOverpassQuery query = builder.build();
  std::string response = query.rawResponse();

  // At least something should be returned in response
  ASSERT_NE(response, "");
}

TEST_F(OSMResponseTests, OsmiumBufferResponseTest) {
  OSMOverpassQueryBuilder builder =
      OSMOverpassQuery::create(southWestCoords, northEastCoords);
  builder.withNodeTag("amenity", "school");
  builder.withWayTag("amenity", "school");
  OSMOverpassQuery query = builder.build();
  Buffer responseBuffer = query.rawBuffer();
  int nn = 0, nw = 0, nr = 0, na = 0;
  for (auto &item : responseBuffer) {
    switch (item.type()) {
    case osmium::item_type::way:
      ++nw;
      break;
    case osmium::item_type::node:
      ++nn;
      break;
    case osmium::item_type::relation:
      ++nr;
      break;
    case osmium::item_type::area:
      ++na;
      break;
    default:
      break;
    }
  }
  //	// These are subject to change depending on external data so set as
  // equalities and hope they stay correct
  EXPECT_LE(nn, 676); // 627 on 17/5/21
  EXPECT_LE(nw, 54);  // 49 on 17/5/21
  EXPECT_LE(nr, 3);   // 3 on 17/5/21
  EXPECT_EQ(na, 0);   // this should be 0 as we did not query for any areas
}

TEST_F(OSMResponseTests, OsmiumSingleHandlerResponseTest) {
  using namespace osmium::handler;
  using namespace osmium::index::map;

  OSMOverpassQueryBuilder builder =
      OSMOverpassQuery::create(southWestCoords, northEastCoords);
  builder.withNodeTag("amenity", "school");
  builder.withWayTag("amenity", "school");
  OSMOverpassQuery query = builder.build();

  WayTestHandler wayTestHandler;
  bool handlerCalled = false;
  wayTestHandler.addExpectationFunction(
      WayTestHandler::assertionFunctionType([&handlerCalled](const Way &obj) {
        if (!handlerCalled)
          handlerCalled = true;
        return true;
      }));

  query.makeQuery(wayTestHandler);

  // Make sure the handler is passed through and lambda is called correctly
  ASSERT_TRUE(handlerCalled);
}

TEST_F(OSMResponseTests, OsmiumMultipleHandlerResponseTest) {
  using namespace osmium::handler;
  using namespace osmium::index::map;

  OSMOverpassQueryBuilder builder =
      OSMOverpassQuery::create(southWestCoords, northEastCoords);
  builder.withNodeTag("amenity", "school");
  builder.withWayTag("amenity", "school");
  OSMOverpassQuery query = builder.build();

  std::bitset<2> handlerCalled{0b00};

  NodeTestHandler nodeTestHandler;
  nodeTestHandler.addExpectationFunction(
      NodeTestHandler::assertionFunctionType([&handlerCalled](const Node &obj) {
        handlerCalled.set(0, true);
        return true;
      }));

  int failedWays = 0;
  WayTestHandler wayTestHandler;
  wayTestHandler.addExpectationFunction(WayTestHandler::assertionFunctionType(
      [&handlerCalled, &failedWays](const Way &obj) {
        for (const auto &n : obj.nodes()) {
          if (!n.location().valid()) {
            //				  std::cout << n.location() <<
            // std::endl;
            std::cout << obj.id() << std::endl;
            ++failedWays;
            break;
          }
        }
        handlerCalled.set(1, true);
        return true;
      }));

  DefaultNodeLocationsForWaysHandler n2wHandler;
  n2wHandler.ignore_errors();

  query.makeQuery(n2wHandler, nodeTestHandler, wayTestHandler);

  std::cout << "Failed Ways " << failedWays << std::endl;

  // Make sure the all handlers are passed through and lambdas are called
  // correctly
  ASSERT_TRUE(handlerCalled.all());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}