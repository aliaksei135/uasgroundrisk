/*
 * GeospatialGridMap.h
 *
 *  Created by A.Pilko on 18/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_GEOSPATIALGRIDMAP_H_
#define UASGROUNDRISK_SRC_MAP_GEN_GEOSPATIALGRIDMAP_H_

#include <grid_map_core/GridMap.hpp>
namespace ugr {
namespace mapping {

class GeospatialGridMap : public grid_map::GridMap {
public:
  GeospatialGridMap(std::array<float, 4> bounds, int resolution,
                    const std::string &frameId);

  void setBounds(std::array<float, 4> boundsArr, int resolution);

  std::array<float, 4> getBounds() const { return bounds; }

  virtual void eval() = 0;

protected:
  std::array<float, 4> bounds;
};

} // namespace mapping
} // namespace ugr
#endif // UASGROUNDRISK_SRC_MAP_GEN_GEOSPATIALGRIDMAP_H_
