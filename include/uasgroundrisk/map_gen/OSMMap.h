#ifndef UGR_OSMMAP_H
#define UGR_OSMMAP_H
#include <uasgroundrisk/map_gen/GeospatialGridMap.h>

#include "osm/DefaultNodeLocationsForWaysHandler.h"
#include "osm/builder/OSMOverpassQueryBuilder.h"
#include "osm/OSMOverpassQuery.h"

namespace ugr
{
	namespace mapping
	{
		class OSMMap : public GeospatialGridMap
		{
		public:
			OSMMap(const std::array<float, 4>& bounds, float resolution);

			void addOSMLayer(const std::string& layerName,
			                 const std::vector<OSMTag>& tags,
			                 float defaultValue = 0);

			template <typename... THandlers>
			void eval(THandlers&&...handlers);

		protected:
			std::map<OSMTag, std::string> tagLayerMap;

			OSMOverpassQueryBuilder builder;
			DefaultNodeLocationsForWaysHandler n2wHandler;
		};
	}
}
#endif // UGR_OSMMAP_H
