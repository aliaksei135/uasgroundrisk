/*
 * PopulationMap.h
 *
 *  Created by A.Pilko on 09/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_
#define UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_

#include "uasgroundrisk/map_gen/OSMMap.h"
#include "../../../src/map_gen/osm/DefaultNodeLocationsForWaysHandler.h"
#include "../../../src/map_gen/osm/builder/OSMOverpassQueryBuilder.h"
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

			void eval();

		protected:
			std::map<GEOSGeometry*, GridMapDataType> popDensityGeomMap;
			std::map<OSMTag, GridMapDataType> densityTagMap;
		};
	} // namespace mapping
} // namespace ugr

#endif // UASGROUNDRISK_SRC_MAP_GEN_POPULATIONMAP_H_
