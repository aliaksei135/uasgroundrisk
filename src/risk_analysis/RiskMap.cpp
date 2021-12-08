/*
 * RiskMap.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "RiskMap.h"
#include "../utils/DataFitting.h"
#include "../utils/VectorOperations.h"
#include <Eigen/Dense>
#include <cassert>
#include <iostream>
#include <omp.h>
#include <random>
#include <utility>

ugr::risk::RiskMap::RiskMap(
	mapping::PopulationMap& populationMap,
	const AircraftDescentModel& aircraftDescent,
	AircraftStateModel aircraftState,
	const WeatherMap& weather)
	: GeospatialGridMap(populationMap.getBounds(),
						static_cast<int>(populationMap.getResolution())), descentModel(aircraftDescent),
	  stateModel(std::move(aircraftState)),
	  weather(weather)
{
	// Evaluate population density map
	populationMap.eval();
	// Copy across only the the population density layer
	constexpr auto popDensityLayerName = "Population Density";
	initLayer(popDensityLayerName);
	get(popDensityLayerName) = populationMap.get(popDensityLayerName);

	// Create objects required for sample distribution generation
	// construct a trivial random generator engine from a time-based seed:
	const auto seed = static_cast<unsigned int>(
		std::chrono::system_clock::now().time_since_epoch().count());
	generator = std::default_random_engine(seed);

	initRiskMapLayers();
}

ugr::gridmap::GridMap& ugr::risk::RiskMap::generateMap(
	const std::vector<RiskType>& risksToGenerate)
{
	if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
				  RiskType::FATALITY) != risksToGenerate.end())
	{
		generateImpactMap();
		generateStrikeMap();
		generateFatalityMap();
	}
	else if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
					   RiskType::STRIKE) != risksToGenerate.end())
	{
		generateImpactMap();
		generateStrikeMap();
	}
	else if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
					   RiskType::IMPACT) != risksToGenerate.end())
	{
		generateImpactMap();
	}
	return *this;
}

void ugr::risk::RiskMap::eval()
{
	generateImpactMap();
	generateStrikeMap();
	generateFatalityMap();
}

void ugr::risk::RiskMap::initRiskMapLayers()
{
	initLayer("Glide Impact Risk");
	initLayer("Glide Strike Risk");
	initLayer("Glide Fatality Risk");
	initLayer("Glide Impact Angle");
	initLayer("Glide Impact Velocity");

	initLayer("Ballistic Impact Risk");
	initLayer("Ballistic Strike Risk");
	initLayer("Ballistic Fatality Risk");
	initLayer("Ballistic Impact Angle");
	initLayer("Ballistic Impact Velocity");
}

void ugr::risk::RiskMap::generateImpactMap()
{
#ifndef NDEBUG
	// Debug iteration counter
	int n = 0;
#endif

	// Iterate through all cells in the grid map

#pragma omp parallel for collapse(2)
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			// TODO: pacakge this as a CUDA function

			addPointStrikeMap({x, y});

#ifndef NDEBUG
			std::cout << n << std::endl;
			++n;
#endif
		}
	}
}

void ugr::risk::RiskMap::initLayer(const std::string& layerName)
{
	add(layerName, 0);
	get(layerName).setZero();
}

void ugr::risk::RiskMap::generateStrikeMap()
{
#ifndef NDEBUG
	// Debug iteration counter
	int n = 0;
#endif

	// Iterate through all cells in the grid map
#pragma omp parallel for collapse(2)
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			// TODO: package this as a CUDA function

			addPointStrikeMap({x, y});

#ifndef NDEBUG
			std::cout << n << std::endl;
			++n;
#endif
		}
	}

	// const auto pixelArea = pow(getResolution(), 2);
	// const auto uasWidth = descentModel.width;
	// const Matrix& populationDensityMap = get("Population Density");
	//
	// const Size size = getSize();
	// const auto len1d = size.x() * size.y();
	//
	// const Matrix& glideImpactRiskMap = get("Glide Impact Risk");
	// initLayer("Glide Lethal Area");
	// const Matrix& glideImpactAngles = get("Glide Impact Angle");
	// Matrix glideLethalAreas(getSize().x(), getSize().y());
	// auto* pGlideLethalAreas = glideLethalAreas.data();
	// for (size_t i = 0; i < len1d; ++i)
	// {
	// 	pGlideLethalAreas[i] = lethalArea(glideImpactAngles(i), uasWidth);
	// }
	//
	// const Matrix unScaledGlideStrikeRisk = (glideImpactRiskMap * populationDensityMap * glideLethalAreas);
	// const Matrix glideStrikeRisk = unScaledGlideStrikeRisk / pixelArea;
	// initLayer("Glide Strike Risk");
	// get("Glide Strike Risk") = glideStrikeRisk;
	//
	// const Matrix& ballisticImpactRiskMap = get("Ballistic Impact Risk");
	// initLayer("Ballistic Lethal Area");
	// const Matrix& ballisticImpactAngles = get("Ballistic Impact Angle");
	// Matrix ballisticLethalAreas(getSize().x(), getSize().y());
	// auto* pBallisticLethalAreas = ballisticLethalAreas.data();
	// for (size_t i = 0; i < len1d; ++i)
	// {
	// 	pBallisticLethalAreas[i] = lethalArea(ballisticImpactAngles(i), uasWidth);
	// }
	//
	// const Matrix unscaledBallisticStrikeRisk = (ballisticImpactRiskMap * populationDensityMap * ballisticLethalAreas);
	// const Matrix ballisticStrikeRisk = unscaledBallisticStrikeRisk / pixelArea;
	// initLayer("Ballistic Strike Risk");
	// get("Ballistic Strike Risk") = ballisticStrikeRisk;
}

void ugr::risk::RiskMap::generateFatalityMap()
{
	const auto uasMass = descentModel.mass;

	const Size size = getSize();
	const auto len1d = size.x() * size.y();

	const Matrix& glideStrikeRiskMap = get("Glide Strike Risk");
	initLayer("Glide Impact Energy");
	const Matrix& glideImpactVelocities = get("Glide Impact Velocity");
	Matrix glideImpactProbs(getSize().x(), getSize().y());
	auto* pGlideImpactProbs = glideImpactProbs.data();
	for (size_t i = 0; i < len1d; ++i)
	{
		pGlideImpactProbs[i] = fatalityProbability(
			1e6, 34, vel2ke(glideImpactVelocities(i), uasMass), 0.3);
	}

	const Matrix glideFatalityRisk = glideStrikeRiskMap * glideImpactProbs;
	initLayer("Glide Fatality Risk");
	get("Glide Fatality Risk") = glideFatalityRisk;

	const auto ballisticStrikeRiskMap = get("Ballistic Strike Risk");
	initLayer("Ballistic Impact Energy");
	const Matrix& ballisticImpactVelocities = get("Ballistic Impact Velocity");
	Matrix ballisticImpactProbs(getSize().x(), getSize().y());
	auto* pBallisticImpactProbs = ballisticImpactProbs.data();
	for (size_t i = 0; i < len1d; ++i)
	{
		pBallisticImpactProbs[i] = fatalityProbability(
			1e6, 34, vel2ke(ballisticImpactVelocities(i), uasMass), 0.3);
	}

	const Matrix ballisticFatalityRisk = ballisticStrikeRiskMap * ballisticImpactProbs;
	initLayer("Ballistic Fatality Risk");
	get("Ballistic Fatality Risk") = ballisticFatalityRisk;
}

void ugr::risk::RiskMap::addPointStrikeMap(const gridmap::Index& index)
{
	const auto& windVelX = weather.at("Wind VelX", index);
	const auto& windVelY = weather.at("Wind VelY", index);
	const Vector2d wind{windVelX, windVelY};

	// Create samples of state distributions
	const auto& altitude = stateModel.getAltitude();
	const auto& velX = stateModel.velocity(0);
	const auto& velY = stateModel.velocity(1);
	auto altDist = std::normal_distribution<double>(altitude, 5);
	auto velXDist = std::normal_distribution<double>(velX, 5);
	auto velYDist = std::normal_distribution<double>(velY, 1);
	std::vector<double> altVect(nSamples);
	std::vector<double> velXVect(nSamples);
	std::vector<double> velYVect(nSamples);
	for (size_t i = 0; i < nSamples; ++i)
	{
		altVect[i] = altDist(generator);
		velXVect[i] = velXDist(generator);
		velYVect[i] = velYDist(generator);
	}

	// Create common heading rotation
	const Rotation2Dd headingRotation(
		util::bearing2Angle(DEG2RAD(stateModel.getHeading())));

	// Convert Index into vector for easy arithmetic later
	const Vector2d indexVec{index[0], index[1]};

	// Propagate samples through impact prediction
	const auto& glideImpactSamples = descentModel.glideImpact(altVect);
	const auto& ballisticImpactSamples =
		descentModel.ballisticImpact(altVect, velXVect, velYVect);
	util::Point2DVector glideImpactPoss(nSamples);
	util::Point2DVector ballisticImpactPoss(nSamples);

	GridMapDataType glideAngle = 0;
	GridMapDataType ballisticAngle = 0;
	GridMapDataType glideVelocity = 0;
	GridMapDataType ballisticVelocity = 0;

#pragma omp parallel for reduction(+:ballisticAngle, ballisticVelocity, glideAngle, glideVelocity)
	for (size_t i = 0; i < nSamples; ++i)
	{
		const auto& glideImpactSample = glideImpactSamples[i];
		glideImpactPoss[i] =
		((headingRotation *
			Vector2d(glideImpactSample.impactDistance, 0) +
			(glideImpactSample.impactTime * wind)) / xyRes) + indexVec;
		const auto& ballisticImpactSample = ballisticImpactSamples[i];

		ballisticImpactPoss[i] =
		((headingRotation *
			Vector2d(ballisticImpactSample.impactDistance, 0) +
			(ballisticImpactSample.impactTime * wind)) / xyRes) + indexVec;

		ballisticAngle += ballisticImpactSample.impactAngle;
		ballisticVelocity += ballisticImpactSample.impactVelocity;
		glideAngle += glideImpactSample.impactAngle;
		glideVelocity += glideImpactSample.impactVelocity;
	}

	// Fit a distribution to the propagated samples
	const auto glideDistParams = util::Gaussian2DFit(glideImpactPoss);
	const auto ballisticDistParams = util::Gaussian2DFit(ballisticImpactPoss);

	glideAngle /= nSamples;
	ballisticAngle /= nSamples;
	glideVelocity /= nSamples;
	ballisticVelocity /= nSamples;


#pragma omp critical
	{
		at("Glide Impact Angle", index) = glideAngle;
		at("Glide Impact Velocity", index) = glideVelocity;
		at("Ballistic Impact Angle", index) = ballisticAngle;
		at("Ballistic Impact Velocity", index) = ballisticVelocity;
	}

	// Matrix glideImpactRisk(sizeX, sizeY);
	// Matrix ballisticImpactRisk(sizeX, sizeY);

	// Iterate through all cells in the grid map
	Eigen::Vector<GridMapDataType, Dynamic> xs(sizeX * sizeY), ys(sizeX * sizeY);

	int i = 0;
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			xs[i] = x;
			ys[i] = y;
			++i;
		}
	}
	const Matrix glideImpactRisk = util::gaussian2D(xs, ys, glideDistParams).reshaped(sizeX, sizeY);
	const Matrix ballisticImpactRisk = util::gaussian2D(xs, ys, ballisticDistParams).reshaped(sizeX, sizeY);

	// #pragma omp parallel for default(shared), collapse(2)
	// 	for (int x = 0; x < sizeX; ++x)
	// 	{
	// 		for (int y = 0; y < sizeY; ++y)
	// 		{
	// 			glideImpactRisk(x, y) = util::gaussian2D(x, y, glideDistParams);
	// 			ballisticImpactRisk(x, y) = util::gaussian2D(x, y, ballisticDistParams);
	// 		}
	// 	}

	/* We have now evaluated the impact risk of the aircraft across the entire map*/
	/* Now we move onto the strike risk analysis */

	// These are used later
	const GridMapDataType pixelArea = pow(getResolution(), 2);
	const auto uasWidth = descentModel.width;
	const Matrix& populationDensityMap = get("Population Density");


	// Work out the lethal area of the aircraft when it crashes
	const auto& glideLethalArea = lethalArea(glideAngle, uasWidth);
	const auto& ballisticLethalArea = lethalArea(ballisticAngle, uasWidth);

	// This is a matrix version of the lethal area calculation
	// We only have a point to analyse, so this is reduced to a scalar
	// initLayer("Glide Lethal Area");
	// const Matrix& glideImpactAngles = get("Glide Impact Angle");
	// Matrix glideLethalAreas(getSize().x(), getSize().y());
	// auto* pGlideLethalAreas = glideLethalAreas.data();
	// for (size_t i = 0; i < len1d; ++i)
	// {
	// 	pGlideLethalAreas[i] = lethalArea(glideImpactAngles(i), uasWidth);
	// }

	const Matrix glideStrikeRisk = (glideImpactRisk.cwiseProduct(populationDensityMap) * glideLethalArea) / pixelArea;
	// const Matrix glideStrikeRisk = unScaledGlideStrikeRisk / pixelArea;


	// Work out the lethal area of the aircraft when it crashes


	//Same as above
	// initLayer("Ballistic Lethal Area");
	// const Matrix& ballisticImpactAngles = get("Ballistic Impact Angle");
	// Matrix ballisticLethalAreas(getSize().x(), getSize().y());
	// auto* pBallisticLethalAreas = ballisticLethalAreas.data();
	// for (size_t i = 0; i < len1d; ++i)
	// {
	// 	pBallisticLethalAreas[i] = lethalArea(ballisticImpactAngles(i), uasWidth);
	// }

	const Matrix ballisticStrikeRisk = (ballisticImpactRisk.cwiseProduct(populationDensityMap) * ballisticLethalArea) / pixelArea;
	// const Matrix ballisticStrikeRisk = unscaledBallisticStrikeRisk / pixelArea;


	// As we are only generating the strike risk from a single point,
	// this is summed across the entire strike risk map for that point
	// and set as the scalar value for the point it was generated for
#pragma omp critical
	{
		at("Glide Strike Risk", index) = glideStrikeRisk.sum();
		at("Ballistic Strike Risk", index) = ballisticStrikeRisk.sum();
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
	return 1 / (sqrt(alpha / beta)) *
		pow(beta / impactEnergy, 1 / (4 * shelterFactor));
}
