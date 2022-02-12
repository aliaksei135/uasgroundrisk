#include "uasgroundrisk/gridmap/TypeDefs.h"
#include "uasgroundrisk/map_gen/TemporalPopulationMap.h"
#include <Eigen/Dense>
#include <array>
#include <fstream>
#include <gtest/gtest.h>

#include "uasgroundrisk/map_gen/osm/OSMTag.h"


using namespace ugr::mapping;
using namespace osm;
using namespace ugr::gridmap;

class TemporalPopulationMapTests : public ::testing::Test
{
protected:
    std::array<float, 4> bounds{
        50.9065510f, -1.4500237f, 50.9517765f,
        -1.3419628f
    };

    int resolution = 20;
};

TEST_F(TemporalPopulationMapTests, GenerateMapTest)
{
    TemporalPopulationMap popMap(bounds, resolution);
    popMap.setHourOfDay(12);
    popMap.eval();

    const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

    const auto layers = popMap.getLayers();
    for (const auto& layer : layers)
    {
        std::cout << layer << " max " << std::scientific << popMap.get(layer).maxCoeff()
            << std::endl;

        std::ofstream file("tpe_" + layer + ".csv");
        if (file.is_open())
        {
            file << popMap.get(layer).format(CSVFormat);
            file.close();
        }
    }

    ASSERT_EQ(popMap.getLayers().size(), 23);

    auto size = popMap.getSize();
    ASSERT_EQ(size.y(), 601);
    ASSERT_EQ(size.x(), 398);

    const Position outPos(-1.338997, 50.933289);
    ASSERT_FALSE(popMap.isInBounds(popMap.world2Local(outPos)));

    const Position inPos(-1.404258, 50.922982);
    ASSERT_TRUE(popMap.isInBounds(popMap.world2Local(inPos)));
}

// TEST_F(TemporalPopulationMapTests, MultiLayerTest)
// {
//     TemporalPopulationMap popMap(bounds, resolution);
//     popMap.eval();
//
//     ASSERT_EQ(popMap.getLayers().size(), 3);
//
//     // Inside Bitterne Park School, Southampton
//     const Position testSchoolPos(-1.369220, 50.929116);
//
//     ASSERT_EQ(popMap.get("Schools").maxCoeff(), 10);
//     EXPECT_EQ(popMap.atPosition("Schools", testSchoolPos), 10);
//
//     // Portswood shops, Southampton
//     const Position testRetailPos(-1.393254, 50.925358);
//
//     ASSERT_EQ(popMap.get("Retail").maxCoeff(), 20);
//     EXPECT_EQ(popMap.atPosition("Retail", testRetailPos), 20);
// }

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
