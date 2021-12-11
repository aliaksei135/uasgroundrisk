/*
 * VectorOperations.h
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_UTILS_VECTOROPERATIONS_H_
#define UASGROUNDRISK_SRC_UTILS_VECTOROPERATIONS_H_

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>

namespace ugr
{
	namespace util
	{
#define DEG2RAD(ang) (ang * (M_PI / 180))
#define RAD2DEG(ang) (ang * (180 / M_PI))

		/**
		 * Rotate a 2-vector by an angle theta
		 * @param vect the vector to rotate
		 * @param theta angle to rotate anticlockwise in radians.
		 * @return
		 */
		template <typename T>
		static Eigen::Vector<T, 2> rotate2D(const Eigen::Vector<T, 2>& vect, const T theta)
		{
			//  if (theta < 0) {
			//    theta = (2 * M_PI) - theta;
			//  }
			//  const double c = cos(theta);
			//  const double s = sin(theta);
			//  Eigen::Matrix2d R;
			//  R << c, -s, s, c;
			const Eigen::Rotation2D<T> rot(theta);
			return rot * vect;
			//  return R * vect;
		}

		/**
		 * Convert a bearing/heading to an angle from the +ve x axis in standard x-y
		 * axes
		 * @param bearing in radians
		 * @return angle in radians
		 */
		template <typename T>
		static T bearing2Angle(const T bearing)
		{
			return fmod(2 * M_PI - (bearing - (0.5 * M_PI)), 2 * M_PI);
		}
	} // namespace util
} // namespace ugr

#endif // UASGROUNDRISK_SRC_UTILS_VECTOROPERATIONS_H_
