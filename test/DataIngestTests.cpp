#include <algorithm>
#include <gtest/gtest.h>
#include <proj.h>
#include "../../src/map_gen/census/Ingest.h"
#include "../../src/utils/GeometryProjectionUtils.h"
#include <vector>
#ifndef UGR_DATA_DIR
#define UGR_DATA_DIR "../data/"
#endif

TEST(DataIngestTests, CensusIngestTest)
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
}

TEST(DataIngestTests, DensityIngestTest)
{
    CensusDensityIngest densityIngest;
    const auto densityMap = densityIngest.readFile(std::string(UGR_DATA_DIR) + "density.csv");
    int i = 4;
}
