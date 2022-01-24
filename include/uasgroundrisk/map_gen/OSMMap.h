#ifndef UGR_OSMMAP_H
#define UGR_OSMMAP_H
#include <map>
#include <uasgroundrisk/map_gen/GeospatialGridMap.h>
#include "uasgroundrisk/map_gen/osm/OSMOverpassQueryBuilder.h"
#include "uasgroundrisk/map_gen/osm/OSMOverpassQuery.h"
#include "uasgroundrisk/map_gen/osm/handlers/DefaultNodeLocationsForWaysHandler.h"
#include "uasgroundrisk/map_gen/osm/OSMTag.h"

namespace ugr
{
	namespace mapping
	{
		class OSMMap : public GeospatialGridMap
		{
		public:
			OSMMap(const std::array<float, 4>& bounds, float resolution);

			void addOSMLayer(const std::string& layerName,
			                 const std::vector<osm::OSMTag>& tags,
			                 float defaultValue = 0);

			template <typename... THandlers>
			void eval(THandlers&&...handlers)
			{
				if (isEvaluated) return;
				if (tagLayerMap.empty())
				{
					// No point evaluating an empty population map
					// Ensure the empty population density layer is created anyway
					// in case it is evaluated
					return;
				}

				osm::OSMOverpassQueryBuilder builder(Coordinates(bounds[1], bounds[0]),
				                                     Coordinates(bounds[3], bounds[2]));
				for (const auto& tagLayerPair : tagLayerMap)
				{
					builder.withNodeTag(tagLayerPair.first).withWayTag(tagLayerPair.first);
				}

				osm::DefaultNodeLocationsForWaysHandler n2wHandler;
				n2wHandler.ignore_errors();

				osm::OSMOverpassQuery query = builder.build();
				query.makeQuery(n2wHandler, handlers...);
				isEvaluated = true;
			}

		protected:
			std::map<osm::OSMTag, std::string> tagLayerMap;
			bool isEvaluated = false;
		};
	}
}
#endif // UGR_OSMMAP_H
