/*
 * PopulationMap.h
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_
#define UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_

#include "uasgroundrisk/map_gen/OSMMap.h"
#include "uasgroundrisk/map_gen/osm/OSMTag.h"
#include <geos_c.h>
#include <map>
#include <string>

namespace ugr
{
	namespace mapping
	{
		class PopulationMap final : public OSMMap
		{
		public:
			/**
			 * Construct a static Population Map
			 * @param bounds the [South, West, North, East] bounds in EPSG4326 coordinates
			 * @param resolution the resolution of a single cell in metres
			 */
			PopulationMap(std::array<float, 4> bounds, int resolution);

			void ugr::mapping::PopulationMap::addOSMLayer(const std::string& layerName,
			                                              const std::vector<osm::OSMTag>& tags,
			                                              float defaultValue);

			void eval();

		protected:
			std::map<GEOSGeometry*, GridMapDataType> popDensityGeomMap;
			std::map<osm::OSMTag, GridMapDataType> densityTagMap;
		};
	} // namespace mapping
} // namespace ugr

#endif // UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_
