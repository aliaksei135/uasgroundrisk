/*
 * PopulationMap.cpp
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#include "PopulationMap.h"
#include "../utils/GeometryProjectionUtils.h"
#include "GridMapOSMHandler.h"


using namespace ugr::util;

ugr::mapping::PopulationMap::PopulationMap(std::array<float, 4> bounds,
										   const int resolution)
	: GeospatialGridMap(bounds, resolution),
	  builder(osmium::geom::Coordinates(bounds[1], bounds[0]),
			  osmium::geom::Coordinates(bounds[3], bounds[2]))
{
	n2wHandler.ignore_errors();
}

void ugr::mapping::PopulationMap::addOSMLayer(const std::string& layerName,
											  const std::vector<OSMTag>& tags,
											  float defaultValue)
{
	add(layerName, defaultValue);
	get(layerName).setZero();
	for (const auto& tag : tags)
	{
		tagLayerMap.emplace(tag, layerName);
		densityTagMap.emplace(tag, defaultValue);
		builder.withNodeTag(OSMTag(tag)).withWayTag(OSMTag(tag));
	}
}

void ugr::mapping::PopulationMap::eval()
{
	GridMapOSMHandler handler(this, tagLayerMap, popDensityGeomMap,
							  densityTagMap);
	OSMOverpassQuery query = builder.build();
	query.makeQuery(n2wHandler, handler);

	// Sum all layers together into a single layer using the max value for each
	// cell
	constexpr auto densitySumLayerName = "Population Density";
	add(densitySumLayerName, 0);
	get(densitySumLayerName).setZero();
	for (const auto& layerName : getLayers())
	{
		// GridMap coordinate frame convention has the y axis increasing to the
		// left, which flips everything around the x (vertical) axis. Here we flip
		// it back.
		get(densitySumLayerName) =
			get(densitySumLayerName).cwiseMax(get(layerName));
	}
}
