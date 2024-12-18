/*
 * RiskMap.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "uasgroundrisk/risk_analysis/RiskMap.h"
#include "../utils/DataFitting.h"
#include "../utils/GeometryOperations.h"
#include "../utils/VectorOperations.h"
#include <chrono>
#include <omp.h>
#include <random>

#include "uasgroundrisk/risk_analysis/aircraft/AircraftModel.h"
#include "uasgroundrisk/risk_analysis/obstacles/ObstacleMap.h"

#include <spdlog/spdlog.h>

using namespace ugr::gridmap;

ugr::risk::RiskMap::RiskMap(mapping::PopulationMap& populationMap,
	const AircraftModel aircraftModel,
	ObstacleMap& obstacleMap, WeatherMap& weatherMap)
	: GeospatialGridMap(populationMap.getBounds(),
	static_cast<int>(populationMap.getResolution())),
	  aircraftModel(aircraftModel),
	  generator(std::default_random_engine(
		  std::chrono::system_clock::now().time_since_epoch().count()))
{
	spdlog::info("Constructing Riskmap");

	// Evaluate population density map
	populationMap.eval();
	// Check if building height layer already exists
	const auto obstacleLayers = obstacleMap.getLayers();
	if (std::find(obstacleLayers.begin(), obstacleLayers.end(),
		"Building Height") == obstacleLayers.end())
		obstacleMap.addBuildingHeights();
	// Evaluate obstacles
	obstacleMap.eval();
	weatherMap.eval();
	// Copy across only the the population density and building height layer
	initLayer("Population Density");
	initLayer("Building Height");
	initLayer("Wind VelX");
	initLayer("Wind VelY");
	// Get population map and convert from people/km^2 to people/m^2
	get("Population Density") = populationMap.get("Population Density") * 1e-6;
	get("Building Height") = obstacleMap.get("Building Height");
	get("Wind VelX") = weatherMap.get("Wind VelX");
	get("Wind VelY") = weatherMap.get("Wind VelY");

	// Create objects required for sample distribution generation
	// construct a trivial random generator engine from a time-based seed:

	initRiskMapLayers();

	// Generate eval grid once instead of on every iteration
	evalXs.resize(this->sizeX * this->sizeY);
	evalYs.resize(this->sizeX * this->sizeY);
	evalMat.resize(2, this->sizeX * this->sizeY);
	int i = 0;
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			// Invert x in line with the axes convention chosen here
			evalXs[i] = static_cast<GridMapDataType>(sizeX - x);
			evalYs[i] = static_cast<GridMapDataType>(y);

			evalMat(0, i) = sizeX - x;
			evalMat(1, i) = y;
			++i;
		}
	}
}

GridMap&
ugr::risk::RiskMap::generateMap(const std::vector<RiskType>& risksToGenerate)
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
	spdlog::info("Evaluating Riskmap");
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

	// Setting shelter factor to 0 results in infinite fatality risk, so we set a
	// small value instead
	add("Shelter Factor", 0.3);
}

void ugr::risk::RiskMap::initLayer(const std::string& layerName)
{
	add(layerName, 0);
	get(layerName).setZero();
}

void ugr::risk::RiskMap::generateStrikeMap()
{
	// Iterate through all cells in the grid map
#pragma omp parallel for collapse(2) schedule(dynamic) default(none)
	for (int x = 0; x < sizeX; ++x)
	{
		for (int y = 0; y < sizeY; ++y)
		{
			// TODO: package this as a CUDA function
			addPointStrikeMap({ x, y });
		}
	}
	add("Strike Risk", 0);
	for (const auto& descent : aircraftModel.descents)
	{
		const auto descentName = descent->getName();
		get("Strike Risk") += get(descentName + " Strike Risk");
	}
}

void ugr::risk::RiskMap::generateFatalityMap()
{
	const auto uasMass = aircraftModel.mass;

	const Matrix shelterFactorMap = get("Shelter Factor");

	for (const auto& descent : aircraftModel.descents)
	{
		const auto descentName = descent->getName();
		const Matrix& strikeRiskMap = get(descentName + " Strike Risk");
		const Matrix& impactVelocities = get(descentName + " Impact Velocity");
		// const Matrix& impactAngles = get(descentName + " Impact Angle");


		Matrix fatalityRisk(sizeX, sizeY);
		fatalityRisk = strikeRiskMap.cwiseProduct(fatalityProbability(
			1e6, 100, vel2ke(impactVelocities, uasMass), shelterFactorMap));

#pragma omp critical
		get(descentName + " Fatality Risk") = fatalityRisk;
	}

	add("Fatality Risk", 0);
	for (const auto& descent : aircraftModel.descents)
	{
		const auto descentName = descent->getName();
		get("Fatality Risk") += get(descentName + " Fatality Risk");
	}
}

void ugr::risk::RiskMap::addPointStrikeMap(const Index& index)
{
	std::vector<GridMapDataType> impactAngles, impactVelocities;
	std::vector<Matrix, aligned_allocator<Matrix>> impactPDFs;
	const auto& altitude = aircraftModel.state.getAltitude();
	const int& heading = anyHeading ? -1 : static_cast<int>(aircraftModel.state.getHeading());

	makePointImpactMap(index, altitude, heading, impactPDFs, impactAngles, impactVelocities);

	/* We have now evaluated the impact PDF of the aircraft*/
	/* Now we move onto the strike risk analysis */

	// These are used later
	const GridMapDataType pixelArea = getResolution() * getResolution();
	const auto uasWidth = aircraftModel.width;
	const Matrix& populationDensityMap = get("Population Density");

	for (int i = 0; i < aircraftModel.descents.size(); ++i)
	{
		// Work out the lethal area of the aircraft when it crashes
		const auto letArea = lethalArea(DEG2RAD(impactAngles[i]), uasWidth);
		const Matrix strikeRisk =
			(impactPDFs[i].cwiseProduct(populationDensityMap) * letArea) /
				pixelArea;
		const auto strikeRiskSum = static_cast<GridMapDataType>(aircraftModel.failureProb) * strikeRisk.sum();
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

void ugr::risk::RiskMap::makePointImpactMap(
	const Index& index,
	const double altitude,
	const int heading,
	std::vector<Matrix, aligned_allocator<Matrix>>& impactPDFs,
	std::vector<GridMapDataType>& impactAngles,
	std::vector<GridMapDataType>& impactVelocities)
{
	const auto& windVelX = at("Wind VelX", index);
	const auto& windVelY = at("Wind VelY", index);
	auto windXVelDist = std::normal_distribution<double>(windVelX, 0.5);
	auto windYVelDist = std::normal_distribution<double>(windVelY, 0.5);
	std::vector<Vector2d, aligned_allocator<Vector2d>> windVect(nSamples);

	// Create samples of state distributions
	const auto& lateralVel = sqrt(pow(aircraftModel.state.velocity(0), 2) +
		pow(aircraftModel.state.velocity(1), 2));
	const auto& verticalVel = aircraftModel.state.velocity(2);
	auto altDist = std::normal_distribution<double>(altitude, 2);
	auto lateralVelDist = std::normal_distribution<double>(lateralVel, 0.5);
	auto verticalVelDist = std::normal_distribution<double>(verticalVel, 0.5);

	// Generate random variable samples for LoC states
	std::vector<double> altVect(nSamples);
	std::vector<double> lateralVelVect(nSamples);
	std::vector<double> verticalVelVect(nSamples);
	std::vector<Rotation2Dd, aligned_allocator<Rotation2Dd>> headingVect(nSamples);
	if (heading < 0)
	{
		// Use uniformly distributed headings
		auto headingUniformDist =
			std::uniform_real_distribution<double>(DEG2RAD(0), DEG2RAD(360));
		for (size_t i = 0; i < nSamples; ++i)
		{
			altVect[i] = std::abs(altDist(generator));
			lateralVelVect[i] = lateralVelDist(generator);
			verticalVelVect[i] = verticalVelDist(generator);
			windVect[i] = { windXVelDist(generator), windYVelDist(generator) };
			headingVect[i] = Rotation2Dd(util::bearing2Angle(headingUniformDist(generator)));

		}
	}
	else
	{
		// Use actual heading with normal distribution
		auto headingNormalDist = std::normal_distribution<double>(
			DEG2RAD(heading % 360), DEG2RAD(5));
		for (size_t i = 0; i < nSamples; ++i)
		{
			altVect[i] = std::abs(altDist(generator));
			lateralVelVect[i] = lateralVelDist(generator);
			verticalVelVect[i] = verticalVelDist(generator);
			windVect[i] = { windXVelDist(generator), windYVelDist(generator) };
			headingVect[i] = Rotation2Dd(util::bearing2Angle(headingNormalDist(generator)));
		}
	}



	// // Create common heading rotation
	// const Rotation2Dd headingRotation(
	// 	util::bearing2Angle(DEG2RAD(aircraftModel.state.getHeading())));

	// Convert Index into vector for easy arithmetic later
	const Vector2d indexVec{ index[0], index[1] };

	// std::vector<util::Gaussian2DParamVector> descentDistrParams;
	// std::vector<util::GaussianParams<GridMapDataType, 2>> distParams;

	for (const auto& descentModel : aircraftModel.descents)
	{
		const auto& samples =
			descentModel->impact(altVect, lateralVelVect, verticalVelVect);

		// util::Point2DVector impactPositions(nSamples);

		double impactAngle = 0, impactVelocity = 0;
		Eigen::Matrix<GridMapDataType, 2, nSamples> impactSampleMat;
		impactSampleMat.setZero();

		// Model the descents of each of the random samples for LoC state vector to
		// find an equal number of ground impact samples we can fit distributions
		// to.
//#ifdef UGR_OMP_NESTED
//#pragma omp parallel for reduction(+ : impactAngle, impactVelocity)
//#endif
		for (int i = 0; i < nSamples; ++i)
		{
			const auto& sample = samples[i];

			// As the heading rotation is an angle not a bearing, it is measured
			// counter clockwise from the x axis corresponding to the geospatial
			// gridmap axes. Therefore a zero rotation should correspond to motion in
			// the x axis only, hence the y=0 here
			const Vector2d dist1D(sample.impactDistance, 0);

			// impactPositions[i] =
			// ((headingVect[i] * dist1D + (sample.impactTime * windVect[i])) / xyRes)
			// + indexVec;

			impactSampleMat.col(i) =
				(((headingVect[i] * dist1D + (sample.impactTime * windVect[i])) /
					xyRes) +
					indexVec)
					.cast<GridMapDataType>();

			impactAngle += sample.impactAngle;
			impactVelocity += sample.impactVelocity;
		}

		// Fit a distribution to the propagated samples for this descent type
		// auto distParams = util::Gaussian2DFit(impactPositions);

		const auto distParams =
			util::fitGaussianParams<float, 2, nSamples>(impactSampleMat);

		// Matrix impactPDFGrid = util::gaussian2D(evalXs, evalYs,
		// distParams).reshaped<RowMajor>(sizeX, sizeY);

		// Fit 2D gaussian kernels to the descent model samples instead of
		// propagating the samples all the way to strike risk. This should account
		// for a more accurate probabilistic picture of the risk.

//		std::cout << "covs " << distParams.cov << "\n";
		const Matrix impactPDFGrid = util::gaussianND(distParams.means, distParams.cov,
			evalMat.cast<GridMapDataType>())
			.reshaped<RowMajor>(sizeX, sizeY);
		const double pdfSum = impactPDFGrid.sum();

		// Turn fitted impact risk gaussians into PDFs that we can use.
		// Work these out first to avoid aliasing Eigen expressions

		Matrix normImpactPDFGrid = impactPDFGrid / pdfSum;
		normImpactPDFGrid.colwise().reverseInPlace();
		impactPDFs.emplace_back(normImpactPDFGrid);

		// descentDistrParams.emplace_back(distParams);
		impactAngles.emplace_back(impactAngle / nSamples);
		impactVelocities.emplace_back(impactVelocity / nSamples);
		//        buildingImpactProbs.emplace_back(buildingCollisionCount /
		//        nSamples);
	}

	// for (auto& distParams : descentDistrParams)
	// {
	//     // Set amplitudes to 1 to avoid zero division errors when normalising
	//     to PDF later distParams[0] = 1;
	//
	//     // Fit 2D gaussian kernels to the descent model samples instead of
	//     propagating the samples all the way to strike risk.
	//     // This should account for a more accurate probabilistic picture of the
	//     risk. Matrix impactPDFGrid = util::gaussian2D(evalXs, evalYs,
	//     distParams).reshaped<RowMajor>(sizeX, sizeY);
	//
	//     // Turn fitted impact risk gaussians into PDFs that we can use.
	//     // Work these out first to avoid aliasing Eigen expressions
	//     const auto pdfQuot = impactPDFGrid.sum();
	//     impactPDFGrid /= pdfQuot;
	//     impactPDFGrid.colwise().reverseInPlace();
	//     impactPDFs.emplace_back(impactPDFGrid);
	// }
}

double ugr::risk::RiskMap::lethalArea(const double impactAngle,
	const double uasWidth)
{
	constexpr auto personRadius = 1.5;
	constexpr auto personHeight = 2;
	const auto uasRadius = uasWidth / 2;
	return 2 * personHeight * (personRadius + uasRadius) / tan(impactAngle) +
		M_PI * pow(uasRadius + personRadius, 2);
}

double ugr::risk::RiskMap::vel2ke(const double velocity, const double mass)
{
	return 0.5 * mass * pow(velocity, 2);
}

double ugr::risk::RiskMap::fatalityProbability(const double alpha,
	const double beta,
	const double impactEnergy,
	const double shelterFactor)
{
	return 1
		/ (1 + ((std::sqrt(alpha / beta)) * std::pow((beta) / (impactEnergy), 1 / (4 * shelterFactor))));
}

ugr::gridmap::Matrix ugr::risk::RiskMap::lethalArea(const Matrix& impactAngle,
	const double uasWidth)
{
	constexpr auto personRadius = 1.5;
	constexpr auto personHeight = 2;
	const auto uasRadius = uasWidth / 2;
	return (2 * personHeight * (personRadius + uasRadius) /
		tan(impactAngle.array()) +
		M_PI * pow(uasRadius + personRadius, 2))
		.matrix();
}

ugr::gridmap::Matrix ugr::risk::RiskMap::vel2ke(const Matrix& velocity,
	const double mass)
{
	return (0.5 * mass * pow(velocity.array(), 2)).matrix();
}

ugr::gridmap::Matrix
ugr::risk::RiskMap::fatalityProbability(const double alpha, const double beta,
	const Matrix& impactEnergy,
	const Matrix& shelterFactor)
{
	return (1
		/ (1 + ((std::sqrt(alpha / beta))
			* Eigen::pow((beta) / (impactEnergy.array()), 1 / (4 * shelterFactor.array()))))).matrix();
}
