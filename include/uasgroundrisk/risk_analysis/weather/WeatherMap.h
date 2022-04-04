/*
 * WeatherModel.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMAP_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMAP_H_

#include <array>

#include "uasgroundrisk/map_gen/GeospatialGridMap.h"

namespace ugr
{
    namespace risk
    {
        class WeatherMap final : public ugr::mapping::GeospatialGridMap
        {
        public:
            WeatherMap(std::array<float, 4> bounds, float resolution);

            /**
             * Specify a constant and steady wind field
             * @param speed the speed of the wind in m/s
             * @param direction the bearing of the wind. By convention, this is the
             * bearing the wind is coming from.
             */
            void addConstantWind(gridmap::GridMapDataType speed, gridmap::GridMapDataType direction);
        };
    } // namespace risk
} // namespace ugr

#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_WEATHER_WEATHERMAP_H_
