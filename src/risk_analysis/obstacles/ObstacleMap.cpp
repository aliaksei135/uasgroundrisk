#include "uasgroundrisk/risk_analysis/obstacles/ObstacleMap.h"

#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMBuildingsHandler.h"

void ugr::risk::ObstacleMap::addBuildingHeights()
{
    addOSMLayer("Building Height", {mapping::osm::OSMTag("building")});
    isEvaluated = false;
}

void ugr::risk::ObstacleMap::eval()
{
    if (isEvaluated) return;
    mapping::GridMapOSMBuildingsHandler handler(this);
    OSMMap::eval(handler);
    isEvaluated = true;
}
