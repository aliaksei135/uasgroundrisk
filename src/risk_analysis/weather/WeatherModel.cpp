/*
 * WeatherModel.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "WeatherModel.h"
#include <cassert>

ugr::risk::WeatherModel::WeatherModel() {
  windField.add("Wind VelX", 0);
  windField["Wind VelX"].setZero();
  windField.add("Wind VelY", 0);
  windField["Wind VelY"].setZero();
}

void ugr::risk::WeatherModel::addConstantWind(const float speed,
                                              const float direction) {
  assert(speed >= 0);
  assert(direction >= 0);
  assert(direction <= 360);
  auto velX = speed * cos(direction);
  auto velY = speed * sin(direction);
  windField["Wind VelX"].setConstant(static_cast<const float &>(velX));
  windField["Wind VelY"].setConstant(static_cast<const float &>(velY));
}
