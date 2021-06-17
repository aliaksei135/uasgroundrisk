/*
 * AircraftModel.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "AircraftModel.h"
#include <algorithm>
#include <cassert>
#include <cmath>

using namespace ugr::risk;

#define GRAVITY_ACCEL 9.81
#define AIR_DENSITY 1.225

AircraftModel::AircraftModel(double mass, double width, double length,
                             double cruiseSpeed, double ballisticFrontalArea,
                             double ballisticDragCoeff, double glideAirspeed,
                             double glideRatio)
    : mass(mass), width(width), length(length), cruiseSpeed(cruiseSpeed),
      ballisticFrontalArea(ballisticFrontalArea),
      ballisticDragCoeff(ballisticDragCoeff), glideAirspeed(glideAirspeed),
      glideRatio(glideRatio) {
  c = 0.5 * ballisticFrontalArea * AIR_DENSITY * ballisticDragCoeff;
  gamma = sqrt((mass * GRAVITY_ACCEL) / c);
}

ugr::risk::ImpactDataStruct
ugr::risk::AircraftModel::glideImpact(double altitude) {
  auto distance = glideRatio * altitude;
  auto time = sqrt(pow(distance, 2) + pow(altitude, 2)) / glideAirspeed;
  auto angle = atan(1 / glideRatio) * (180 / M_PI);
  auto velocity = distance / time;
  return {distance, velocity, angle, time};
}
std::vector<ImpactDataStruct>
ugr::risk::AircraftModel::glideImpact(const std::vector<double> &altitude) {
  std::vector<ImpactDataStruct> out(altitude.size());
  for (size_t i = 0; i < altitude.size(); ++i) {
    out[i] = glideImpact(altitude[i]);
  }
  return out;
}
ugr::risk::ImpactDataStruct
ugr::risk::AircraftModel::ballisticImpact(double altitude, double velX,
                                          double velY) const {
  // This is essentially a port of
  // https://github.com/JARUS-QM/casex/blob/master/casex/ballistic_descent_models.py

  if (gamma < velY) {
    velY = fmin(gamma * 0.999, velY);
  }
  auto Hd = atanh(velY / gamma);
  auto Gd = -1. / 2 * log(1 + pow(velY, 2) / pow(gamma, 2));
  auto tTop = -gamma / GRAVITY_ACCEL * atan(velY / gamma);
  auto x1 = mass / c * log(1 + velX * c * tTop / mass);
  auto tC =
      (mass * (GRAVITY_ACCEL * tTop - gamma * Hd +
               velX * (1 + pow(Hd - GRAVITY_ACCEL / gamma * tTop, 2)))) /
      (mass * GRAVITY_ACCEL + velX * c * (GRAVITY_ACCEL * tTop - gamma * Hd));
  auto yT = (-1. / 2 * log(1 + pow(velY, 2) / pow(gamma, 2))) * mass / c;
  auto tD = gamma / GRAVITY_ACCEL *
            (acosh(exp(c * (altitude - yT) / mass + Gd)) - Hd);
  auto impactTime = tTop + tD;
  auto vxTop = velX / (1 + (tC * velX) / (mass / c));
  auto x2 =
      mass / c * log(1 + vxTop * c * (fmin(impactTime, tC) - tTop) / mass);
  auto vixC = velX / (1 + (tC * velX) / (mass / c));
  auto viyC = fmin(gamma * 0.999,
                   gamma * tanh(GRAVITY_ACCEL * (tC - tTop) / gamma + Hd));
  auto mx = fmax(0, impactTime - tC);
  auto x3 = vixC * exp(-1 / 2 * log(1 - pow(viyC, 2) / pow(gamma, 2))) * gamma /
            GRAVITY_ACCEL *
            (atan(sinh(GRAVITY_ACCEL * mx / gamma + (atanh(viyC / gamma)))) -
             asin(viyC / gamma));
  double vTx;
  if (impactTime > tC) {
    vTx =
        vixC * exp(-1 / 2 * log(1 - pow(viyC, 2) / pow(gamma, 2))) /
        cosh(GRAVITY_ACCEL * (impactTime - tC) / gamma + (atanh(viyC / gamma)));
  } else {
    vTx = velX / (1 + (impactTime * velX) / (mass / c));
  }
  auto vTy = gamma * tanh(GRAVITY_ACCEL * (impactTime - tTop) / gamma + Hd);

  auto impactDistance = x1 + x2 + x3;
  auto impactAngle = atan2(vTy, vTx) * (180 / M_PI);
  auto impactVelocity = sqrt(pow(vTx, 2) + pow(vTy, 2));

  return {impactDistance, impactVelocity, impactAngle, impactTime};
}
std::vector<ImpactDataStruct>
ugr::risk::AircraftModel::ballisticImpact(std::vector<double> altitude,
                                          std::vector<double> velX,
                                          std::vector<double> velY) {
  assert(altitude.size() == velX.size());
  assert(altitude.size() == velY.size());

  std::vector<ImpactDataStruct> out(altitude.size());
  for (size_t i = 0; i < altitude.size(); ++i) {
    out[i] = ballisticImpact(altitude[i], velX[i], velY[i]);
  }
  return out;
}
