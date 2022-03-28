#ifndef UGR_TEMPORALPOPULATIONMAP_H
#define UGR_TEMPORALPOPULATIONMAP_H
#include "uasgroundrisk/map_gen/PopulationMap.h"

namespace ugr
{
    namespace mapping
    {
        class TemporalPopulationMap : public PopulationMap
        {
        public:
            TemporalPopulationMap(const std::array<float, 4>& bounds, int resolution, short defaultHour = 12);

            ~TemporalPopulationMap() override;

            // Delete this for now, as all OSM tags come from the NHAPS mapping
            // void addOSMLayer(const std::string& layerName,
            //                  const std::vector<osm::OSMTag>& tags,
            //                  float defaultValue) override = delete;

            void setHourOfDay(const short hourOfDay);

            std::vector<double> calculateAreas(const std::vector<GEOSGeometry*>& geoms) const;

            void intersectResidentialGeometries();

            void setOSMGeometries();

            void fillGridMapPoly(const std::string& layerName, const GEOSGeometry* geom,
                                 const GridMapDataType& geomDensity);

            void eval() override;

        protected:
            short hourOfDay;
            int totalPopulation;
            GEOSContextHandle_t geosCtx;

            std::vector<std::vector<float>> nhapsProps;
            std::vector<GEOSGeometry*> boundedGeometries;
            std::map<GEOSGeometry*, double> geomAreas;
            std::map<GEOSGeometry*, GridMapDataType> activeGeomDensityMap;
            std::map<osm::OSMTag, std::vector<GEOSGeometry*>> tagGeomMap;
            std::map<osm::OSMTag, double> tagAreas;
        };
    }
}
#endif // UGR_TEMPORALPOPULATIONMAP_H
