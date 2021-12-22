/*
 * AircraftModel.cpp
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "uasgroundrisk/risk_analysis/aircraft/AircraftDescentModel.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace ugr::risk;

#define GRAVITY_ACCEL 9.81
#define AIR_DENSITY 1.225

AircraftDescentModel::AircraftDescentModel(const double mass, const double width,
                                           const double length, const double cruiseSpeed,
                                           const double ballisticFrontalArea,
                                           const double ballisticDragCoeff,
                                           const double glideAirspeed,
                                           const double glideRatio, const double parachuteDragCoeff,
                                           const double parachuteArea, const double parachuteDeployTime)
	: mass(mass), width(width), length(length), cruiseSpeed(cruiseSpeed),
	  ballisticFrontalArea(ballisticFrontalArea),
	  ballisticDragCoeff(ballisticDragCoeff), glideAirspeed(glideAirspeed),
	  glideRatio(glideRatio), parachuteDragCoeff(parachuteDragCoeff), parachuteArea(parachuteArea),
	  parachuteDeployTime(parachuteDeployTime),
	  c(0.5 * ballisticFrontalArea * AIR_DENSITY * ballisticDragCoeff),
	  gamma(sqrt((mass * GRAVITY_ACCEL) / c))
{
}

ugr::risk::ImpactDataStruct ugr::risk::AircraftDescentModel::glideImpact(const double altitude) const
{
	const auto distance = glideRatio * altitude;
	const auto time = sqrt(pow(distance, 2) + pow(altitude, 2)) / glideAirspeed;
	const auto angle = atan(1 / glideRatio) * (180 / M_PI);
	const auto velocity = distance / time;
	return {distance, velocity, angle, time};
}

std::vector<ImpactDataStruct> ugr::risk::AircraftDescentModel::glideImpact(
	const std::vector<double>& altitude) const
{
	std::vector<ImpactDataStruct> out(altitude.size());
	for (size_t i = 0; i < altitude.size(); ++i)
	{
		out[i] = glideImpact(altitude[i]);
	}
	return out;
}

ugr::risk::ImpactDataStruct ugr::risk::AircraftDescentModel::ballisticImpact(const double altitude, const double velX,
                                                                             double velZ) const
{
	// This is essentially a port of
	// https://github.com/JARUS-QM/casex/blob/master/casex/ballistic_descent_models.py

	if (gamma < velZ)
	{
		velZ = fmin(gamma * 0.999, velZ);
	}
	const auto Hd = atanh(velZ / gamma);
	const auto Gd = -1. / 2 * log1p(pow(velZ, 2) / pow(gamma, 2));
	const auto tTop = -gamma / GRAVITY_ACCEL * atan(velZ / gamma);
	const auto x1 = mass / c * log1p(velX * c * tTop / mass);
	const auto tC =
		(mass * (GRAVITY_ACCEL * tTop - gamma * Hd +
			velX * (1 + pow(Hd - GRAVITY_ACCEL / gamma * tTop, 2)))) /
		(mass * GRAVITY_ACCEL + velX * c * (GRAVITY_ACCEL * tTop - gamma * Hd));
	const auto yT = (-1. / 2 * log1p(pow(velZ, 2) / pow(gamma, 2))) * mass / c;
	const auto tD = gamma / GRAVITY_ACCEL *
		(acosh(exp(c * (altitude - yT) / mass + Gd)) - Hd);
	const auto impactTime = tTop + tD;
	const auto vxTop = velX / (1 + (tC * velX) / (mass / c));
	const auto x2 =
		mass / c * log1p(vxTop * c * (fmin(impactTime, tC) - tTop) / mass);
	const auto vixC = velX / (1 + (tC * velX) / (mass / c));
	const auto viyC = fmin(gamma * 0.999,
	                       gamma * tanh(GRAVITY_ACCEL * (tC - tTop) / gamma + Hd));
	const auto mx = fmax(0, impactTime - tC);
	const auto x3 = vixC * exp(-1. / 2 * log(1 - pow(viyC, 2) / pow(gamma, 2))) *
		gamma / GRAVITY_ACCEL *
		(atan(sinh(GRAVITY_ACCEL * mx / gamma + (atanh(viyC / gamma)))) -
			asin(viyC / gamma));
	double vTx;
	if (impactTime > tC)
	{
		vTx =
			vixC * exp(-1. / 2 * log(1 - pow(viyC, 2) / pow(gamma, 2))) /
			cosh(GRAVITY_ACCEL * (impactTime - tC) / gamma + (atanh(viyC / gamma)));
	}
	else
	{
		vTx = velX / (1 + (impactTime * velX) / (mass / c));
	}
	const auto vTy = gamma * tanh(GRAVITY_ACCEL * (impactTime - tTop) / gamma + Hd);

	const auto impactDistance = x1 + x2 + x3;
	const auto impactAngle = atan2(vTy, vTx) * (180 / M_PI);
	const auto impactVelocity = sqrt(pow(vTx, 2) + pow(vTy, 2));

	return {impactDistance, impactVelocity, impactAngle, impactTime};
}

std::vector<ImpactDataStruct> ugr::risk::AircraftDescentModel::ballisticImpact(
	const std::vector<double>& altitude, const std::vector<double>& velX,
	const std::vector<double>& velZ) const
{
	assert(altitude.size() == velX.size());
	assert(altitude.size() == velZ.size());

	std::vector<ImpactDataStruct> out(altitude.size());
	for (size_t i = 0; i < altitude.size(); ++i)
	{
		out[i] = ballisticImpact(altitude[i], velX[i], velZ[i]);
	}
	return out;
}

ImpactDataStruct AircraftDescentModel::parachuteImpact(const double altitude, const double velX) const
{
	/* Distance travelled before parachute fully deploys */
	const auto preDeployDistance = parachuteDeployTime * velX;

	/* Lateral velocity instantly reduces to zero after deploy */
	const auto dropTime = altitude * sqrt((parachuteArea * parachuteDragCoeff) / 2 * mass * GRAVITY_ACCEL);
	const auto impactVel = altitude / dropTime;
	constexpr auto impactAngle = 90; //degrees

	return {preDeployDistance, impactVel, impactAngle, dropTime};
}

std::vector<ImpactDataStruct> AircraftDescentModel::parachuteImpact(const std::vector<double>& altitude,
                                                                    const std::vector<double>& velX) const
{
	assert(altitude.size() == velX.size());

	std::vector<ImpactDataStruct> out(altitude.size());
	for (int i = 0; i < altitude.size(); ++i)
	{
		out[i] = parachuteImpact(altitude[i], velX[i]);
	}

	return out;
}
