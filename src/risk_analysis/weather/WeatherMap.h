/*
 * WeatherModel.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMAP_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMAP_H_

#include "../../map_gen/GeospatialGridMap.h"
#include <grid_map_core/GridMap.hpp>

namespace ugr {
namespace risk {

class WeatherMap : public ugr::mapping::GeospatialGridMap {
public:
  WeatherMap(std::array<float, 4> bounds, int resolution);

  /**
   * Specify a constant and steady wind field
   * @param speed the speed of the wind in m/s
   * @param direction the bearing of the wind. By convention, this is the
   * bearing the wind is coming from.
   */
  void addConstantWind(float speed, float direction);

  // Not yet needed, will be required when lazy evaluating things like gribs etc
  void eval() override{};
};
} // namespace risk
} // namespace ugr

#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMAP_H_
