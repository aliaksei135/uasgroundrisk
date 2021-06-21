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
#include <grid_map_core/iterators/GridMapIterator.hpp>
#include <iostream>
#include <omp.h>
#include <random>

ugr::risk::RiskMap::RiskMap(
    ugr::mapping::PopulationMap &populationMap,
    const ugr::risk::AircraftDescentModel aircraftDescent,
    const ugr::risk::AircraftStateModel aircraftState,
    const ugr::risk::WeatherMap weather)
    : descentModel(aircraftDescent), stateModel(aircraftState),
      weather(weather),
      GeospatialGridMap(populationMap.getBounds(),
                        static_cast<int>(populationMap.getResolution()),
                        "Risk Map") {
  // Evaluate population density map
  populationMap.eval();
  // Copy across only the the population density layer
  auto popDensityLayerName = "Population Density";
  initLayer(popDensityLayerName);
  get(popDensityLayerName) = populationMap.get(popDensityLayerName);
}

GridMap &ugr::risk::RiskMap::generateMap(
    const std::vector<ugr::risk::RiskType> &risksToGenerate) {
  if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
                RiskType::FATALITY) != risksToGenerate.end()) {
    generateImpactMap();
    generateStrikeMap();
    generateFatalityMap();
  } else if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
                       RiskType::STRIKE) != risksToGenerate.end()) {
    generateImpactMap();
    generateStrikeMap();
  } else if (std::find(risksToGenerate.begin(), risksToGenerate.end(),
                       RiskType::IMPACT) != risksToGenerate.end()) {
    generateImpactMap();
  }
  return *this;
}

void ugr::risk::RiskMap::eval() {
  generateImpactMap();
  generateStrikeMap();
  generateFatalityMap();
}

void ugr::risk::RiskMap::generateImpactMap() {
  // Init all layers
  initLayer("Glide Impact Risk");
  initLayer("Glide Impact Angle");
  initLayer("Glide Impact Velocity");

  initLayer("Ballistic Impact Risk");
  initLayer("Ballistic Impact Angle");
  initLayer("Ballistic Impact Velocity");

  // Create objects required for sample distribution generation
  // construct a trivial random generator engine from a time-based seed:
  auto seed = static_cast<unsigned int>(
      std::chrono::system_clock::now().time_since_epoch().count());
  std::default_random_engine generator(seed);

  int n = 0;

// Iterate through all cells in the grid map
#pragma omp parallel for
  for (grid_map::GridMapIterator iter(*this); !iter.isPastEnd(); ++iter) {
    // TODO: pacakge this as a CUDA function
    //  Get the position of this cell
    grid_map::Position pos;
    getPosition(*iter, pos);

    // Get the wind at this cell
    auto windVelX = weather.at("Wind VelX", *iter);
    auto windVelY = weather.at("Wind VelY", *iter);
    Eigen::Vector2d wind{windVelX, windVelY};

    // Create samples of state distributions
    auto altitude = stateModel.getAltitude();
    auto velX = stateModel.velocity(0);
    auto velY = stateModel.velocity(1);
    auto nSamples = 300;
    std::normal_distribution<double> altDist =
        std::normal_distribution<double>(altitude, 5);
    std::normal_distribution<double> velXDist =
        std::normal_distribution<double>(velX, 5);
    std::normal_distribution<double> velYDist =
        std::normal_distribution<double>(velY, 1);
    std::vector<double> altVect(nSamples);
    std::vector<double> velXVect(nSamples);
    std::vector<double> velYVect(nSamples);
    for (size_t i = 0; i < nSamples; ++i) {
      altVect[i] = altDist(generator);
      velXVect[i] = velXDist(generator);
      velYVect[i] = velYDist(generator);
    }
    assert(altitude > 0);

    // Create common heading rotation
    Eigen::Rotation2Dd headingRotation(
        ugr::util::bearing2Angle(DEG2RAD(stateModel.getHeading())));

    // Propagate samples through glide impact prediction
    const auto glideImpactSamples = descentModel.glideImpact(altVect);
    const auto ballisticImpactSamples =
        descentModel.ballisticImpact(altVect, velXVect, velYVect);
    ugr::util::Point2DVector glideImpactPoss(nSamples);
    ugr::util::Point2DVector ballisticImpactPoss(nSamples);
    for (size_t i = 0; i < nSamples; ++i) {
      auto glideImpactSample = glideImpactSamples[i];
      glideImpactPoss[i] =
          (headingRotation *
               Eigen::Vector2d(glideImpactSample.impactDistance, 0) +
           (glideImpactSample.impactTime * wind)) +
          pos;
      auto ballisticImpactSample = ballisticImpactSamples[i];
      ballisticImpactPoss[i] =
          (headingRotation *
               Eigen::Vector2d(ballisticImpactSample.impactDistance, 0) +
           (ballisticImpactSample.impactTime * wind)) +
          pos;
    }

    // Fit a distribution to the propagated samples
    auto glideDistParams = ugr::util::Gaussian2DFit(glideImpactPoss);
    auto ballisticDistParams = ugr::util::Gaussian2DFit(ballisticImpactPoss);

    auto *pGlideImpactRisk = get("Glide Impact Risk").data();
    auto *pBallisticImpactRisk = get("Ballistic Impact Risk").data();

    int j = 0;
    for (grid_map::GridMapIterator jter(*this); !jter.isPastEnd(); ++jter) {
      grid_map::Position jPos;
      getPosition(*jter, jPos);
      // Write direct to pointer to skip bounds checking etc.
      pGlideImpactRisk[j] +=
          ugr::util::gaussian2D(pos.x(), pos.y(), glideDistParams);
      pBallisticImpactRisk[j] +=
          ugr::util::gaussian2D(pos.x(), pos.y(), ballisticDistParams);
      ++j;
    }
#ifndef NDEBUG
    std::cout << n << std::endl;
    ++n;
#endif
  }
}

void ugr::risk::RiskMap::initLayer(const std::string &layerName) {
  add(layerName, 0);
  get(layerName).setZero();
}

void ugr::risk::RiskMap::generateStrikeMap() {
  const auto pixelArea = pow(getResolution(), 2);
  const auto uasWidth = descentModel.width;
  const auto populationDensityMap = get("Population Density");

  const auto size = getSize();
  const auto len1d = size.x() * size.y();

  const auto glideImpactRiskMap = get("Glide Impact Risk");
  initLayer("Glide Lethal Area");
  auto *pGlideImpactAngles = get("Glide Impact Angle").data();
  Eigen::MatrixXf glideLethalAreas(getSize().x(), getSize().y());
  auto *pGlideLethalAreas = glideLethalAreas.data();
  for (size_t i = 0; i < len1d; ++i) {
    pGlideLethalAreas[i] = lethalArea(pGlideImpactAngles[i], uasWidth);
  }

  Eigen::MatrixXf glideStrikeRisk(size.x(), size.y());
  glideStrikeRisk =
      (glideImpactRiskMap * populationDensityMap * glideLethalAreas) /
      pixelArea;
  initLayer("Glide Strike Risk");
  get("Glide Strike Risk") = glideStrikeRisk;

  const auto ballisticImpactRiskMap = get("Ballistic Impact Risk");
  initLayer("Ballistic Lethal Area");
  auto *pBallisticImpactAngles = get("Ballistic Impact Angle").data();
  Eigen::MatrixXf ballisticLethalAreas(getSize().x(), getSize().y());
  auto *pBallisticLethalAreas = ballisticLethalAreas.data();
  for (size_t i = 0; i < len1d; ++i) {
    pBallisticLethalAreas[i] = lethalArea(pBallisticImpactAngles[i], uasWidth);
  }

  Eigen::MatrixXf ballisticStrikeRisk(size.x(), size.y());
  ballisticStrikeRisk =
      (ballisticImpactRiskMap * populationDensityMap * ballisticLethalAreas) /
      pixelArea;
  initLayer("Ballistic Strike Risk");
  get("Ballistic Strike Risk") = ballisticStrikeRisk;
}

void ugr::risk::RiskMap::generateFatalityMap() {
  const auto uasMass = descentModel.mass;

  const auto size = getSize();
  const auto len1d = size.x() * size.y();

  const auto glideStrikeRiskMap = get("Glide Strike Risk");
  initLayer("Glide Impact Energy");
  auto *pGlideImpactVelocities = get("Glide Impact Velocity").data();
  Eigen::MatrixXf glideImpactProbs(getSize().x(), getSize().y());
  auto *pGlideImpactProbs = glideImpactProbs.data();
  for (size_t i = 0; i < len1d; ++i) {
    pGlideImpactProbs[i] = fatalityProbability(
        1e6, 34, vel2ke(pGlideImpactVelocities[i], uasMass), 0.3);
  }

  Eigen::MatrixXf glideFatalityRisk(size.x(), size.y());
  glideFatalityRisk = glideStrikeRiskMap * glideImpactProbs;
  initLayer("Glide Fatality Risk");
  get("Glide Fatality Risk") = glideFatalityRisk;

  const auto ballisticStrikeRiskMap = get("Ballistic Strike Risk");
  initLayer("Ballistic Impact Energy");
  auto *pBallisticImpactVelocities = get("Ballistic Impact Velocity").data();
  Eigen::MatrixXf ballisticImpactProbs(getSize().x(), getSize().y());
  auto *pBallisticImpactProbs = ballisticImpactProbs.data();
  for (size_t i = 0; i < len1d; ++i) {
    pBallisticImpactProbs[i] = fatalityProbability(
        1e6, 34, vel2ke(pBallisticImpactVelocities[i], uasMass), 0.3);
  }

  Eigen::MatrixXf ballisticFatalityRisk(size.x(), size.y());
  ballisticFatalityRisk = ballisticStrikeRiskMap * ballisticImpactProbs;
  initLayer("Ballistic Fatality Risk");
  get("Ballistic Fatality Risk") = ballisticFatalityRisk;
}

double ugr::risk::RiskMap::lethalArea(double impactAngle, double uasWidth) {
  auto personRadius = 1;
  auto personHeight = 1;
  auto uasRadius = uasWidth / 2;
  return ((2 * personHeight * (personRadius + uasRadius)) /
          (tan(impactAngle))) +
         (M_PI * pow(uasRadius + personRadius, 2));
}

double ugr::risk::RiskMap::vel2ke(double velocity, double mass) {
  return 0.5 * mass * pow(velocity, 2);
}

double ugr::risk::RiskMap::fatalityProbability(double alpha, double beta,
                                               double impactEnergy,
                                               double shelterFactor) {
  return 1 / (sqrt(alpha / beta)) *
         pow(beta / impactEnergy, 1 / (4 * shelterFactor));
}