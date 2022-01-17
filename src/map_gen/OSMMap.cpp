#include "uasgroundrisk/map_gen/OSMMap.h"

ugr::mapping::OSMMap::OSMMap(const std::array<float, 4>& bounds, const float resolution):
	GeospatialGridMap(bounds, resolution), builder(osmium::geom::Coordinates(bounds[1], bounds[0]),
												   osmium::geom::Coordinates(bounds[3], bounds[2]))
{
	n2wHandler.ignore_errors();
}

void ugr::mapping::OSMMap::addOSMLayer(const std::string& layerName, const std::vector<OSMTag>& tags,
									   const float defaultValue)
{
	add(layerName, defaultValue);
	get(layerName).setZero();
	for (const auto& tag : tags)
	{
		tagLayerMap.emplace(tag, layerName);
		builder.withNodeTag(OSMTag(tag)).withWayTag(OSMTag(tag));
	}
}

template <typename... THandlers>
void ugr::mapping::OSMMap::eval(THandlers&&...handlers)
{
	if (tagLayerMap.empty())
	{
		// No point evaluating an empty population map
		// Ensure the empty population density layer is created anyway
		// in case it is evaluated
		return;
	}

	OSMOverpassQuery query = builder.build();
	query.makeQuery(n2wHandler, handlers);
}
