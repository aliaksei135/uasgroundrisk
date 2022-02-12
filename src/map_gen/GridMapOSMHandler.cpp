/*
 * GridMapOSMHandler.cpp
 *
 *  Created by A.Pilko on 09/04/2021.
 */


#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMHandler.h"
#include "uasgroundrisk/map_gen/osm/OSMTag.h"
#include "uasgroundrisk/gridmap/GridMap.h"
#include "uasgroundrisk/gridmap/Iterators.h"
#include "../utils/GeometryProjectionUtils.h"
#include <osmium/osm/way.hpp>
#include <utility>

#include <geos_c.h>
#include <iostream>

#include "osmium/osm/area.hpp"
#include "osmium/osm/relation.hpp"

using namespace ugr::gridmap;
using namespace ugr::mapping::osm;

GridMapOSMHandler::GridMapOSMHandler(
    GeospatialGridMap* const gridMap, std::map<OSMTag, std::string> tagLayerMap,
    const std::map<GEOSGeometry*, GridMapDataType>& densityGeometryMap,
    std::map<OSMTag, GridMapDataType> densityTagMap, std::string gridCRS)
    : gridMap(gridMap), tagLayerMap(std::move(tagLayerMap)),
      densityTagMap(std::move(densityTagMap)), gridCRS(std::move(gridCRS))
{
    std::tie(reproj, projCtx) = util::makeProjObject();

    std::map<GEOSGeometry*, float> reprojectedPopulationGeomMap;
    std::transform(densityGeometryMap.cbegin(), densityGeometryMap.cend(),
                   std::inserter(reprojectedPopulationGeomMap,
                                 reprojectedPopulationGeomMap.begin()),
                   [this](std::pair<GEOSGeometry*, float> in)
               -> std::pair<GEOSGeometry*, float>
                   {
                       auto* reprojPoly = ugr::util::reprojectPolygon(
                           reproj,
                           in.first);
                       return {reprojPoly, in.second};
                   });
}

void GridMapOSMHandler::way(const osmium::Way& way) const noexcept
{
	GeoPolygon poly;
	// Add to a polygon
	for (const auto& n : way.nodes())
	{
		// Nodes are usually invalid because ways have not had node locations mapped
		// to them
		if (!n.location().valid())
		{
			// std::cerr << "Invalid location for way node; have you used "
			// 	"NodeLocationsForWays handler?"
			// 	<< std::endl;
			continue;
		}
		// PJ_COORD c = proj_trans(reproj, PJ_FWD, proj_coord(n.lat(), n.lon(), 0, 0));
		poly.emplace_back(Position(n.lon(), n.lat()));
	}

    // Check if there is any geometry defining density
    // We do this outside the loop as it does not change within the scope of the loop
    const bool emptyDensityGeom = densityGeometryMap.empty();

    // Iterate through the tags associated with the way.
    // This is usually a single relevant tag that is mapped to a grid map layer,
    // however a single geometry can appear on multiple layer
    for (const auto& tag : way.tags())
    {
        OSMTag fullTag(tag.key(), tag.value());
        // Try to find the tag in our map
        auto tagLayerIter = tagLayerMap.find(fullTag);
        if (tagLayerIter != tagLayerMap.end())
        {
            // In case the population hasn't been found within predefined geometries
            // Test if it is a uniform density by tag
            GridMapDataType fallbackDensity = 0;
            auto densityIter = densityTagMap.find(fullTag);
            if (densityIter != densityTagMap.end())
            {
                fallbackDensity = densityIter->second;
            }

            // If we find a relevant tag, we can iterate the geometry using grid map'
            // polygon iterator
            std::string layerName = tagLayerIter->second;
            for (PolygonIterator iter(*gridMap, poly); !iter.isPastEnd();
                 ++iter)
            {
                const auto gridMapPoint = (*iter);

                if (!emptyDensityGeom)
                {
                    // Each point must be converted to a GEOS geometry for the geometric
                    // predicates to work
                    auto* p = GEOSGeom_createPointFromXY(gridMapPoint.x(),
                                                         gridMapPoint.y()); // GEOS alloc

                    // Iterate through population geometries to find the which one this
                    // point is within
                    for (const auto& populationGeomPair : densityGeometryMap)
                    {
                        if (GEOSWithin(p, populationGeomPair.first) == 1)
                        {
                            // Set the grid map at this point to the population density
                            // estimate in this geometry
                            gridMap->at(layerName, gridMapPoint) = populationGeomPair.second;
                            break;
                        }
                    }
                    // Free test GEOS point
                    GEOSGeom_destroy(p); // GEOS free
                }

                //If we haven't broken out the loop by here then use the fallback density
                // Set the grid map at this point to the population density estimate
                // in this geometry
                setFallbackValue(layerName, gridMapPoint, fallbackDensity);
            }
        }
    }
}

void GridMapOSMHandler::area(const osmium::Area& area) const noexcept
{
    std::vector<OSMTag> tags;
    for (const auto& tag : area.tags())
    {
        OSMTag fullTag(tag.key(), tag.value());
        tags.emplace_back(fullTag);
        // Try to find the tag in our map
        auto tagLayerIter = tagLayerMap.find(fullTag);
        if (tagLayerIter != tagLayerMap.end())
        {
            // In case the population hasn't been found within predefined geometries
            // Test if it is a uniform density by tag
            GridMapDataType fallbackDensity = -1;
            auto densityIter = densityTagMap.find(fullTag);
            if (densityIter != densityTagMap.end())
            {
                fallbackDensity = densityIter->second;
            }
            
            for (const auto& outerRing : area.outer_rings())
            {
                std::vector<GeoPolygon> inners;
                for (const auto& innerRing : area.inner_rings(outerRing))
                {
                    GeoPolygon poly;
                    // Add to a polygon
                    for (const auto& n : innerRing)
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
                    inners.emplace_back(poly);
                }

                GeoPolygon orPoly;
                // Add to a polygon
                for (const auto& n : outerRing)
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
                    orPoly.emplace_back(Position(n.lon(), n.lat()));
                }

                std::string layerName = tagLayerIter->second;
                for (PolygonIterator iter(*gridMap, orPoly); !iter.isPastEnd();
                     ++iter)
                {
                    const auto gridMapPoint = (*iter);
                    setFallbackValue(layerName, gridMapPoint, fallbackDensity);
                }
                for (const auto irPoly : inners)
                {
                    for (PolygonIterator iter(*gridMap, irPoly); !iter.isPastEnd();
                         ++iter)
                    {
                        const auto gridMapPoint = (*iter);
                        setFallbackValue(layerName, gridMapPoint, 0);
                    }
                }
            }
        }
    }
}

void GridMapOSMHandler::setFallbackValue(const std::string& layerName, const gridmap::Index& gridMapPoint,
                                         const gridmap::Matrix::Scalar& fallbackDensity) const
{
    gridMap->at(layerName, gridMapPoint) = fallbackDensity;
}

GridMapOSMHandler::~GridMapOSMHandler()
{
    // Free PROJ objects
    proj_destroy(reproj);
    proj_context_destroy(projCtx);

    // Free all GEOS geometries
    for (const auto& pair : densityGeometryMap)
    {
        GEOSGeom_destroy(pair.first);
    }
}
