/*
 * RiskMap.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
#include <random>

#include "../map_gen/PopulationMap.h"
#include "RiskEnums.h"
#include "aircraft/AircraftDescentModel.h"
#include "aircraft/AircraftStateModel.h"
#include "weather/WeatherMap.h"
#include "../gridmap/GridMap.h"


namespace ugr
{
	namespace risk
	{
		using namespace ugr::gridmap;

		class RiskMap final : public ugr::mapping::GeospatialGridMap
		{
		public:
			/**
			 * Construct a static Risk map of a single AircraftModel
			 * @param populationMap a GridMap of population density. Usually from
			 * PopulationMap#eval()
			 * @param aircraftDescent the aircraft descent model to use
			 * @param aircraftState the current aircraft state
			 * @param weather the weather map
			 */
			RiskMap(ugr::mapping::PopulationMap& populationMap,
			        const AircraftDescentModel& aircraftDescent,
			        AircraftStateModel aircraftState, const WeatherMap& weather);

			/**
			 * Generate the actual risk map(s). This takes a vector of RiskType enum
			 * values to determine which layers should be generated
			 * @param risksToGenerate a vector of RiskType enum values for risk layers to
			 * generate
			 * @return a GridMap with risk layers generated
			 */
			GridMap& generateMap(const std::vector<ugr::risk::RiskType>& risksToGenerate);

			void makePointImpactMap(const gridmap::Index& index,
			                        gridmap::Matrix& outGlide, gridmap::Matrix& outBallistic,
			                        GridMapDataType& outGlideAngle, GridMapDataType& outGlideVelocity,
			                        GridMapDataType& outBallisticAngle, GridMapDataType& outBallisticVelocity);

			void eval();

		protected:
			AircraftDescentModel descentModel;
			AircraftStateModel stateModel;
			WeatherMap weather;
			int nSamples = 40; //CLT says 30-50 samples is good enough
			std::default_random_engine generator;

			void generateStrikeMap();

			void generateFatalityMap();

			void addPointStrikeMap(const gridmap::Index& index);

			void initRiskMapLayers();

			void initLayer(const std::string& layerName);

			static double lethalArea(double impactAngle, double uasWidth);

			static double vel2ke(double velocity, double mass);

			static double fatalityProbability(double alpha, double beta,
			                                  double impactEnergy, double shelterFactor);
		};
	} // namespace risk
} // namespace ugr
#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
