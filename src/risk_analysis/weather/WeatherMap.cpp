/*
 * WeatherModel.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "WeatherMap.h"
#include "../../utils/VectorOperations.h"
#include <cassert>
#include <cmath>

ugr::risk::WeatherMap::WeatherMap(const std::array<float, 4> bounds, const float resolution)
	: GeospatialGridMap(bounds, resolution)
{
	add("Wind VelX", 0);
	get("Wind VelX").setZero();
	add("Wind VelY", 0);
	get("Wind VelY").setZero();
}

void ugr::risk::WeatherMap::addConstantWind(const gridmap::GridMapDataType speed,
                                            const gridmap::GridMapDataType direction)
{
	if (speed < 0) throw std::out_of_range("Wind speed must be positive");
	if (direction < 0 || direction > 360)
		throw std::out_of_range(
			"Wind direction must be between 0 and 360 degrees inclusive");
	const gridmap::GridMapDataType velX = -speed * sin(ugr::util::bearing2Angle(DEG2RAD(direction)));
	const gridmap::GridMapDataType velY = -speed * cos(ugr::util::bearing2Angle(DEG2RAD(direction)));
	get("Wind VelX").setConstant(velX);
	get("Wind VelY").setConstant(velY);
}
