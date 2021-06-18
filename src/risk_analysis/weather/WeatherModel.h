/*
 * WeatherModel.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMODEL_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMODEL_H_

#include <grid_map_core/GridMap.hpp>

namespace ugr {
namespace risk {

class WeatherModel {
public:
  WeatherModel();

  /**
   * Specify a constant and steady wind field
   * @param speed the speed of the wind in m/s
   * @param direction the bearing of the wind. By convention, this is the
   * bearing the wind is coming from.
   */
  void addConstantWind(const float speed, const float direction);

protected:
  grid_map::GridMap windField; /// A GridMap describing wind containing exactly
                               /// 2 layers: "Wind Speed" and "Wind Direction"
};
} // namespace risk
} // namespace ugr

#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMODEL_H_
