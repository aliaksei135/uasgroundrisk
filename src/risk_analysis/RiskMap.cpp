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

	initLayer("Parachute Strike Risk");
	initLayer("Parachute Fatality Risk");
	initLayer("Parachute Impact Angle");
	initLayer("Parachute Impact Velocity");
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
	std::vector<GridMapDataType> impactAngles, impactVelocities;
	std::vector<Matrix, aligned_allocator<GridMapDataType>> impactPDFs;

	makePointImpactMap(index, impactPDFs, impactAngles, impactVelocities);


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
		const auto letArea = lethalArea(impactAngles[i], uasWidth);
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

void ugr::risk::RiskMap::makePointImpactMap(const gridmap::Index& index,
                                            std::vector<gridmap::Matrix, aligned_allocator<GridMapDataType>>&
                                            impactPDFs,
                                            std::vector<GridMapDataType>& impactAngles,
                                            std::vector<GridMapDataType>& impactVelocities)
{
	const auto& windVelX = weather.at("Wind VelX", index);
	const auto& windVelY = weather.at("Wind VelY", index);
	auto windXVelDist = std::normal_distribution<double>(windVelX, 5);
	auto windYVelDist = std::normal_distribution<double>(windVelY, 0.5);
	std::vector<Eigen::Vector2d, Eigen::aligned_allocator<double>> windVect(nSamples);
	// const Vector2d windMeans{windVelX, windVelY};

	// Create samples of state distributions
	const auto& altitude = aircraftModel.state.getAltitude();
	const auto& lateralVel = sqrt(pow(aircraftModel.state.velocity(0), 2) + pow(aircraftModel.state.velocity(1), 2));
	const auto& verticalVel = aircraftModel.state.velocity(2);
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
		util::bearing2Angle(DEG2RAD(aircraftModel.state.getHeading())));

	// Convert Index into vector for easy arithmetic later
	const Vector2d indexVec{index[0], index[1]};

	std::vector<util::Gaussian2DParamVector> descentDistrParams;

	for (const auto& descentModel : aircraftModel.descents)
	{
		const auto& samples = descentModel->impact(altVect, lateralVelVect, verticalVelVect);

		util::Point2DVector impactPositions(nSamples);
		double impactAngle = 0, impactVelocity = 0;

		// Model the descents of each of the random samples for LoC state vector to find an equal
		// number of ground impact samples we can fit distributions to.
#pragma omp parallel for reduction(+:impactAngle, impactVelocity)
		for (int i = 0; i < nSamples; ++i)
		{
			const auto& sample = samples[i];

			// As the heading rotation is an angle not a bearing, it is measured counter clockwise from
			// the x axis corresponding to the geospatial gridmap axes.
			// Therefore a zero rotation should correspond to motion in the x axis only, hence the y=0 here
			const Vector2d dist1D(sample.impactDistance, 0);

			impactPositions[i] =
				((headingRotation * dist1D + (sample.impactTime * windVect[i])) / xyRes) + indexVec;


			impactAngle += sample.impactAngle;
			impactVelocity += sample.impactVelocity;
		}

		// Fit a distribution to the propagated samples
		auto distParams = util::Gaussian2DFit(impactPositions);
		impactAngle /= nSamples;
		impactVelocity /= nSamples;

		descentDistrParams.emplace_back(distParams);
		impactAngles.emplace_back(impactAngle);
		impactVelocities.emplace_back(impactVelocity);
	}

	//TODO: This evaluation grid can be pregenerated
	// Iterate through all cells in the grid map
	Eigen::Vector<GridMapDataType, Dynamic> xs(this->sizeX * this->sizeY), ys(this->sizeX * this->sizeY);
	int i = 0;
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			// Invert x in line with the axes convention chosen here
			xs[i] = sizeX - x;
			ys[i] = y;
			++i;
		}
	}

	for (auto& distParams : descentDistrParams)
	{
		// Set amplitudes to 1 to avoid zero division errors when normalising to PDF later
		distParams[0] = 1;

		// Fit 2D gaussian kernels to the descent model samples instead of propagating the samples all the way to strike risk.
		// This should account for a more accurate probabilistic picture of the risk.
		Matrix impactPDFGrid = util::gaussian2D(xs, ys, distParams).reshaped<RowMajor>(sizeX, sizeY);

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
