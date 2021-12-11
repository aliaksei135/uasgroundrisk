/*
 * PopulationMap.h
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_
#define UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_

#include "GeospatialGridMap.h"
#include "osm/DefaultNodeLocationsForWaysHandler.h"
#include "osm/builder/OSMOverpassQueryBuilder.h"
#include <geos_c.h>
#include <map>

namespace ugr
{
	namespace mapping
	{
		class PopulationMap final : public GeospatialGridMap
		{
		public:
			/**
			 * Construct a static Population Map
			 * @param bounds the [South, West, North, East] bounds in EPSG4326 coordinates
			 * @param resolution the resolution of a single cell in metres
			 */
			PopulationMap(std::array<float, 4> bounds, int resolution);

			void addOSMLayer(const std::string& layerName,
			                 const std::vector<OSMTag>& tags, float defaultValue = 0);

			void eval();

		protected:
			std::map<OSMTag, std::string> tagLayerMap;
			std::map<GEOSGeometry*, GridMapDataType> popDensityGeomMap;
			std::map<OSMTag, GridMapDataType> densityTagMap;

			OSMOverpassQueryBuilder builder;
			DefaultNodeLocationsForWaysHandler n2wHandler;
		};
	} // namespace mapping
} // namespace ugr

#endif // UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_
