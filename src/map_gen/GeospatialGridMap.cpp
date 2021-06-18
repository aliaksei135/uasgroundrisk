/*
 * GeospatialGridMap.cpp
 *
 *  Created by A.Pilko on 18/06/2021.
 */

#include "GeospatialGridMap.h"
#include "../utils/GeometryProjectionUtils.h"
#include "grid_map_core/GridMap.hpp"

ugr::mapping::GeospatialGridMap::GeospatialGridMap(
    const std::array<float, 4> bounds, int resolution,
    const std::string &frameId) {
  setBounds(bounds, resolution);
  setFrameId(frameId);
}

void ugr::mapping::GeospatialGridMap::setBounds(
    const std::array<float, 4> boundsArr, int resolution) {
  this->bounds = boundsArr;

  auto swProjPoint = ugr::util::reprojectCoordinate(boundsArr[0], boundsArr[1]);
  auto neProjPoint = ugr::util::reprojectCoordinate(boundsArr[2], boundsArr[3]);
  int xLength = static_cast<int>(std::abs(swProjPoint.xy.x - neProjPoint.xy.x));
  int yLength = static_cast<int>(std::abs(swProjPoint.xy.y - neProjPoint.xy.y));
  int xCentre = static_cast<int>((swProjPoint.xy.x + neProjPoint.xy.x) / 2);
  int yCentre = static_cast<int>((swProjPoint.xy.y + neProjPoint.xy.y) / 2);

  setGeometry(grid_map::Length(xLength, yLength), resolution,
              grid_map::Position(xCentre, yCentre));
}
