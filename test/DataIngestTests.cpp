#include <algorithm>
#include <gtest/gtest.h>
#include <proj.h>
#include "../src/map_gen/census/Ingest.h"
#include "../src/utils/GeometryProjectionUtils.h"
#include <vector>

class DataIngestTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        geosCtx = initGEOS_r(notice, log_and_exit);
    }

    GEOSContextHandle_t geosCtx;
};

TEST_F(DataIngestTests, GeometryIngestTest)
{
    CensusGeometryIngest geomIngest(geosCtx);
    const auto geoms = geomIngest.readFile(UGR_DATA_DIR "england_wa_2011_clipped.shp");

    ASSERT_EQ(geoms.size(), 7424);
}

TEST_F(DataIngestTests, DensityIngestTest)
{
    CensusDensityIngest densityIngest(geosCtx);
    const auto densityMap = densityIngest.readFile(UGR_DATA_DIR "density.csv");

    ASSERT_EQ(densityMap.size(), 8570);
}

TEST_F(DataIngestTests, NHAPSIngestTest)
{
    CensusNHAPSIngest nhapsIngest;
    const auto nhapsProps = nhapsIngest.readFile(UGR_DATA_DIR "nhaps.json");

    ASSERT_EQ(nhapsProps.size(), 24);
    ASSERT_FLOAT_EQ(nhapsProps[3][0], 0.97);
}

TEST_F(DataIngestTests, MergedIngestTest)
{
    CensusIngest censusIngest(geosCtx);
    const auto out = censusIngest.makePopulationDensityMap<double>();

    ASSERT_EQ(out.size(), 7407);
}
