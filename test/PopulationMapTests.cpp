/*
 * PopulationMapTests
 *
 *  Created by A.Pilko on 15/06/2021.
 */

#include "uasgroundrisk/gridmap/TypeDefs.h"
#include "uasgroundrisk/map_gen/PopulationMap.h"
#include <Eigen/Dense>
#include "TestPlottingUtils.h"
#include <array>
#include <fstream>
#include <gtest/gtest.h>

#include "uasgroundrisk/map_gen/osm/OSMTag.h"


using namespace ugr::mapping;
using namespace osm;
using namespace ugr::gridmap;

class PopulationMapTests : public ::testing::Test
{
protected:
    std::array<float, 4> bounds{
        50.9065510f, -1.4500237f, 50.9517765f,
        -1.3419628f
    };
    int resolution = 20;
};

TEST_F(PopulationMapTests, EmptyMapTest)
{
    PopulationMap popMap(bounds, resolution);
    popMap.eval();

    ASSERT_EQ(popMap.getLayers().size(), 2);

    auto size = popMap.getSize();
    ASSERT_EQ(size.y(), 601);
    ASSERT_EQ(size.x(), 398);

    const Position outPos(-1.338997, 50.933289);
    ASSERT_FALSE(popMap.isInBounds(popMap.world2Local(outPos)));

    const Position inPos(-1.404258, 50.922982);
    ASSERT_TRUE(popMap.isInBounds(popMap.world2Local(inPos)));
}

TEST_F(PopulationMapTests, SingleLayerTest)
{
    PopulationMap popMap(bounds, resolution);
    popMap.addOSMLayer("Schools", {OSMTag("amenity", "school")}, 10);
    popMap.eval();

    // Inside Bitterne Park School, Southampton
    const Position testPos(-1.369220, 50.929116);

    ASSERT_EQ(popMap.getLayers().size(), 3);
    ASSERT_EQ(popMap.get("Schools").maxCoeff(), 10);
    EXPECT_EQ(popMap.atPosition("Schools", testPos), 10);

    auto size = popMap.getSize();

    const static Eigen::IOFormat CSVFormat(Eigen::FullPrecision, Eigen::DontAlignCols, ", ", "\n");

    std::ofstream file("schools_pop_mat.csv");
    if (file.is_open())
    {
        file << popMap.get("Schools").format(CSVFormat);
        file.close();
    }

	outputMat(popMap.get("Schools"), ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

TEST_F(PopulationMapTests, MultiLayerTest)
{
    PopulationMap popMap(bounds, resolution);
    popMap.addOSMLayer("Schools", {{"amenity", "school"}}, 10);
    popMap.addOSMLayer("Retail", {{"landuse", "retail"}}, 20);
    popMap.eval();

    ASSERT_EQ(popMap.getLayers().size(), 4);

    // Inside Bitterne Park School, Southampton
    const Position testSchoolPos(-1.369220, 50.929116);

    ASSERT_EQ(popMap.get("Schools").maxCoeff(), 10);
    EXPECT_EQ(popMap.atPosition("Schools", testSchoolPos), 10);

    // Portswood shops, Southampton
    const Position testRetailPos(-1.393254, 50.925358);

    ASSERT_EQ(popMap.get("Retail").maxCoeff(), 20);
    EXPECT_EQ(popMap.atPosition("Retail", testRetailPos), 20);

	outputMat(popMap.get("Population Density"), ::testing::UnitTest::GetInstance()->current_test_info()->name());

}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
