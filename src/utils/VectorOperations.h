/*
 * VectorOperations.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_UTILS_VECTOROPERATIONS_H_
#define UASGROUNDRISK_SRC_UTILS_VECTOROPERATIONS_H_

#include <cmath>
#include <eigen3/Eigen/Core>

namespace ugr {
namespace util {

static Eigen::Vector2d rotate2D(const Eigen::Vector2d &vect, double theta) {
  if (theta < 0) {
    theta = (2 * M_PI) - theta;
  }
  double c = cos(theta);
  double s = sin(theta);
  Eigen::Matrix2d R;
  R << c, -s, s, c;
  return R * vect;
}

} // namespace util
} // namespace ugr

#endif // UASGROUNDRISK_SRC_UTILS_VECTOROPERATIONS_H_
