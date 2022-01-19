/*
 * OSMQLQueryTests.cpp
 *
 *  Created by A.Pilko on 25/03/2021.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <osmium/geom/coordinates.hpp>

#include "uasgroundrisk/map_gen/osm/OSMOverpassQueryBuilder.h"
#include "uasgroundrisk/map_gen/osm/OSMTag.h"

#if __unix__
#define GTEST_USES_POSIX_RE 1
#endif

class OSMQueryTests : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
		southWestCoords = osmium::geom::Coordinates{1.0, 1.0};
		northEastCoords = osmium::geom::Coordinates{5.0, 5.0};
	}

	osmium::geom::Coordinates southWestCoords;
	osmium::geom::Coordinates northEastCoords;
};

class OSMQLQueryTests : public OSMQueryTests
{
};

class OSMXMLQueryTests : public OSMQueryTests
{
};

TEST_F(OSMQLQueryTests, EmptyQueryStringTest)
{
	ugr::mapping::osm::OSMOverpassQueryBuilder builder =
		ugr::mapping::osm::OSMOverpassQuery::create(southWestCoords, northEastCoords);
	ugr::mapping::osm::OSMOverpassQuery query = builder.build();
	std::string queryString = query.buildQueryString(false);

#if GTEST_USES_POSIX_RE
	EXPECT_THAT(queryString, testing::ContainsRegex(
		            R"(\[bbox\s*:\s*([0-9]+(\.[0-9]+)?,?)+\];?)"));
	EXPECT_THAT(queryString,
	            testing::ContainsRegex(R"(\[out\s*:\s*(xml|json)\];?)"));
	EXPECT_THAT(queryString,
	            testing::ContainsRegex(R"(\[timeout\s*:\s*[0-9]+\];?)"));
#elif GTEST_USES_SIMPLE_RE
	EXPECT_THAT(queryString, testing::ContainsRegex(
		            "bbox\\s*:\\s*\\d+\\.\\d+,\\d+\\.\\d+,\\d+\\.\\d+,\\d+\\.\\d+,?.;?"));
	EXPECT_THAT(queryString,
	            testing::ContainsRegex("out\\s*:\\s*xml.;?"));
	EXPECT_THAT(queryString,
	            testing::ContainsRegex("timeout\\s*:\\s*\\d+.;?"));
#endif
	EXPECT_THAT(queryString, testing::ContainsRegex("out .+;"));

	std::stringstream ss;
	ss << "bbox\\s*:\\s*"
		<< std::to_string(southWestCoords.y) << "\\s*,\\s*"
		<< std::to_string(southWestCoords.x) << "\\s*,\\s*"
		<< std::to_string(northEastCoords.y) << "\\s*,\\s*"
		<< std::to_string(northEastCoords.x) << "\\s*.;?";
	EXPECT_THAT(queryString, testing::ContainsRegex(ss.str()));
}

TEST_F(OSMQLQueryTests, BuilderNodeOverloadTest)
{
	ugr::mapping::osm::OSMOverpassQueryBuilder builder =
		ugr::mapping::osm::OSMOverpassQuery::create(southWestCoords, northEastCoords);
	builder.withNodeTag(ugr::mapping::osm::OSMTag("key", "value"));
	std::string s1 = builder.build().buildQueryString(false);
	builder = ugr::mapping::osm::OSMOverpassQuery::create(southWestCoords, northEastCoords);
	builder.withNodeTag("key", "value");
	std::string s2 = builder.build().buildQueryString(false);

	EXPECT_EQ(s1, s2);
}

TEST_F(OSMQLQueryTests, BuilderWayOverloadTest)
{
	ugr::mapping::osm::OSMOverpassQueryBuilder builder =
		ugr::mapping::osm::OSMOverpassQuery::create(southWestCoords, northEastCoords);
	builder.withWayTag(ugr::mapping::osm::OSMTag("key", "value"));
	std::string s1 = builder.build().buildQueryString(false);
	builder = ugr::mapping::osm::OSMOverpassQuery::create(southWestCoords, northEastCoords);
	builder.withWayTag("key", "value");
	std::string s2 = builder.build().buildQueryString(false);

	EXPECT_EQ(s1, s2);
}

TEST_F(OSMQLQueryTests, BuilderRelationOverloadTest)
{
	ugr::mapping::osm::OSMOverpassQueryBuilder builder =
		ugr::mapping::osm::OSMOverpassQuery::create(southWestCoords, northEastCoords);
	builder.withRelationTag(ugr::mapping::osm::OSMTag("key", "value"));
	std::string s1 = builder.build().buildQueryString(false);
	builder = ugr::mapping::osm::OSMOverpassQuery::create(southWestCoords, northEastCoords);
	builder.withRelationTag("key", "value");
	std::string s2 = builder.build().buildQueryString(false);

	EXPECT_EQ(s1, s2);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
