#ifndef UGR_GRIDMAPOSMTEMPORALHANDLER_H
#define UGR_GRIDMAPOSMTEMPORALHANDLER_H

#include <osmium/handler.hpp>
#include <geos_c.h>
#include "uasgroundrisk/map_gen/osm/OSMTag.h"
#include <map>
#include <vector>

namespace ugr
{
    namespace mapping
    {
        namespace osm
        {
            class OSMTagGeometryHandler : public osmium::handler::Handler
            {
            public:
                OSMTagGeometryHandler(const std::vector<OSMTag>& tags,
                                      std::map<OSMTag, std::vector<GEOSGeometry*>>& tagGeometryMap,
                                      const GEOSContextHandle_t& geosCtx): tags(tags),
                                                                           tagGeometryMap(tagGeometryMap),
                                                                           geosCtx(geosCtx)
                {
                }

                void way(const osmium::Way& way) noexcept;

                void area(const osmium::Area& area) noexcept;

            protected:
                const std::vector<OSMTag>& tags;
                std::map<OSMTag, std::vector<GEOSGeometry*>>& tagGeometryMap;
                const GEOSContextHandle_t& geosCtx;
            };
        }
    }
}
#endif // UGR_GRIDMAPOSMTEMPORALHANDLER_H
