#ifndef UGR_TEMPORALPOPULATIONMAP_H
#define UGR_TEMPORALPOPULATIONMAP_H
#include "uasgroundrisk/map_gen/PopulationMap.h"
#include "../src/utils/GeometryOperations.h"

namespace ugr
{
    namespace mapping
    {
        class TemporalPopulationMap : public PopulationMap
        {
        public:
            TemporalPopulationMap(const std::array<float, 4>& bounds, const int resolution)
                : PopulationMap(bounds, resolution)
            {
                CensusIngest censusIngest;
                // TODO: use an STR tree (impl in GEOS) for faster search
                auto geomDensityMap = censusIngest.makePopulationDensityMap<GridMapDataType>();
                auto boundedGeomDensityMap = util::boundGeometriesMap(geomDensityMap, bounds);
                popDensityGeomMap.swap(boundedGeomDensityMap);

                nhapsProps = censusIngest.makeNHAPSProportions();
            }

            ~TemporalPopulationMap()
            {
                util::destroyGEOSGeoms(boundedGeometries);
            }

            using PopulationMap::addOSMLayer;

            void SetHourOfDay(const short hourOfDay)
            {
                this->hourOfDay = hourOfDay;
                densityTagMap.clear();

                // densityTagMap
            }

            void eval()
            {
            }

        protected:
            short hourOfDay = 5;
            std::vector<std::vector<float>> nhapsProps;
            std::vector<GEOSGeometry*> boundedGeometries;
        };
    }
}
#endif // UGR_TEMPORALPOPULATIONMAP_H
