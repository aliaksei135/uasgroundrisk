/*
 * RiskMap.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
#include "../map_gen/PopulationMap.h"
#include "RiskEnums.h"
#include "aircraft/AircraftDescentModel.h"
#include "aircraft/AircraftStateModel.h"
#include "weather/WeatherMap.h"
#include <grid_map_core/GridMap.hpp>
namespace ugr {
namespace risk {

class RiskMap : public ugr::mapping::GeospatialGridMap {
public:
  /**
   * Construct a static Risk map of a single AircraftModel
   * @param populationMap a GridMap of population density. Usually from
   * PopulationMap#eval()
   * @param aircraft the aircraft model to use
   */
  RiskMap(ugr::mapping::PopulationMap &populationMap,
          AircraftDescentModel aircraftDescent,
          AircraftStateModel aircraftState, WeatherMap weather);

  /**
   * Generate the actual risk map(s). This takes a vector of RiskType enum
   * values to determine which layers should be generated
   * @param risksToGenerate a vector of RiskType enum values for risk layers to
   * generate
   * @return a GridMap with risk layers generated
   */
  GridMap &generateMap(const std::vector<ugr::risk::RiskType> &risksToGenerate);

  void eval() override;

protected:
  AircraftDescentModel descentModel;
  AircraftStateModel stateModel;
  WeatherMap weather;

  void generateImpactMap();

  void generateStrikeMap();

  void generateFatalityMap();

  void initLayer(const std::string &layerName);

  static double lethalArea(double impactAngle, double uasWidth);

  static double vel2ke(double velocity, double mass);

  static double fatalityProbability(double alpha, double beta,
                                    double impactEnergy, double shelterFactor);
};

} // namespace risk
} // namespace ugr
#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_