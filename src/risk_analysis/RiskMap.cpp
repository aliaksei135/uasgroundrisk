/*
 * RiskMap.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "uasgroundrisk/risk_analysis/RiskMap.h"
#include "../utils/DataFitting.h"
#include "../utils/VectorOperations.h"
#include "../utils/GeometryOperations.h"
#include <Eigen/Dense>
#include <cassert>
#include <chrono>
#include <iostream>
#include <omp.h>
#include <random>
#include <utility>

#include "uasgroundrisk/risk_analysis/aircraft/AircraftModel.h"


using namespace ugr::gridmap;

ugr::risk::RiskMap::RiskMap(
	mapping::PopulationMap& populationMap,
	AircraftModel& aircraftModel,
	const WeatherMap& weather)
	: GeospatialGridMap(populationMap.getBounds(),
	                    static_cast<int>(populationMap.getResolution())), aircraftModel(std::move(aircraftModel)),
	  weather(weather),
	  generator(std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()))
{
	// Evaluate population density map
	populationMap.eval();
	// Copy across only the the population density and building height layer
	initLayer("Population Density");
	initLayer("Building Height");
	get("Population Density") = populationMap.get("Population Density");
	get("Building Height") = populationMap.get("Building Height");

	// Create objects required for sample distribution generation
	// construct a trivial random generator engine from a time-based seed:

	initRiskMapLayers();

	// Generate eval grid once instead of on every iteration
	evalXs.resize(this->sizeX * this->sizeY);
	evalYs.resize(this->sizeX * this->sizeY);
	int i = 0;
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			// Invert x in line with the axes convention chosen here
			evalXs[i] = sizeX - x;
			evalYs[i] = y;
			++i;
		}
	}
}

GridMap& ugr::risk::RiskMap::generateMap(
	const std::vector<RiskType>& risksToGenerate)
{
	if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
	              RiskType::FATALITY) != risksToGenerate.end())
	{
		generateStrikeMap();
		generateFatalityMap();
	}
	else if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
	                   RiskType::STRIKE) != risksToGenerate.end())
	{
		generateStrikeMap();
	}
	// else if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
	//                    RiskType::IMPACT) != risksToGenerate.end())
	// {
	//
	// }
	return *this;
}

void ugr::risk::RiskMap::eval()
{
	generateStrikeMap();
	generateFatalityMap();
}

void ugr::risk::RiskMap::initRiskMapLayers()
{
	initLayer("Glide Strike Risk");
	initLayer("Glide Fatality Risk");
	initLayer("Glide Impact Angle");
	initLayer("Glide Impact Velocity");

	initLayer("Ballistic Strike Risk");
	initLayer("Ballistic Fatality Risk");
	initLayer("Ballistic Impact Angle");
	initLayer("Ballistic Impact Velocity");

	initLayer("Parachute Strike Risk");
	initLayer("Parachute Fatality Risk");
	initLayer("Parachute Impact Angle");
	initLayer("Parachute Impact Velocity");

	// Setting shelter factor to 0 results in infinite fatality risk, so we set a small value instead
	add("Shelter Factor", 0.1);
}


void ugr::risk::RiskMap::initLayer(const std::string& layerName)
{
	add(layerName, 0);
	get(layerName).setZero();
}

void ugr::risk::RiskMap::generateStrikeMap()
{
	// Iterate through all cells in the grid map
#pragma omp parallel for collapse(2)
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			// TODO: package this as a CUDA function
			addPointStrikeMap({x, y});
		}
	}
}

void ugr::risk::RiskMap::generateFatalityMap()
{
	const auto uasMass = aircraftModel.mass;

	const Size size = getSize();
	const auto len1d = size.x() * size.y();

	// TODO Add mapped shelter factor
	// add("Shelter Factor", 0.3);
	const Matrix shelterFactorMap = get("Shelter Factor");

	for (int i = 0; i < aircraftModel.descents.size(); ++i)
	{
		const auto descentName = aircraftModel.descents[i]->getName();
		const Matrix& strikeRiskMap = get(descentName + " Strike Risk");
		const Matrix& impactVelocities = get(descentName + " Impact Velocity");
		// const Matrix& impactAngles = get(descentName + " Impact Angle");

		const Matrix fatalityRisk = strikeRiskMap * fatalityProbability(
			1e6, 34, vel2ke(impactVelocities, uasMass), shelterFactorMap);

#pragma omp critical
		get(descentName + " Fatality Risk") = fatalityRisk;
	}
}

void ugr::risk::RiskMap::addPointStrikeMap(const ugr::gridmap::Index& index)
{
	std::vector<GridMapDataType> impactAngles, impactVelocities, buildingImpactProbs;
	std::vector<Matrix, aligned_allocator<GridMapDataType>> impactPDFs;

	makePointImpactMap(index, impactPDFs, impactAngles, impactVelocities, buildingImpactProbs);

	const auto shelterFactorSum = std::accumulate(buildingImpactProbs.begin(), buildingImpactProbs.end(), 0.0);
	if (shelterFactorSum > 0.0)
		at("Shelter Factor", index) = shelterFactorSum / nSamples;

	/* We have now evaluated the impact PDF of the aircraft*/
	/* Now we move onto the strike risk analysis */

	// These are used later
	const GridMapDataType pixelArea = getResolution() * getResolution();
	const auto uasWidth = aircraftModel.width;
	// Get population map and convert from people/km^2 to people/m^2
	const Matrix& populationDensityMap = get("Population Density") * 1e-6;


	for (int i = 0; i < aircraftModel.descents.size(); ++i)
	{
		// Work out the lethal area of the aircraft when it crashes
		const auto letArea = lethalArea(DEG2RAD(impactAngles[i]), uasWidth);
		const Matrix strikeRisk = (impactPDFs[i].cwiseProduct(populationDensityMap) * letArea) / pixelArea;
		const auto strikeRiskSum = strikeRisk.sum();

		const auto descentName = aircraftModel.descents[i]->getName();

		// Synchronise writing to the common gridmap
#pragma omp critical
		{
			// As we are only generating the strike risk from a single point,
			// this is summed across the entire strike risk map for that point
			// and set as the scalar value for the point it was generated for
			at(descentName + " Strike Risk", index) = strikeRiskSum;
			at(descentName + " Impact Angle", index) = impactAngles[i];
			at(descentName + " Impact Velocity", index) = impactVelocities[i];
		}
	}
}

void ugr::risk::RiskMap::makePointImpactMap(const ugr::gridmap::Index& index,
                                            std::vector<ugr::gridmap::Matrix, aligned_allocator<GridMapDataType>>&
                                            impactPDFs,
                                            std::vector<GridMapDataType>& impactAngles,
                                            std::vector<GridMapDataType>& impactVelocities,
                                            std::vector<GridMapDataType>& buildingImpactProbs)
{
	const auto& windVelX = weather.at("Wind VelX", index);
	const auto& windVelY = weather.at("Wind VelY", index);
	auto windXVelDist = std::normal_distribution<double>(windVelX, 5);
	auto windYVelDist = std::normal_distribution<double>(windVelY, 0.5);
	std::vector<Vector2d, aligned_allocator<double>> windVect(nSamples);

	// Create samples of state distributions
	const auto& altitude = aircraftModel.state.getAltitude();
	const auto& lateralVel = sqrt(pow(aircraftModel.state.velocity(0), 2) + pow(aircraftModel.state.velocity(1), 2));
	const auto& verticalVel = aircraftModel.state.velocity(2);
	auto altDist = std::normal_distribution<double>(altitude, 5);
	auto lateralVelDist = std::normal_distribution<double>(lateralVel, 1.5);
	auto verticalVelDist = std::normal_distribution<double>(verticalVel, 0.5);
	auto headingUniformDist = std::uniform_real_distribution<double>(DEG2RAD(0),DEG2RAD(360));
	auto headingNormalDist = std::normal_distribution<double>(DEG2RAD(aircraftModel.state.getHeading()), DEG2RAD(5));


	// Generate random variable samples for LoC states
	std::vector<double> altVect(nSamples);
	std::vector<double> lateralVelVect(nSamples);
	std::vector<double> verticalVelVect(nSamples);
	std::vector<Rotation2Dd, Eigen::aligned_allocator<Rotation2Dd>> headingVect(nSamples);
	for (size_t i = 0; i < nSamples; ++i)
	{
		altVect[i] = altDist(generator);
		lateralVelVect[i] = lateralVelDist(generator);
		verticalVelVect[i] = verticalVelDist(generator);
		windVect[i] = {windXVelDist(generator), windYVelDist(generator)};
		if (anyHeading)
		{
			headingVect[i] = Rotation2Dd(util::bearing2Angle(headingUniformDist(generator)));
		}
		else
		{
			headingVect[i] = Rotation2Dd(util::bearing2Angle(headingNormalDist(generator)));
		}
	}

	// // Create common heading rotation
	// const Rotation2Dd headingRotation(
	// 	util::bearing2Angle(DEG2RAD(aircraftModel.state.getHeading())));

	// Convert Index into vector for easy arithmetic later
	const Vector2d indexVec{index[0], index[1]};

	std::vector<util::Gaussian2DParamVector> descentDistrParams;

	for (const auto& descentModel : aircraftModel.descents)
	{
		const auto& samples = descentModel->impact(altVect, lateralVelVect, verticalVelVect);

		util::Point2DVector impactPositions(nSamples);
		double impactAngle = 0, impactVelocity = 0;
		int buildingCollisionCount = 0;

		// Model the descents of each of the random samples for LoC state vector to find an equal
		// number of ground impact samples we can fit distributions to.
#pragma omp parallel for reduction(+:impactAngle, impactVelocity, buildingCollisionCount)
		for (int i = 0; i < nSamples; ++i)
		{
			const auto& sample = samples[i];

			// As the heading rotation is an angle not a bearing, it is measured counter clockwise from
			// the x axis corresponding to the geospatial gridmap axes.
			// Therefore a zero rotation should correspond to motion in the x axis only, hence the y=0 here
			const Vector2d dist1D(sample.impactDistance, 0);

			impactPositions[i] =
				((headingVect[i] * dist1D + (sample.impactTime * windVect[i])) / xyRes) + indexVec;

			const auto groundTrackIndices = ugr::util::bresenham2D(index, {
				                                                       std::ceil(impactPositions[i][0]),
				                                                       std::ceil(impactPositions[i][1])
			                                                       });
			for (const auto& trackIndex : groundTrackIndices)
			{
				if (!isInBounds(trackIndex)) continue;
				// Get this points distance from the impact point
				const auto dx = trackIndex[0] - index[0];
				const auto dy = trackIndex[1] - index[1];
				const auto distToImpact = std::sqrt(dx * dx + dy * dy) * xyRes;
				if (distToImpact < xyRes) continue;
				// Estimate the altitude at this point
				const auto altAtPoint = distToImpact * std::tan(DEG2RAD(sample.impactAngle));
				const GridMapDataType& buildingHeightAtPoint = at("Building Height", trackIndex);
				if (altAtPoint <= buildingHeightAtPoint)
				{
					++buildingCollisionCount;
					break;
				}
			}

			impactAngle += sample.impactAngle;
			impactVelocity += sample.impactVelocity;
		}

		// Fit a distribution to the propagated samples
		auto distParams = util::Gaussian2DFit(impactPositions);
		impactAngle /= nSamples;
		impactVelocity /= nSamples;
		const double buildingCollisionProb = buildingCollisionCount / nSamples;

		descentDistrParams.emplace_back(distParams);
		impactAngles.emplace_back(impactAngle);
		impactVelocities.emplace_back(impactVelocity);
		buildingImpactProbs.emplace_back(buildingCollisionProb);
	}

	for (auto& distParams : descentDistrParams)
	{
		// Set amplitudes to 1 to avoid zero division errors when normalising to PDF later
		distParams[0] = 1;

		// Fit 2D gaussian kernels to the descent model samples instead of propagating the samples all the way to strike risk.
		// This should account for a more accurate probabilistic picture of the risk.
		Matrix impactPDFGrid = util::gaussian2D(evalXs, evalYs, distParams).reshaped<RowMajor>(sizeX, sizeY);

		// Turn fitted impact risk gaussians into PDFs that we can use.
		// Work these out first to avoid aliasing Eigen expressions
		const auto pdfQuot = impactPDFGrid.sum();
		impactPDFGrid /= pdfQuot;
		impactPDFs.emplace_back(impactPDFGrid);
	}
}

double ugr::risk::RiskMap::lethalArea(const double impactAngle, const double uasWidth)
{
	constexpr auto personRadius = 1;
	constexpr auto personHeight = 1;
	const auto uasRadius = uasWidth / 2;
	return 2 * personHeight * (personRadius + uasRadius) /
		tan(impactAngle) +
		M_PI * pow(uasRadius + personRadius, 2);
}

double ugr::risk::RiskMap::vel2ke(const double velocity, const double mass)
{
	return 0.5 * mass * pow(velocity, 2);
}

double ugr::risk::RiskMap::fatalityProbability(const double alpha, const double beta,
                                               const double impactEnergy,
                                               const double shelterFactor)
{
	if (impactEnergy < 1e-15) return 0;
	return 1 / (sqrt(alpha / beta)) *
		pow(beta / impactEnergy, 1 / (4 * shelterFactor));
}

ugr::gridmap::Matrix ugr::risk::RiskMap::lethalArea(const ugr::gridmap::Matrix& impactAngle, const double uasWidth)
{
	constexpr auto personRadius = 1;
	constexpr auto personHeight = 1.8;
	const auto uasRadius = uasWidth / 2;
	return (2 * personHeight * (personRadius + uasRadius) /
		tan(impactAngle.array()) +
		M_PI * pow(uasRadius + personRadius, 2)).matrix();
}

ugr::gridmap::Matrix ugr::risk::RiskMap::vel2ke(const ugr::gridmap::Matrix& velocity, const double mass)
{
	return (0.5 * mass * pow(velocity.array(), 2)).matrix();
}

ugr::gridmap::Matrix ugr::risk::RiskMap::fatalityProbability(const double alpha, const double beta,
                                                             const ugr::gridmap::Matrix& impactEnergy,
                                                             const ugr::gridmap::Matrix& shelterFactor)
{
	return (1 / (sqrt(alpha / beta)) *
		pow(beta / impactEnergy.array(), 1 / (4 * shelterFactor.array()))).matrix();
}
