#ifndef UGR_CENSUS_INGEST_H
#define UGR_CENSUS_INGEST_H
#include <shapefil.h>
#include <geos_c.h>
#include <iostream>
#include <string>
#include <vector>
#include "../../utils/DefaultGEOSMessageHandlers.h"
#include "../../utils/GeometryProjectionUtils.h"
#include <csv.h>

#ifndef UGR_DATA_DIR
#define UGR_DATA_DIR "../data/"
#endif

template <typename Key, typename Value>
class DataIngester
{
public:
    virtual std::map<Key, Value> readFile(const std::string& file) = 0;
};

class CensusGeometryIngest final : public DataIngester<std::string, GEOSGeometry*>
{
public:
    CensusGeometryIngest()
    {
        initGEOS(notice, log_and_exit);
    }

    ~CensusGeometryIngest()
    {
        finishGEOS();
    }

    std::map<std::string, GEOSGeometry*> readFile(const std::string& file) override
    {
        std::map<std::string, GEOSGeometry*> outMap;

        // Remove the extension and assume the .shp, .shx and .dbf files all share a common path and name
        const auto extIdx = file.find_last_of(".");
        const auto basePath = file.substr(0, extIdx);

        // Open vertex (shp), index (shx) and attribute (dbf) files and keep the handles
        const auto shpHandle = SHPOpen(basePath.c_str(), "rb");
        const auto dbfHandle = DBFOpen(basePath.c_str(), "rb");

        // Get the number of shapes we are extracting
        int nEntities;
        SHPGetInfo(shpHandle, &nEntities, nullptr, nullptr, nullptr);

        for (int i = 0; i < nEntities; ++i)
        {
            auto* obj = SHPReadObject(shpHandle, i); //alloc
            if (obj == nullptr)
            {
                std::cerr << "SHP file header gave wrong number of entities";
                break;
            }

            GEOSGeometry* outerRing;
            std::vector<GEOSGeometry*> innerRings;

            for (int j = 0; j < obj->nParts; ++j)
            {
                const unsigned int nVert = obj->nVertices;

                // Allocate a GEOS coord sequence
                // ugr is only 2D and so is the grid map, so only 2 dimensions used, even if data has more
                auto* objGeosCoordSeq = GEOSCoordSeq_create(nVert, 2);

                const auto startVertIdx = obj->panPartStart[j];
                const double startX = obj->padfX[startVertIdx], startY = obj->padfY[startVertIdx];

                GEOSCoordSeq_setOrdinate(objGeosCoordSeq, 0, 0, startX);
                GEOSCoordSeq_setOrdinate(objGeosCoordSeq, 0, 1, startY);

                for (unsigned k = startVertIdx + 1; k < nVert; ++k)
                {
                    GEOSCoordSeq_setOrdinate(objGeosCoordSeq, k, 0, obj->padfX[k]);
                    GEOSCoordSeq_setOrdinate(objGeosCoordSeq, k, 1, obj->padfY[k]);
                }

                // Ensure a closed ring is formed
                double endX, endY;
                GEOSCoordSeq_getOrdinate(objGeosCoordSeq, nVert - 1, 0, &endX);
                GEOSCoordSeq_getOrdinate(objGeosCoordSeq, nVert - 1, 1, &endY);
                if (startX != endX || startY != endY)
                {
                    GEOSCoordSeq_setOrdinate(objGeosCoordSeq, nVert - 1, 0, startX);
                    GEOSCoordSeq_setOrdinate(objGeosCoordSeq, nVert - 1, 1, startY);
                }


                switch (obj->panPartType[j])
                {
                case SHPP_FIRSTRING:
                case SHPP_OUTERRING:
                case SHPP_RING:
                    outerRing = GEOSGeom_createLinearRing(objGeosCoordSeq);
                    break;
                case SHPP_INNERRING:
                    innerRings.emplace_back(GEOSGeom_createLinearRing(objGeosCoordSeq));
                    break;
                default:
                    break;
                }
            }

            GEOSGeometry* geom;
            switch (obj->nSHPType)
            {
            case SHPT_POINT:
            case SHPT_POINTZ:
            case SHPT_POINTM:
                {
                    auto* coordSeq = GEOSGeom_getCoordSeq(outerRing);
                    double x, y;
                    GEOSCoordSeq_getXY(coordSeq, 0, &x, &y);
                    geom = GEOSGeom_createPointFromXY(x, y);
                }
                break;
            case SHPT_POLYGON:
            case SHPT_POLYGONZ:
            case SHPT_POLYGONM:
                geom = GEOSGeom_createPolygon(outerRing, innerRings.data(), innerRings.size());
                break;
            default:
                break;
            }
            auto* code = DBFReadStringAttribute(dbfHandle, i, 3);
            outMap.emplace(code, geom);

            SHPDestroyObject(obj); //dealloc
        }
        return outMap;
    }
};

class CensusDensityIngest final : public DataIngester<std::string, double>
{
public:
    CensusDensityIngest() = default;
    ~CensusDensityIngest() = default;

    typedef ::io::CSVReader<4, ::io::trim_chars<' ', '\t'>, ::io::no_quote_escape<','>, ::io::ignore_overflow,
                            ::io::no_comment> CsvParser;

    std::map<std::string, double> readFile(const std::string& file)
    {
        std::map<std::string, double> outMap;

        CsvParser reader(file);
        reader.read_header(io::ignore_extra_column, "code", "population", "area", "density");
        std::string code;
        int population;
        double area, density;
        while (reader.read_row(code, population, area, density))
        {
            // Scale density from per hectare to /km^2
            outMap.emplace(code, density / 0.01);
        }
        return outMap;
    }
};

class CensusIngest
{
public:
    std::map<GEOSGeometry*, double> makePopulationDensityMap()
    {
        CensusGeometryIngest geomIngest;
        const auto geoms = geomIngest.readFile(std::string(UGR_DATA_DIR) + "england_wa_2011_clipped.shp");

        const auto projObjs = ugr::util::makeProjObject("EPSG:27700", "EPSG:3395");
        PJ* reproj = std::get<0>(projObjs);
        PJ_CONTEXT* projCtx = std::get<1>(projObjs);

        std::for_each(geoms.begin(), geoms.end(), [reproj](std::pair<const std::string, GEOSGeometry*> p)
        {
            auto* rg = ugr::util::reprojectPolygon(reproj, p.second);
            p.second = rg;
        });

        proj_context_destroy(projCtx);

        CensusDensityIngest densityIngest;
        const auto densityMap = densityIngest.readFile(std::string(UGR_DATA_DIR) + "density.csv");

        std::map<GEOSGeometry*, double> mergedMap;
        for (const std::pair<std::string, GEOSGeometry*> p : geoms)
        {
            if (densityMap.find(p.first) != densityMap.end())
            {
                mergedMap.emplace(p.second, densityMap.at(p.first));
            }
        }
        return mergedMap;
    }
};

#endif // UGR_CENSUS_INGEST_H
