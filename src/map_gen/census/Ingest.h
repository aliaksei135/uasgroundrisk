#ifndef UGR_CENSUS_INGEST_H
#define UGR_CENSUS_INGEST_H
#include <string>
#include <vector>
#include <map>

#include "uasgroundrisk/map_gen/osm/OSMTag.h"

#ifndef UGR_DATA_DIR
#define UGR_DATA_DIR "../data/"
#endif

class GEOSGeometry;
class GEOSGeom_t;

template <typename Key, typename Value>
class DataIngester
{
public:
    virtual std::map<Key, Value> readFile(const std::string& file) = 0;
};

class CensusGeometryIngest final : public DataIngester<std::string, GEOSGeom_t*>
{
public:
    CensusGeometryIngest();

    ~CensusGeometryIngest();

    std::map<std::string, GEOSGeom_t*> readFile(const std::string& file) override;
};

class CensusDensityIngest final : public DataIngester<std::string, double>
{
public:
    CensusDensityIngest() = default;
    ~CensusDensityIngest() = default;

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
    template <typename Scalar>
    std::map<GEOSGeom_t*, Scalar> makePopulationDensityMap();

    std::vector<std::vector<float>> makeNHAPSProportions();
};

#endif // UGR_CENSUS_INGEST_H
