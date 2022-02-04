#include <algorithm>
#include <gtest/gtest.h>
#include <proj.h>
#include "../../src/map_gen/census/Ingest.h"
#include "../../src/utils/GeometryProjectionUtils.h"
#include <vector>


TEST(DataIngestTests, GeometryIngestTest)
{
    CensusGeometryIngest geomIngest;
    // Get the GEOS geoms, these are in EPSG:27700 so need reprojecting
    auto geoms = geomIngest.readFile(std::string(UGR_DATA_DIR) + "england_wa_2011_clipped.shp");

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

TEST(DataIngestTests, DensityIngestTest)
{
    CensusDensityIngest densityIngest;
    const auto densityMap = densityIngest.readFile(std::string(UGR_DATA_DIR) + "density.csv");

    ASSERT_EQ(densityMap.size(), 8570);
}

TEST(DataIngestTests, NHAPSIngestTest)
{
    CensusNHAPSIngest nhapsIngest;
    const auto nhapsProps = nhapsIngest.readFile(std::string(UGR_DATA_DIR) + "nhaps.json");

    ASSERT_EQ(nhapsProps.size(), 24);
    ASSERT_FLOAT_EQ(nhapsProps[3][0], 0.97);
}

TEST(DataIngestTests, MergedIngestTest)
{
    CensusIngest censusIngest;
    const auto out = censusIngest.makePopulationDensityMap();

    ASSERT_EQ(out.size(), 7689);
}
