/*
 * GridMapOSMHandler.cpp
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#include "GridMapOSMHandler.h"
#include "../utils/GeometryProjectionUtils.h"
#include <grid_map_core/Polygon.hpp>
#include <grid_map_core/iterators/PolygonIterator.hpp>
#include <osmium/osm/way.hpp>
#include <utility>

#include <geos_c.h>
#include <iostream>

GridMapOSMHandler::GridMapOSMHandler(
    grid_map::GridMap *const gridMap, std::map<OSMTag, std::string> tagLayerMap,
    const std::map<GEOSGeometry *, float> &densityGeometryMap,
    std::map<OSMTag, float> densityTagMap, std::string gridCRS)
    : gridMap(gridMap), tagLayerMap(std::move(tagLayerMap)),
      densityTagMap(std::move(densityTagMap)), gridCRS(std::move(gridCRS)) {
  projCtx = proj_context_create();
#ifdef PROJ_DATA_PATH
  const char *projDataPaths[1];
  projDataPaths[0] = PROJ_DATA_PATH;
  proj_context_set_search_paths(projCtx, 1, projDataPaths);
#endif
  reproj = proj_create_crs_to_crs(projCtx, "EPSG:4326", this->gridCRS.c_str(),
                                  nullptr);
  std::map<GEOSGeometry *, float> reprojectedPopulationGeomMap;
  std::transform(densityGeometryMap.cbegin(), densityGeometryMap.cend(),
                 std::inserter(reprojectedPopulationGeomMap,
                               reprojectedPopulationGeomMap.begin()),
                 [this](std::pair<GEOSGeometry *, float> in)
                     -> std::pair<GEOSGeometry *, float> {
                   auto *reprojPoly = ugr::util::reprojectPolygon(
                       reproj,
                       reinterpret_cast<const GEOSGeometry *>(in.first));
                   return {reprojPoly, in.second};
                 });
}

void GridMapOSMHandler::way(const osmium::Way &way) const noexcept {
  grid_map::Polygon poly;
  poly.setFrameId(gridMap->getFrameId());
  // Reproject nodes and add to a grid_map polygon
  for (const auto &n : way.nodes()) {
    // Nodes are usually invalid because ways have not had node locations mapped
    // to them
    if (!n.location().valid()) {
      std::cerr << "Invalid location for way node; have you used "
                   "NodeLocationsForWays handler?"
                << std::endl;
      continue;
    }
    PJ_COORD c = proj_trans(reproj, PJ_FWD, proj_coord(n.lat(), n.lon(), 0, 0));
    poly.addVertex(grid_map::Position(c.enu.e, c.enu.n));
  }

  // Iterate through the tags associated with the way.
  // This is usually a single relevant tag that is mapped to a grid map layer,
  // however a single geometry can appear on multiple layer
  for (const auto &tag : way.tags()) {
    OSMTag fullTag(tag.key(), tag.value());
    // Try to find the tag in our map
    auto tagLayerIter = tagLayerMap.find(fullTag);
    if (tagLayerIter != tagLayerMap.end()) {
      // If we find a relevant tag, we can iterate the geometry using grid map'
      // polygon iterator
      std::string layerName = tagLayerIter->second;
      grid_map::Matrix &layer = (*gridMap)[layerName];
      for (grid_map::PolygonIterator iter(*gridMap, poly); !iter.isPastEnd();
           ++iter) {
        auto gridMapPoint = (*iter);

        if (!densityGeometryMap.empty()) {
          // Each point must be converted to a GEOS geometry for the geometric
          // predicates to work
          auto *p = GEOSGeom_createPointFromXY(gridMapPoint.x(),
                                               gridMapPoint.y()); // GEOS alloc

          // Iterate through population geometries to find the which one this
          // point is within
          for (auto &populationGeomPair : densityGeometryMap) {
            if (GEOSWithin(p, populationGeomPair.first) == 1) {
              // Set the grid map at this point to the population density
              // estimate in this geometry
              gridMap->at(layerName, gridMapPoint) = populationGeomPair.second;
              break;
            }
          }
          // Free test GEOS point
          GEOSGeom_destroy(p); // GEOS free
        }

        // Population hasn't been found within predefined geometries
        // Test if it is a uniform density by tag
        auto densityIter = densityTagMap.find(fullTag);
        if (densityIter != densityTagMap.end()) {
          // Set the grid map at this point to the population density estimate
          // in this geometry
          gridMap->at(layerName, gridMapPoint) = densityIter->second;
        }
      }
    }
  }
}

GridMapOSMHandler::~GridMapOSMHandler() {
  // Free PROJ objects
  proj_destroy(reproj);
  proj_context_destroy(projCtx);

  // Free all GEOS geometries
  for (auto &pair : densityGeometryMap) {
    GEOSGeom_destroy(pair.first);
  }
}
