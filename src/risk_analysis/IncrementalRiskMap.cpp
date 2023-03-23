#include "uasgroundrisk/risk_analysis/IncrementalRiskMap.h"

#include "../utils/DataFitting.h"
#include "../utils/GeometryOperations.h"
#include "../utils/VectorOperations.h"
#include <chrono>
#include <omp.h>
#include <random>

#include "uasgroundrisk/risk_analysis/aircraft/AircraftModel.h"
#include "uasgroundrisk/risk_analysis/obstacles/ObstacleMap.h"

#include <spdlog/spdlog.h>

ugr::risk::IncrementalRiskMap::IncrementalRiskMap(ugr::mapping::PopulationMap& populationMap,
	const ugr::risk::AircraftModel& aircraftModel,
	ugr::risk::ObstacleMap& obstacleMap,
	const ugr::risk::WeatherMap& weather) : RiskMap(populationMap, aircraftModel, obstacleMap, weather)
{

}

double ugr::risk::IncrementalRiskMap::getPointStrikeProbability(const ugr::gridmap::Position3& position,
	const int heading)
{
	const auto& index = world2Local(position.x(), position.y());

	std::vector<GridMapDataType> impactAngles, impactVelocities;
	std::vector<Matrix, aligned_allocator<Matrix>> impactPDFs;
	const auto& altitude = position.z();

	makePointImpactMap(index, altitude, heading, impactPDFs, impactAngles, impactVelocities);

	/* We have now evaluated the impact PDF of the aircraft*/
	/* Now we move onto the strike risk analysis */

	// These are used later
	const GridMapDataType pixelArea = getResolution() * getResolution();
	const auto uasWidth = aircraftModel.width;
	const Matrix& populationDensityMap = get("Population Density");

	GridMapDataType allDescentStrikeRiskSum = 0;

	for (int i = 0; i < aircraftModel.descents.size(); ++i)
	{
		// Work out the lethal area of the aircraft when it crashes
		const auto letArea = lethalArea(DEG2RAD(impactAngles[i]), uasWidth);
		const Matrix strikeRisk = (impactPDFs[i].cwiseProduct(populationDensityMap) * letArea) / pixelArea;
		const auto strikeRiskSum = static_cast<GridMapDataType>(aircraftModel.failureProb) * strikeRisk.sum();
		allDescentStrikeRiskSum += strikeRiskSum;

		const auto descentName = aircraftModel.descents[i]->getName();
		at(descentName + " Strike Risk", index) = strikeRiskSum;
		at(descentName + " Impact Angle", index) = impactAngles[i];
		at(descentName + " Impact Velocity", index) = impactVelocities[i];
	}

	return allDescentStrikeRiskSum;
}
double ugr::risk::IncrementalRiskMap::getPointFatalityProbability(const ugr::gridmap::Position3& position,
	const int heading)
{
	const auto& index = world2Local(position.x(), position.y());
	const auto uasMass = aircraftModel.mass;
	const Matrix shelterFactorMap = get("Shelter Factor");
	const auto& shelterFactor = shelterFactorMap(index.x(), index.y());

	// Need to generate the individual descent strike risk maps and impact characteristics
	getPointStrikeProbability(position, heading);

	double fatalityRisk = 0;

	for (const auto& descent : aircraftModel.descents)
	{
		const auto descentName = descent->getName();
		const Matrix& strikeRiskMap = get(descentName + " Strike Risk");
		const Matrix& impactVelocities = get(descentName + " Impact Velocity");

		fatalityRisk += strikeRiskMap(index.x(), index.y()) * fatalityProbability(
			1e6, 100, vel2ke(impactVelocities(index.x(), index.y()), uasMass), shelterFactor);
	}

	return fatalityRisk;
}
