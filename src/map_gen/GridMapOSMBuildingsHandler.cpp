#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMBuildingsHandler.h"

#include <iostream>

#include "uasgroundrisk/gridmap/TypeDefs.h"
#include "uasgroundrisk/map_gen/GeospatialGridMap.h"
#include "uasgroundrisk/gridmap/GridMap.h"
#include "uasgroundrisk/gridmap/Iterators.h"
#include "../utils/GeometryProjectionUtils.h"
#include <osmium/osm/way.hpp>

using namespace ugr::gridmap;
using namespace ugr::mapping;

GridMapOSMBuildingsHandler::GridMapOSMBuildingsHandler(ugr::mapping::GeospatialGridMap* gridMap,
                                                       const float levelHeight,
                                                       std::string gridCRS) : gridMap(gridMap),
                                                                              buildingLevelHeight(levelHeight),
                                                                              gridCRS(gridCRS)
{
	gridMap->add("Building Height", 0);
}


void GridMapOSMBuildingsHandler::way(const osmium::Way& way) const noexcept
{
	ugr::gridmap::GeoPolygon poly;
	// Add to a polygon
	for (const auto& n : way.nodes())
	{
		// Nodes are usually invalid because ways have not had node locations mapped
		// to them
		if (!n.location().valid())
		{
			std::cerr << "Invalid location for way node; have you used "
				"NodeLocationsForWays handler?"
				<< std::endl;
			continue;
		}
		poly.emplace_back(ugr::gridmap::Position(n.lon(), n.lat()));
	}

	// Initialise to a single floor, as the building must be at least a single level
	float buildingHeight = buildingLevelHeight;
	if (way.tags().has_key("building:height"))
	{
		buildingHeight = std::stof(way.tags().get_value_by_key("building:height"));
	}
	else if (way.tags().has_key("height"))
	{
		buildingHeight = std::stof(way.tags().get_value_by_key("height"));
	}
	else if (way.tags().has_key("building:levels"))
	{
		buildingHeight = std::stof(way.tags().get_value_by_key("building:levels")) * buildingLevelHeight;
	}
	else if (way.tags().has_key("levels"))
	{
		buildingHeight = std::stof(way.tags().get_value_by_key("levels")) * buildingLevelHeight;
	}

	for (PolygonIterator iter(*gridMap, poly); !iter.isPastEnd();
	     ++iter)
	{
		const auto gridMapPoint = (*iter);
		gridMap->at("Building Height", gridMapPoint) = buildingHeight;
	}
}
