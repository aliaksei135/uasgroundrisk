/*
 * RiskMap.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
#include <random>

#include "aircraft/AircraftModel.h"
#include "obstacles/ObstacleMap.h"
#include "uasgroundrisk/map_gen/PopulationMap.h"
#include "uasgroundrisk/map_gen/GeospatialGridMap.h"
#include "uasgroundrisk/risk_analysis/RiskEnums.h"
#include "uasgroundrisk/risk_analysis/weather/WeatherMap.h"
#include "uasgroundrisk/gridmap/GridMap.h"


namespace ugr
{
    namespace risk
    {
        using namespace gridmap;

        class RiskMap final : public mapping::GeospatialGridMap
        {
        public:
            /**
             * Construct a static Risk map of a single AircraftModel
             * @param populationMap a GridMap of population density. Usually from
             * PopulationMap#eval()
             * @param aircraftModel the aircraft model to use
             * @param weather the weather map
             */
            RiskMap(mapping::PopulationMap& populationMap,
                    AircraftModel& aircraftModel, ObstacleMap& obstacleMap,
                    const WeatherMap& weather);

            RiskMap(const RiskMap& other) = delete;
            RiskMap(RiskMap&& other) noexcept = default;
            RiskMap& operator=(const RiskMap& other) = delete;
            RiskMap& operator=(RiskMap&& other) noexcept = default;
            ~RiskMap() = default;

            /**
             * Generate the actual risk map(s). This takes a vector of RiskType enum
             * values to determine which layers should be generated
             *
             * @return a GridMap with risk layers generated
             */
            GridMap& generateMap(const std::vector<ugr::risk::RiskType>& risksToGenerate);

            void makePointImpactMap(const gridmap::Index& index,
                                    std::vector<gridmap::Matrix, aligned_allocator<gridmap::Matrix>>& impactPDFs,
                                    std::vector<GridMapDataType>& impactAngles,
                                    std::vector<GridMapDataType>& impactVelocities,
                                    std::vector<GridMapDataType>& buildingImpactProbs);

            void eval();

            bool IsAnyHeading() const
            {
                return anyHeading;
            }

            void SetAnyHeading(const bool anyHeading)
            {
                this->anyHeading = anyHeading;
            }

        protected:
            AircraftModel aircraftModel;

            WeatherMap weather;
            int nSamples = 50; //CLT says 30-50 samples is good enough
            std::default_random_engine generator;
            Eigen::Vector<GridMapDataType, Dynamic> evalXs, evalYs;
            bool anyHeading = false;

            void generateStrikeMap();

            void generateFatalityMap();

            void addPointStrikeMap(const gridmap::Index& index);

            void initRiskMapLayers();

            void initLayer(const std::string& layerName);

            static double lethalArea(double impactAngle, double uasWidth);

            static double vel2ke(double velocity, double mass);

            static double fatalityProbability(double alpha, double beta,
                                              double impactEnergy, double shelterFactor);

            static ugr::gridmap::Matrix lethalArea(const ugr::gridmap::Matrix& impactAngle, double uasWidth);

            static ugr::gridmap::Matrix vel2ke(const ugr::gridmap::Matrix& velocity, double mass);

            static ugr::gridmap::Matrix fatalityProbability(double alpha, double beta,
                                                            const ugr::gridmap::Matrix& impactEnergy,
                                                            const ugr::gridmap::Matrix& shelterFactor);
        };
    } // namespace risk
} // namespace ugr
#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_RISKMAP_H_
