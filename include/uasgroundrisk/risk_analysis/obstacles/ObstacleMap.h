#ifndef UGR_OBSTACLEMAP_H
#define UGR_OBSTACLEMAP_H
#include "uasgroundrisk/map_gen/OSMMap.h"
#include <vector>

namespace ugr
{
    namespace risk
    {
        class ObstacleMap : public mapping::OSMMap
        {
        public:
            ObstacleMap(const std::array<float, 4>& bounds, const float resolution)
                : OSMMap(bounds, resolution)
            {
            }

            using OSMMap::addOSMLayer;

            void addBuildingHeights();

            void eval() override;

        protected:
            // std::vector<std::unique_ptr<osmium::handler::Handler>> handlers;
        };
    }
}
#endif // UGR_OBSTACLEMAP_H
