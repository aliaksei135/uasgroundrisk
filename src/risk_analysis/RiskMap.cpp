/*
 * RiskMap.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "uasgroundrisk/risk_analysis/RiskMap.h"
#include "../utils/DataFitting.h"
#include "../utils/VectorOperations.h"
#include <Eigen/Dense>
#include <cassert>
#include <iostream>
#include <omp.h>
#include <random>
#include <utility>


using namespace ugr::gridmap;

ugr::risk::RiskMap::RiskMap(
	mapping::PopulationMap& populationMap,
	const AircraftDescentModel& aircraftDescent,
	AircraftStateModel aircraftState,
	const WeatherMap& weather)
	: GeospatialGridMap(populationMap.getBounds(),
						static_cast<int>(populationMap.getResolution())), descentModel(aircraftDescent),
	  stateModel(std::move(aircraftState)),
	  weather(weather),
	  generator(std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()))
{
	// Evaluate population density map
	populationMap.eval();
	// Copy across only the the population density layer
	constexpr auto popDensityLayerName = "Population Density";
	initLayer(popDensityLayerName);
	get(popDensityLayerName) = populationMap.get(popDensityLayerName);

	// Create objects required for sample distribution generation
	// construct a trivial random generator engine from a time-based seed:

	initRiskMapLayers();
}

ugr::gridmap::GridMap& ugr::risk::RiskMap::generateMap(
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
	GridMapDataType glideAngle = 0;
	GridMapDataType ballisticAngle = 0;
	GridMapDataType glideVelocity = 0;
	GridMapDataType ballisticVelocity = 0;

	Matrix glideImpactRisk(sizeX, sizeY);
	Matrix ballisticImpactRisk(sizeX, sizeY);

	makePointImpactMap(index, glideImpactRisk, ballisticImpactRisk, glideAngle, glideVelocity, ballisticAngle,
					   ballisticVelocity);

	// Synchronise writing to the common gridmap
#pragma omp critical
	{
		at("Glide Impact Angle", index) = glideAngle;
		at("Glide Impact Velocity", index) = glideVelocity;
		at("Ballistic Impact Angle", index) = ballisticAngle;
		at("Ballistic Impact Velocity", index) = ballisticVelocity;
	}

	/* We have now evaluated the impact risk of the aircraft across the entire map*/
	/* Now we move onto the strike risk analysis */

	// These are used later
	const GridMapDataType pixelArea = getResolution() * getResolution();
	const auto uasWidth = descentModel.width;
	// Get population map and convert from people/km^2 to people/m^2
	const Matrix& populationDensityMap = get("Population Density") * 1e-6;


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
	const Matrix ballisticStrikeRisk = (ballisticImpactRisk.cwiseProduct(populationDensityMap) * ballisticLethalArea) /
		pixelArea;

	// As we are only generating the strike risk from a single point,
	// this is summed across the entire strike risk map for that point
	// and set as the scalar value for the point it was generated for
#pragma omp critical
	{
		at("Glide Strike Risk", index) = glideStrikeRisk.sum();
		at("Ballistic Strike Risk", index) = ballisticStrikeRisk.sum();
	}
}

void ugr::risk::RiskMap::makePointImpactMap(const gridmap::Index& index, gridmap::Matrix& outGlide,
											gridmap::Matrix& outBallistic,
											GridMapDataType& outGlideAngle, GridMapDataType& outGlideVelocity,
											GridMapDataType& outBallisticAngle,
											GridMapDataType& outBallisticVelocity)
{
	assert(outGlide.size() == outBallistic.size());
	const Size size{outGlide.rows(), outGlide.cols()};

	const auto& windVelX = weather.at("Wind VelX", index);
	const auto& windVelY = weather.at("Wind VelY", index);
	auto windXVelDist = std::normal_distribution<double>(windVelX, 5);
	auto windYVelDist = std::normal_distribution<double>(windVelY, 0.5);
	std::vector<Eigen::Vector2d, Eigen::aligned_allocator<double>> windVect(nSamples);
	// const Vector2d windMeans{windVelX, windVelY};

	// Create samples of state distributions
	const auto& altitude = stateModel.getAltitude();
	const auto& lateralVel = sqrt(pow(stateModel.velocity(0), 2) + pow(stateModel.velocity(1), 2));
	const auto& verticalVel = stateModel.velocity(2);
	auto altDist = std::normal_distribution<double>(altitude, 5);
	auto lateralVelDist = std::normal_distribution<double>(lateralVel, 1.5);
	auto verticalVelDist = std::normal_distribution<double>(verticalVel, 0.5);
	std::vector<double> altVect(nSamples);
	std::vector<double> lateralVelVect(nSamples);
	std::vector<double> verticalVelVect(nSamples);
	for (size_t i = 0; i < nSamples; ++i)
	{
		altVect[i] = altDist(generator);
		lateralVelVect[i] = lateralVelDist(generator);
		verticalVelVect[i] = verticalVelDist(generator);
		windVect[i] = {windXVelDist(generator), windYVelDist(generator)};
	}

	// Create common heading rotation
	const Rotation2Dd headingRotation(
		util::bearing2Angle(DEG2RAD(stateModel.getHeading())));


	// Convert Index into vector for easy arithmetic later
	const Vector2d indexVec{index[0], index[1]};

	// Propagate samples through impact prediction
	const auto& glideImpactSamples = descentModel.glideImpact(altVect);
	const auto& ballisticImpactSamples =
		descentModel.ballisticImpact(altVect, lateralVelVect, verticalVelVect);
	util::Point2DVector glideImpactPoss(nSamples);
	util::Point2DVector ballisticImpactPoss(nSamples);

	double glideAngle = 0;
	double ballisticAngle = 0;
	double glideVelocity = 0;
	double ballisticVelocity = 0;

	// Model the descents of each of the random samples for LoC state vector to find an equal
	// number of ground impact samples we can fit distributions to.
#pragma omp parallel for reduction(+:ballisticAngle, ballisticVelocity, glideAngle, glideVelocity)
	for (int i = 0; i < nSamples; ++i)
	{
		const auto& glideImpactSample = glideImpactSamples[i];
		const auto& ballisticImpactSample = ballisticImpactSamples[i];

		// As the heading rotation is an angle not a bearing, it is measured counter clockwise from
		// the x axis corresponding to the geospatial gridmap axes.
		// Therefore a zero rotation should correspond to motion in the x axis only, hence the y=0 here
		const Vector2d glideDist1D(glideImpactSample.impactDistance, 0);
		const Vector2d ballisticDist1D(ballisticImpactSample.impactDistance, 0);

		glideImpactPoss[i] =
			((headingRotation * glideDist1D + (glideImpactSample.impactTime * windVect[i])) / xyRes) + indexVec;
		ballisticImpactPoss[i] =
			((headingRotation * ballisticDist1D + (ballisticImpactSample.impactTime * windVect[i])) / xyRes) + indexVec;

		ballisticAngle += ballisticImpactSample.impactAngle;
		ballisticVelocity += ballisticImpactSample.impactVelocity;
		glideAngle += glideImpactSample.impactAngle;
		glideVelocity += glideImpactSample.impactVelocity;
	}

	//TODO: The parameter vector can be memoised for a given set of input distribution means. Ignores the variance though?
	// Fit a distribution to the propagated samples
	auto glideDistParams = util::Gaussian2DFit(glideImpactPoss);
	auto ballisticDistParams = util::Gaussian2DFit(ballisticImpactPoss);

	// Find the impact state means instead of fitting a distribution to them again
	outGlideAngle = glideAngle / nSamples;
	outBallisticAngle = ballisticAngle / nSamples;
	outGlideVelocity = glideVelocity / nSamples;
	outBallisticVelocity = ballisticVelocity / nSamples;


	// Iterate through all cells in the grid map
	Eigen::Vector<GridMapDataType, Dynamic> xs(size[0] * size[1]), ys(size[0] * size[1]);


	int i = 0;
	for (int x = 0; x < size[0]; ++x)
	{
		for (int y = 0; y < size[1]; ++y)
		{
			// Invert x in line with the axes convention chosen here
			xs[i] = size[0] - x;
			ys[i] = y;
			++i;
		}
	}
	// Set amplitudes to 1 to avoid zero division errors when normalising to PDF later
	glideDistParams[0] = 1;
	ballisticDistParams[0] = 1;

	// Fit 2D gaussian kernels to the descent model samples instead of propagating the samples all the way to strike risk.
	// This should account for a more accurate probabilstic picture of the risk.
	outGlide = util::gaussian2D(xs, ys, glideDistParams).reshaped<RowMajor>(size[0], size[1]);
	outBallistic = util::gaussian2D(xs, ys, ballisticDistParams).reshaped<RowMajor>(size[0], size[1]);

	// Turn fitted impact risk gaussians into PDFs that we can use.
	// Work these out first to avoid aliasing Eigen expressions
	const auto glideQuot = outGlide.sum();
	const auto ballisticQuot = outBallistic.sum();
	outGlide /= glideQuot;
	outBallistic /= ballisticQuot;
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
