/*
 * WeatherModel.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "WeatherMap.h"
#include <cassert>
#include <cmath>

ugr::risk::WeatherMap::WeatherMap(std::array<float, 4> bounds, int resolution)
    : GeospatialGridMap(bounds, resolution, "Weather Map") {
  add("Wind VelX", 0);
  get("Wind VelX").setZero();
  add("Wind VelY", 0);
  get("Wind VelY").setZero();
}

void ugr::risk::WeatherMap::addConstantWind(const float speed,
                                            const float direction) {
  assert(speed >= 0);
  assert(direction >= 0);
  assert(direction <= 360);
  auto velX = speed * cos(direction);
  auto velY = speed * sin(direction);
  get("Wind VelX").setConstant(static_cast<const float &>(velX));
  get("Wind VelY").setConstant(static_cast<const float &>(velY));
}
