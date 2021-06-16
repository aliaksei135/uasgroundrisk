/*
 * PopulationMap.cpp
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#include "PopulationMap.h"
#include "../utils/GeometryProjectionUtils.h"
#include "GridMapOSMHandler.h"

using namespace ugr::util;

ugr::mapping::PopulationMap::PopulationMap(std::array<float, 4> bounds,
                                           int resolution)
    : builder(osmium::geom::Coordinates(bounds[1], bounds[0]),
              osmium::geom::Coordinates(bounds[3], bounds[2])) {
  // Osmium expects lon, lat order, while PROJ expects lat, lon order!
  auto swProjPoint = reprojectCoordinate(bounds[0], bounds[1]);
  auto neProjPoint = reprojectCoordinate(bounds[2], bounds[3]);
  int xLength = static_cast<int>(abs(swProjPoint.xy.x - neProjPoint.xy.x));
  int yLength = static_cast<int>(abs(swProjPoint.xy.y - neProjPoint.xy.y));
  int xCentre = static_cast<int>((swProjPoint.xy.x + neProjPoint.xy.x) / 2);
  int yCentre = static_cast<int>((swProjPoint.xy.y + neProjPoint.xy.y) / 2);

  gridMap.setGeometry(grid_map::Length(xLength, yLength), resolution,
                      grid_map::Position(xCentre, yCentre));
  gridMap.setFrameId("Population Map");

  n2wHandler.ignore_errors();
}

void ugr::mapping::PopulationMap::addLayer(const std::string &layerName,
                                           const std::vector<OSMTag> &tags,
                                           float defaultValue) {
  gridMap.add(layerName, defaultValue);
  gridMap[layerName].setZero();
  for (const auto &tag : tags) {
    tagLayerMap.emplace(tag, layerName);
    densityTagMap.emplace(tag, defaultValue);
    builder.withNodeTag(OSMTag(tag)).withWayTag(OSMTag(tag));
  }
}

GridMap &ugr::mapping::PopulationMap::generateMap() {
  GridMapOSMHandler handler(&gridMap, tagLayerMap, popDensityGeomMap,
                            densityTagMap);
  OSMOverpassQuery query = builder.build();
  query.makeQuery(n2wHandler, handler);

  // Sum all layers together into a single layer using the max value for each
  // cell
  const auto densitySumLayerName = "Population Density";
  gridMap.add(densitySumLayerName, 0);
  gridMap[densitySumLayerName].setZero();
  for (const auto &layerName : gridMap.getLayers()) {
    // GridMap coordinate frame convention has the y axis increasing to the
    // left, which flips everything around the x (vertical) axis. Here we flip
    // it back.
    gridMap[densitySumLayerName] =
        gridMap[densitySumLayerName].cwiseMax(gridMap[layerName]);
  }

  return gridMap;
}
