#ifndef UGR_CENSUS_INGEST_H
#define UGR_CENSUS_INGEST_H
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <geos_c.h>
#include <proj.h>
#include "../../utils/GeometryProjectionUtils.h"
#include "../../utils/GeometryOperations.h"

#include "uasgroundrisk/map_gen/osm/OSMTag.h"

#ifndef UGR_DATA_DIR
#define UGR_DATA_DIR "../data/"
#endif


template <typename Key, typename Value>
class DataIngester
{
public:
    DataIngester(GEOSContextHandle_t& geosCtx): geosCtx(geosCtx)
    {
    };
    virtual std::map<Key, Value> readFile(const std::string& file) = 0;
protected:
    GEOSContextHandle_t& geosCtx;
};

class CensusGeometryIngest final : public DataIngester<std::string, GEOSGeometry*>
{
public:
    explicit CensusGeometryIngest(GEOSContextHandle_t& geosCtx)
        : DataIngester<std::string, GEOSGeom_t*>(geosCtx)
    {
    }

    std::map<std::string, GEOSGeometry*> readFile(const std::string& file) override;
};

class CensusDensityIngest final : public DataIngester<std::string, double>
{
public:
    explicit CensusDensityIngest(GEOSContextHandle_t& geosCtx)
        : DataIngester<std::string, double>(geosCtx)
    {
    }

    std::map<std::string, double> readFile(const std::string& file) override;
};

class CensusNHAPSIngest final
{
public:
    static const std::vector<std::vector<int>> NHAPS_GROUPING;

    static const std::vector<std::vector<ugr::mapping::osm::OSMTag>> NHAPS_OSM_MAPPING;

    std::vector<std::vector<float>> readFile(const std::string& file);
};


class CensusIngest
{
public:
    explicit CensusIngest(GEOSContextHandle_t& geosCtx)
        : geosCtx(geosCtx)
    {
    }

    template <typename Scalar>
    std::map<GEOSGeometry*, Scalar> makePopulationDensityMap()
    {
        CensusGeometryIngest geomIngest(geosCtx);
        auto geoms = geomIngest.readFile(UGR_DATA_DIR "/Wards_(December_2011)_Boundaries_EW_BGC.shp");

        const auto projObjs = ugr::util::makeProjObject("EPSG:27700", "EPSG:4326");
        PJ* reproj = std::get<0>(projObjs);
        PJ_CONTEXT* projCtx = std::get<1>(projObjs);

        std::for_each(geoms.begin(), geoms.end(), [reproj, this](std::pair<const std::string, GEOSGeometry*>& p)
        {
            auto* rg = ugr::util::reprojectPolygon_r(reproj, p.second, geosCtx);
            auto* irg = ugr::util::swapCoordOrder_r(rg, geosCtx);
            GEOSGeom_destroy_r(geosCtx, rg);
            p.second = irg;
        });

        proj_context_destroy(projCtx);

        CensusDensityIngest densityIngest(geosCtx);
        const auto densityMap = densityIngest.readFile(UGR_DATA_DIR "/density.csv");

        std::map<GEOSGeometry*, Scalar> mergedMap;
        for (const std::pair<std::string, GEOSGeometry*> p : geoms)
        {
            if(p.second == nullptr) continue;
            if (densityMap.find(p.first) != densityMap.end())
            {
                if (p.second != nullptr)
                    mergedMap.emplace(p.second, static_cast<Scalar>(densityMap.at(p.first)));
            }
        }

        return mergedMap;
    }

    std::vector<std::vector<float>> makeNHAPSProportions();
protected:
    GEOSContextHandle_t& geosCtx;
};

#endif // UGR_CENSUS_INGEST_H
