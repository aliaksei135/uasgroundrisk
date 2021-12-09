/*
 * DataFitting.h
 *
 *  Created by A.Pilko on 20/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_UTILS_MATHS_DATAFITTING_H_
#define UASGROUNDRISK_SRC_UTILS_MATHS_DATAFITTING_H_

#include <Eigen/Core>
#include <Eigen/QR>
#include <Eigen/StdVector>
#include <cmath>

#include <unsupported/Eigen/NonLinearOptimization>
#include <vector>
#include <numeric>

namespace ugr
{
	namespace util
	{
		typedef Eigen::Vector<double, 7> Gaussian2DParamVector;
		typedef std::vector<Eigen::Vector2d, Eigen::aligned_allocator<Eigen::Vector2d>>
		Point2DVector;

		/**
		 * Evaluate a 2D rotated Gaussian distribution
		 *
		 * p_0: amplitude
		 * p_1: center coordinate x
		 * p_2: center coordinate y
		 * p_3: width x (standard deviation)
		 * p_4: width y (standard deviation)
		 * p_5: offset
		 * p_6: rotation angle [radians]
		 *
		 * @param x x location to evaluate
		 * @param y y location to evaluate
		 * @param p parameter array of size 7
		 * @return value of distribution at x,y
		 */
		static double gaussian2D(const double x, const double y, const Gaussian2DParamVector& p)
		{
			return p[0] *
				exp(-0.5 *
					(pow((x - p[1]) * cos(p[6]) - (y - p[2]) * sin(p[6]), 2) /
						pow(p[3], 2) +
						pow((x - p[1]) * sin(p[6]) - (y - p[2]) * cos(p[6]), 2) /
						pow(p[4], 2))) +
				p[5];
		}

		/**
		 * A vectorised version of above
		*/
		template <typename Derived>
		static Eigen::Vector<Derived, Eigen::Dynamic> gaussian2D(const Eigen::Vector<Derived, Eigen::Dynamic>& x,
		                                                         Eigen::Vector<Derived, Eigen::Dynamic>& y,
		                                                         const Gaussian2DParamVector& p)
		{
			return p[0] * Eigen::exp(-0.5 *
				(Eigen::pow((x.array() - p[1]) * cos(p[6]) - (y.array() - p[2]) * sin(p[6]), 2) / (p[3] * p[3]) +
					Eigen::pow((x.array() - p[1]) * sin(p[6]) - (y.array() - p[2]) * cos(p[6]), 2) / (p[4] * p[4]))
			) + p[5];
		}
	} // namespace util
	namespace internal
	{
		typedef Eigen::Matrix<double, 7, 1> Gaussian2DParamVector;
		typedef std::vector<Eigen::Vector2d, Eigen::aligned_allocator<Eigen::Vector2d>>
		Point2DVector;

		// Generic functor
		template <typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>
		struct Functor
		{
			typedef _Scalar Scalar;

			enum { InputsAtCompileTime = NX, ValuesAtCompileTime = NY };

			typedef Eigen::Matrix<Scalar, InputsAtCompileTime, 1> InputType;
			typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, 1> ValueType;
			typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, InputsAtCompileTime>
			JacobianType;

			int m_inputs, m_values;

			Functor() : m_inputs(InputsAtCompileTime), m_values(ValuesAtCompileTime)
			{
			}

			Functor(int inputs, int values) : m_inputs(inputs), m_values(values)
			{
			}

			int inputs() const { return m_inputs; }
			int values() const { return m_values; }
		};

		struct Gaussian2DFunctor : Functor<double>
		{
			int operator()(const Gaussian2DParamVector& x, Eigen::VectorXd& fvec) const
			{
				// "a" in the model is x(0), and "b" is x(1)
				for (unsigned int i = 0; i < points.size(); ++i)
				{
					fvec(i) = ugr::util::gaussian2D(points[i](0), points[i](1), x);
				}

				return 0;
			}

			Point2DVector points;

			int inputs() const { return 7; }

			int values() const
			{
				return static_cast<int>(points.size());
			} // The number of observations
		};

		struct Gaussian2DNumericalDiff : Eigen::NumericalDiff<Gaussian2DFunctor>
		{
		};
	} // namespace internal

	namespace util
	{
		/**
		 * @brief Fit a 2D Gaussian kernel to a data using the Levenberg-Marquadt method.
		 * @warning This only fits a gaussian kernel and does NOT provide the parameters to a Probability Density Function (PDF). A PDF can be approximated by normalising by the sum of the evaluation grid.
		 * @param data vector of 2D points to fit to
		 * @return 
		 */
		static Gaussian2DParamVector Gaussian2DFit(Point2DVector data)
		{
			ugr::internal::Gaussian2DNumericalDiff functor;
			functor.points = data;
			Eigen::LevenbergMarquardt<ugr::internal::Gaussian2DNumericalDiff> lm(functor);

			//  Gaussian2DParamVector params;
			Eigen::VectorXd params(7);
			params.fill(1.0);
			// Set centre coord estimates
			Eigen::Vector2d centres =
				std::accumulate(data.begin(), data.end(), Eigen::Vector2d(0, 0)) /
				data.size();
			params(1) = centres(0);
			params(2) = centres(1);

			lm.parameters.xtol = 1e-18;
			lm.parameters.ftol = 1e-18;
			lm.minimize(params);

			return params;
		}
	} // namespace util
} // namespace ugr

#endif // UASGROUNDRISK_SRC_UTILS_MATHS_DATAFITTING_H_
