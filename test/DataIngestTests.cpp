#include <algorithm>
#include <gtest/gtest.h>
#include <proj.h>
#include "../../src/map_gen/census/Ingest.h"
#include "../../src/utils/GeometryProjectionUtils.h"
#include <vector>
#ifndef UGR_DATA_DIR
#define UGR_DATA_DIR "../data/england_wa_2011_clipped.shp"
#endif

TEST(DataIngestTests, CensusIngestTest)
{
    CensusGeometryIngest geomIngest;
    // Get the GEOS geoms, these are in EPSG:27700 so need reprojecting
    const auto geoms = geomIngest.readFile(UGR_DATA_DIR);

    const auto projObjs = ugr::util::makeProjObject("EPSG:27700", "EPSG:3395");
    PJ* reproj = std::get<0>(projObjs);
    PJ_CONTEXT* projCtx = std::get<1>(projObjs);

    std::vector<GEOSGeometry*> reprojPolys(geoms.size());
    std::transform(geoms.begin(), geoms.end(), reprojPolys.begin(), [reproj](const GEOSGeometry* in)-> GEOSGeometry*
    {
        return ugr::util::reprojectPolygon(reproj, in);
    });

    proj_context_destroy(projCtx);
}
