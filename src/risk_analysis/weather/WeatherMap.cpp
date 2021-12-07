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

void ugr::risk::WeatherMap::addConstantWind(const float speed,
											const float direction) const
{
	assert(speed >= 0);
	assert(direction >= 0);
	assert(direction <= 360);
	const auto velX = speed * cos(ugr::util::bearing2Angle(DEG2RAD(direction)));
	const auto velY = speed * sin(ugr::util::bearing2Angle(DEG2RAD(direction)));
	get("Wind VelX").setConstant(static_cast<const float&>(velX));
	get("Wind VelY").setConstant(static_cast<const float&>(velY));
}
