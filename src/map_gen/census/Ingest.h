#ifndef UGR_CENSUS_INGEST_H
#define UGR_CENSUS_INGEST_H
#include <shapefil.h>
#include <geos_c.h>
#include <iostream>
#include <string>
#include <vector>
#include "../../utils/DefaultGEOSMessageHandlers.h"

class DataIngester
{
public:
    virtual std::vector<GEOSGeometry*> readFile(const std::string& file) = 0;
};

class CensusGeometryIngest final : public DataIngester
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

    std::vector<GEOSGeometry*> readFile(const std::string& file) override
    {
        std::vector<GEOSGeometry*> outGeoms;

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
            outGeoms.emplace_back(geom);

            SHPDestroyObject(obj); //dealloc
        }
        return outGeoms;
    }
};


#endif // UGR_CENSUS_INGEST_H
