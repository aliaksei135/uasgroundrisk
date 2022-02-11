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
    // Get the GEOS geoms, these are in EPSG:27700 so need reprojecting
    auto geoms = geomIngest.readFile(UGR_DATA_DIR "england_wa_2011_clipped.shp");

    const auto projObjs = ugr::util::makeProjObject("EPSG:27700", "EPSG:3395");
    PJ* reproj = std::get<0>(projObjs);
    PJ_CONTEXT* projCtx = std::get<1>(projObjs);

    std::for_each(geoms.begin(), geoms.end(), [reproj](std::pair<const std::string, GEOSGeometry*> p)
    {
        auto* rg = ugr::util::reprojectPolygon(reproj, p.second);
        p.second = rg;
    });

    proj_context_destroy(projCtx);

    ASSERT_EQ(geoms.size(), 7707);
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

    ASSERT_EQ(out.size(), 7689);
}
