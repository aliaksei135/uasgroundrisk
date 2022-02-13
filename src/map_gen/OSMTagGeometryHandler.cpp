#include "uasgroundrisk/map_gen/osm/handlers/OSMTagGeometryHandler.h"

#include <osmium/osm/way.hpp>
#include <osmium/osm/area.hpp>

#include "uasgroundrisk/gridmap/TypeDefs.h"
#include "uasgroundrisk/gridmap/Iterators.h"

void ugr::mapping::osm::OSMTagGeometryHandler::way(const osmium::Way& way) noexcept
{
    gridmap::GeoPolygon poly;
    // Add to a polygon
    for (const auto& n : way.nodes())
    {
        // Nodes are usually invalid because ways have not had node locations mapped
        // to them
        if (!n.location().valid())
        {
            // std::cerr << "Invalid location for way node; have you used "
            //     "NodeLocationsForWays handler?"
            //     << std::endl;
            continue;
        }
        // PJ_COORD c = proj_trans(reproj, PJ_FWD, proj_coord(n.lat(), n.lon(), 0, 0));
        poly.emplace_back(Position(n.lon(), n.lat()));
    }

    // Iterate through the tags associated with the way.
    // This is usually a single relevant tag that is mapped to a grid map layer,
    // however a single geometry can appear on multiple layer
    for (const auto& tag : way.tags())
    {
        OSMTag fullTag(tag.key(), tag.value());
        if (std::find(tags.begin(), tags.end(), fullTag) != tags.end())
        {
            auto* geosCoordSeq = GEOSCoordSeq_create_r(geosCtx, poly.size(), 2);
            for (int i = 0; i < poly.size(); ++i)
            {
                GEOSCoordSeq_setXY_r(geosCtx, geosCoordSeq, i, poly[i].x(), poly[i].y());
            }
            auto* outerRing = GEOSGeom_createLinearRing_r(geosCtx, geosCoordSeq);
            auto* geosPoly = GEOSGeom_createPolygon_r(geosCtx, outerRing, nullptr, 0);

            tagGeometryMap[fullTag].emplace_back(geosPoly);
        }
    }
}

void ugr::mapping::osm::OSMTagGeometryHandler::area(const osmium::Area& area) noexcept
{
    for (const auto& tag : area.tags())
    {
        OSMTag fullTag(tag.key(), tag.value());
        if (std::find(tags.begin(), tags.end(), fullTag) != tags.end())
        {
            std::vector<std::vector<GEOSGeometry*>> innerRingGeomsVec;
            std::vector<GEOSGeometry*> orGeoms;

            for (const auto& outerRing : area.outer_rings())
            {
                std::vector<GEOSGeometry*> innerRingGeoms;
                for (const auto& innerRing : area.inner_rings(outerRing))
                {
                    const auto nIrCoord = innerRing.size();
                    auto* innerCS = GEOSCoordSeq_create_r(geosCtx, nIrCoord, 2);

                    for (int i = 0; i < nIrCoord; ++i)
                    {
                        const auto n = innerRing[i];

                        // Nodes are usually invalid because ways have not had node locations mapped
                        // to them
                        if (!n.location().valid())
                        {
                            // std::cerr << "Invalid location for way node; have you used "
                            //     "NodeLocationsForWays handler?"
                            //     << std::endl;
                            continue;
                        }
                        GEOSCoordSeq_setXY_r(geosCtx, innerCS, i, n.lon(), n.lat());
                    }
                    auto* irGeom = GEOSGeom_createLinearRing_r(geosCtx, innerCS);
                    innerRingGeoms.emplace_back(irGeom);
                }

                const auto nOrCoord = outerRing.size();
                auto* outerCS = GEOSCoordSeq_create_r(geosCtx, nOrCoord, 2);
                for (int i = 0; i < nOrCoord; ++i)
                {
                    const auto n = outerRing[i];
                    // Nodes are usually invalid because ways have not had node locations mapped
                    // to them
                    if (!n.location().valid())
                    {
                        // std::cerr << "Invalid location for way node; have you used "
                        //     "NodeLocationsForWays handler?"
                        //     << std::endl;
                        continue;
                    }
                    GEOSCoordSeq_setXY_r(geosCtx, outerCS, i, n.lon(), n.lat());
                }
                auto* orGeom = GEOSGeom_createLinearRing_r(geosCtx, outerCS);

                innerRingGeomsVec.emplace_back(innerRingGeoms);
                orGeoms.emplace_back(orGeom);
            }

            std::vector<GEOSGeometry*> polys(orGeoms.size());
            for (int i = 0; i < orGeoms.size(); ++i)
            {
                auto* poly = GEOSGeom_createPolygon_r(geosCtx, orGeoms[i], innerRingGeomsVec[i].data(),
                                                      innerRingGeomsVec[i].size());
                polys[i] = poly;
            }

            GEOSGeometry* finalGeom;
            if (polys.size() > 1)
            {
                finalGeom = GEOSGeom_createCollection_r(
                    geosCtx, GEOS_MULTIPOLYGON, polys.data(), polys.size());
            }
            else
            {
                if (polys.size() > 0)
                {
                    finalGeom = polys[0];
                }
                else
                {
                    break;
                }
            }
            tagGeometryMap[fullTag].emplace_back(finalGeom);
        }
    }
}
