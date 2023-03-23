/*
 * RiskMap.h
 *
 *  Created by A.Pilko on 22/03/2023.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_INCREMENTALRISKMAP_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_INCREMENTALRISKMAP_H_
#include <random>

#include "aircraft/AircraftModel.h"
#include "obstacles/ObstacleMap.h"
#include "RiskMap.h"
#include "uasgroundrisk/map_gen/PopulationMap.h"
#include "uasgroundrisk/map_gen/GeospatialGridMap.h"
#include "uasgroundrisk/risk_analysis/RiskEnums.h"
#include "uasgroundrisk/risk_analysis/weather/WeatherMap.h"
#include "uasgroundrisk/gridmap/GridMap.h"

#define EIGEN_DONT_PARALLELIZE

namespace ugr
{
	namespace risk
	{
		using namespace gridmap;

		class IncrementalRiskMap : protected RiskMap
		{
		 public:
			EIGEN_MAKE_ALIGNED_OPERATOR_NEW

			/**
			 * Construct a static Risk map of a single AircraftModel
			 * @param populationMap a GridMap of population density. Usually from
			 * PopulationMap#eval()
			 * @param aircraftModel the aircraft model to use
			 * @param weather the weather map
			 */


			IncrementalRiskMap(mapping::PopulationMap& populationMap,
				const AircraftModel& aircraftModel, ObstacleMap& obstacleMap,
				const WeatherMap& weather);

			IncrementalRiskMap(const IncrementalRiskMap& other) = delete;
			IncrementalRiskMap(IncrementalRiskMap&& other) noexcept = default;
			IncrementalRiskMap& operator=(const IncrementalRiskMap& other) = delete;
			IncrementalRiskMap& operator=(IncrementalRiskMap&& other) noexcept = delete;
			~IncrementalRiskMap() override = default;

			double getPointStrikeProbability(const Position3& position, int heading);

			double getPointFatalityProbability(const Position3& position, int heading);

		};
	} // namespace risk
} // namespace ugr
#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_INCREMENTALRISKMAP_H_
