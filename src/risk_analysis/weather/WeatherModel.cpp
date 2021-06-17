/*
 * WeatherModel.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "WeatherModel.h"
#include <cassert>

WeatherModel::WeatherModel() {
  windField.add("Speed", 0);
  windField["Speed"].setZero();
  windField.add("Direction", 0);
  windField["Direction"].setZero();
}

void WeatherModel::addConstantWind(const float speed, const float direction) {
  assert(speed >= 0);
  assert(direction >= 0);
  assert(direction <= 360);
  windField["Speed"].setConstant(speed);
  windField["Direction"].setConstant(direction);
}
