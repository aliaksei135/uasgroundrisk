#include "uasgroundrisk/gridmap/TypeDefs.h"
#include "uasgroundrisk/map_gen/OSMMap.h"
#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMBuildingsHandler.h"
#include "uasgroundrisk/map_gen/osm/OSMTag.h"
#include <Eigen/Dense>
// #include <matplotlibcpp.h>
#include <array>
#include <fstream>
#include <gtest/gtest.h>

#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMBuildingsHandler.h"


using namespace ugr::mapping;
using namespace ugr::gridmap;
using namespace ugr::mapping::osm;
// namespace plt = matplotlibcpp;

class BuildingHeightMapTests : public ::testing::Test
{
protected:
	std::array<float, 4> bounds{
		50.9065510f, -1.4500237f, 50.9517765f,
		-1.3419628f
	};
	int resolution = 20;
};

TEST_F(BuildingHeightMapTests, SmallMapTest)
{
	OSMMap gridMap(bounds, resolution);
	gridMap.addOSMLayer("Building Height", {OSMTag("building")});

	GridMapOSMBuildingsHandler handler(&gridMap);
	gridMap.eval(handler);

	ASSERT_EQ(gridMap.getLayers().size(), 1);

	auto size = gridMap.getSize();
	ASSERT_EQ(size.y(), 601);
	ASSERT_EQ(size.x(), 398);

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	std::cout << "Building Height max " << std::scientific << gridMap.get("Building Height").maxCoeff()
		<< std::endl;

	std::ofstream file("Building_Heights.csv");
	if (file.is_open())
	{
		file << gridMap.get("Building Height").format(CSVFormat);
		file.close();
	}

	// Position of the Centenary Building on University of Southampton Highfield campus.
	// Should be 8 levels
	// therefore height should be 3.048 * 8 = 24.384
	const Position centenaryBuildingPos(-1.397588, 50.936920);
	const auto height = gridMap.atPosition("Building Height", centenaryBuildingPos);
	ASSERT_FLOAT_EQ(height, 24.384f);

	// const Position outPos(-1.338997, 50.933289);
	// ASSERT_FALSE(popMap.isInBounds(popMap.world2Local(outPos)));
	//
	// const Position inPos(-1.404258, 50.922982);
	// ASSERT_TRUE(popMap.isInBounds(popMap.world2Local(inPos)));
}