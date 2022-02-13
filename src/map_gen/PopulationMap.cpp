/*
 * PopulationMap.cpp
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#include "uasgroundrisk/map_gen/PopulationMap.h"
#include "../utils/GeometryProjectionUtils.h"
#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMHandler.h"
#include "uasgroundrisk/map_gen/osm/OSMOverpassQuery.h"
#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMBuildingsHandler.h"

using namespace ugr::util;

ugr::mapping::PopulationMap::PopulationMap(const std::array<float, 4> bounds,
                                           const int resolution)
    : OSMMap(bounds, resolution)
{
}

void ugr::mapping::PopulationMap::addOSMLayer(const std::string& layerName, const std::vector<osm::OSMTag>& tags,
                                              float defaultValue)
{
    for (const auto& tag : tags)
    {
        densityTagMap.emplace(tag, defaultValue);
    }
    OSMMap::addOSMLayer(layerName, tags, defaultValue);
}

void ugr::mapping::PopulationMap::eval()
{
	if (isEvaluated) return;
	// Sum all layers together into a single layer using the max value for each
	// cell
	constexpr auto densitySumLayerName = "Population Density";
	add(densitySumLayerName, 0);

	osm::GridMapOSMHandler handler(this, tagLayerMap, popDensityGeomMap,
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
	isEvaluated = true;
}
