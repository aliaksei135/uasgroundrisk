/*
 * AircraftStateModel.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTSTATEMODEL_H_
#define UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTSTATEMODEL_H_

#include <Eigen/Dense>
#include <cmath>

using namespace Eigen;

namespace ugr {
namespace risk {
/**
 * A class encapsulating the current state of an aircraft
 */
class AircraftStateModel {
public:
  Vector3d position;
  Vector3d velocity;

  /**
   * Return the heading of the aircraft in degrees
   * @return
   */
  double getHeading() const {
    return fmod((180 / M_PI) * atan2(velocity(0), velocity(1)) + 360, 360);
  }

  /**
   * Return the altitude of the aircraft in metres
   * @return
   */
  double getAltitude() const { return position(2); }
};
} // namespace risk
} // namespace ugr
#endif // UASGROUNDRISK_SRC_RISK_ANALYSIS_AIRCRAFT_AIRCRAFTSTATEMODEL_H_
