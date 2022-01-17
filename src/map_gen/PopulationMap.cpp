/*
 * PopulationMap.cpp
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#include "uasgroundrisk/map_gen/PopulationMap.h"
#include "../utils/GeometryProjectionUtils.h"
#include "GridMapOSMHandler.h"


using namespace ugr::util;

ugr::mapping::PopulationMap::PopulationMap(std::array<float, 4> bounds,
										   const int resolution)
	: OSMMap(bounds, resolution)
{
}


void ugr::mapping::PopulationMap::eval()
{
	// Sum all layers together into a single layer using the max value for each
	// cell
	constexpr auto densitySumLayerName = "Population Density";
	add(densitySumLayerName, 0);
	get(densitySumLayerName).setZero();

	if (popDensityGeomMap.empty() && densityTagMap.empty())
	{
		// No point evaluating an empty population map
		// Ensure the empty population density layer is created anyway
		// in case it is evaluated
		return;
	}
	GridMapOSMHandler handler(this, tagLayerMap, popDensityGeomMap,
							  densityTagMap);
	OSMMap::eval(handler);

	for (const auto& layerName : getLayers())
	{
		// GridMap coordinate frame convention has the y axis increasing to the
		// left, which flips everything around the x (vertical) axis. Here we flip
		// it back.
		get(densitySumLayerName) =
			get(densitySumLayerName).cwiseMax(get(layerName));
	}
}
