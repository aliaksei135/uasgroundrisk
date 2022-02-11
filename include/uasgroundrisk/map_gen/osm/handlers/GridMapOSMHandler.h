/*
 * GridMapOSMHandler.h
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_GRIDMAPOSMHANDLER_H_
#define UASGROUNDRISK_SRC_MAP_GEN_GRIDMAPOSMHANDLER_H_

#include <geos_c.h>
#include "uasgroundrisk/gridmap/GridMap.h"
#include <map>
#include <osmium/handler.hpp>
#include <proj.h>

#include "uasgroundrisk/map_gen/GeospatialGridMap.h"

namespace ugr
{
    namespace mapping
    {
        namespace osm
        {
            struct OSMTag;
            using namespace ugr::gridmap;
            using namespace ugr::mapping;

            class GridMapOSMHandler : public osmium::handler::Handler
            {
            public:
                /**
                 * \brief Construct an osmium handler that write geometries to a gridmap.
                 *
                 * \param gridMap the gridmap to write to
                 * \param tagLayerMap a map of OSM tags to gridmap layer names.
                 * 					  Tags can only map to a single layer name,
                 * but multiple tags can map to a single layer name.
                 * \param densityGeometryMap a map of GEOS polygons to population values.
                 * Geometries must be in EPSG:4326 projection. This class assumes
                 * responsibility for freeing the GEOS objects here.
                 * \param densityTagMap a map of OSM tags to uniform densities in
                 * correspondingly tagged areas. densityGeometryMap takes precedence over this
                 * in setting the grid map value.
                 * \param gridCRS the grid coordinate reference system with units in metres
                 */
                GridMapOSMHandler(GeospatialGridMap* gridMap,
                                  std::map<OSMTag, std::string> tagLayerMap,
                                  const std::map<GEOSGeometry*, GridMapDataType>& densityGeometryMap,
                                  std::map<OSMTag, GridMapDataType> densityTagMap,
                                  std::string gridCRS = "EPSG:3395");
                ~GridMapOSMHandler();

                void way(const osmium::Way& way) const noexcept;

                void area(const osmium::Area& area) const noexcept;

            protected:
                PJ_CONTEXT* projCtx;
                PJ* reproj;

                GeospatialGridMap* gridMap;
                std::map<OSMTag, std::string> tagLayerMap;
                std::map<OSMTag, GridMapDataType> densityTagMap;
                std::map<GEOSGeometry*, GridMapDataType> densityGeometryMap;

                std::string gridCRS;

                void setFallbackValue(const std::string& layerName, const Index& gridMapPoint,
                                      const Matrix::Scalar& fallbackDensity) const;
            };
        }
    }
}
#endif // UASGROUNDRISK_SRC_MAP_GEN_GRIDMAPOSMHANDLER_H_
