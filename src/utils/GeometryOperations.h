#ifndef GEOMETRYOPERATIONS_H
#define GEOMETRYOPERATIONS_H
#include "uasgroundrisk/gridmap/TypeDefs.h"
#include <array>
#include <map>
#include <geos_c.h>
#include "DefaultGEOSMessageHandlers.h"

namespace ugr
{
    namespace util
    {
        template <typename T>
        static std::array<T, 4> getPolygonBounds(const gridmap::Polygon& polygon)
        {
            T minX = std::numeric_limits<T>::max();
            T maxX = std::numeric_limits<T>::min();
            T minY = std::numeric_limits<T>::max();
            T maxY = std::numeric_limits<T>::min();

            for (const auto& pos : polygon)
            {
                const auto x = pos[0];
                const auto y = pos[1];

                if (x > maxX)
                {
                    maxX = x;
                }
                if (x < minX)
                {
                    minX = x;
                }

                if (y > maxY)
                {
                    maxY = y;
                }
                if (y < minY)
                {
                    minY = y;
                }
            }

            return {minY, minX, maxY, maxX};
        }

        template <typename T>
        static bool isInsidePolygon(const gridmap::Polygon& polygon, const T& position)
        {
            unsigned cross = 0;
            for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
            {
                if (polygon[i].y() > position.y() != polygon[j].y() > position.y()
                    && position.x() < (polygon[j].x() - polygon[i].x()) * (position.y() - polygon[i].y()) /
                    (polygon[j].y() - polygon[i].y()) + polygon[i].x())

                    cross++;
            }
            return cross % 2;
        }

        template <typename T>
        static std::map<GEOSGeometry*, T> boundGeometriesMap(std::map<GEOSGeometry*, T>& geomMap,
                                                             const std::array<float, 4>& bounds)
        {
            initGEOS(notice, log_and_exit);

            // Create bounding box as a GEOS Geometry
            auto* boundingCoordSeq = GEOSCoordSeq_create(5, 2); //alloc
            GEOSCoordSeq_setXY(boundingCoordSeq, 0, bounds[1], bounds[0]); //SW corner
            GEOSCoordSeq_setXY(boundingCoordSeq, 1, bounds[1], bounds[2]); //NW corner
            GEOSCoordSeq_setXY(boundingCoordSeq, 2, bounds[3], bounds[2]); //NE corner
            GEOSCoordSeq_setXY(boundingCoordSeq, 3, bounds[3], bounds[0]); //SE corner
            GEOSCoordSeq_setXY(boundingCoordSeq, 4, bounds[1], bounds[0]); //SW corner to close ring
            auto* boundingGeom = GEOSGeom_createLinearRing(boundingCoordSeq);

            std::map<GEOSGeometry*, T> outMap;

            for (auto& pair : geomMap)
            {
                auto* inGeom = pair.first;
                auto* intersection = GEOSIntersection(boundingGeom, inGeom);
                if (intersection != nullptr)
                {
                    outMap.emplace(intersection, pair.second);
                }

                // This is either intersected in which case a new geometry is produced,
                // or it out of bounds in which case we don't want it anyway
                GEOSGeom_destroy(inGeom);
            }

            finishGEOS();
        }

        template <typename C, typename T = typename C::value_type>
        static void destroyGEOSGeoms(C const& container)
        {
            for (GEOSGeometry* geom : container)
            {
                GEOSGeom_destroy(geom);
            }
        }
    }
}
#endif // GEOMETRYOPERATIONS_H
