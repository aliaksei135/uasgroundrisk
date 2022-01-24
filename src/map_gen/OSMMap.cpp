#include "uasgroundrisk/map_gen/OSMMap.h"
#include "osmium/geom/coordinates.hpp"


ugr::mapping::OSMMap::OSMMap(const std::array<float, 4>& bounds, const float resolution):
	GeospatialGridMap(bounds, resolution)
{
}

void ugr::mapping::OSMMap::addOSMLayer(const std::string& layerName, const std::vector<osm::OSMTag>& tags,
                                       const float defaultValue)
{
	add(layerName, defaultValue);
	for (const auto& tag : tags)
	{
		tagLayerMap.emplace(tag, layerName);
	}
	isEvaluated = false;
}
