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
#include <omp.h>

#include <unsupported/Eigen/NonLinearOptimization>
#include <vector>
#include <numeric>
#include <cfenv>

#define SQRT2PI 2.50662827463

namespace ugr
{
	namespace util
	{
		typedef Eigen::Vector<double, 7> Gaussian2DParamVector;
		typedef std::vector<Eigen::Vector2d, Eigen::aligned_allocator<Eigen::Vector2d>>
			Point2DVector;

		template<typename Type, int Dimensions>
		struct GaussianParams
		{
			Eigen::Matrix<Type, Dimensions, 1> means;
			Eigen::Matrix<Type, Dimensions, Dimensions> cov;

			GaussianParams(const Eigen::Matrix<Type, Dimensions, 1>& means,
				const Eigen::Matrix<Type, Dimensions, Dimensions>& cov)
				: means(means),
				  cov(cov)
			{
			}
		};

		template<typename MeansDerived, typename CovDerived, typename SamplesDerived>
		static Eigen::VectorXf gaussianND(
			const Eigen::MatrixBase<MeansDerived>& means,
			const Eigen::MatrixBase<CovDerived>& cov,
			const Eigen::MatrixBase<SamplesDerived>& pos)
		{
			typedef typename SamplesDerived::Scalar Type;
			const int NDimensions = means.rows();

			//TODO vectorise this, using something like colwise()?
			Eigen::Vector<Type, Eigen::Dynamic> out;
			out.resize(pos.cols());

			const double norm = std::pow(SQRT2PI, -NDimensions) * std::pow(std::abs(cov.determinant()), -0.5);

			for (int i = 0; i < pos.cols(); ++i)
			{
				const double quadform = (pos.col(i) - means).transpose() * cov.inverse() * (pos.col(i) - means);
				const double halfNegQuadform = -0.5 * quadform;
				// exp(-103) is already ~1e-45. Smaller than this cannot be represented as a float
				if (halfNegQuadform < -102)
				{
					out(i) = 0;
				}
				else
				{
					out(i) = norm * std::exp(halfNegQuadform);
				}
			}
			return std::move(out);
		}

		template<typename Type, int Dimensions, int Samples = Eigen::Dynamic>
		static GaussianParams<Type, Dimensions> fitGaussianParams(
			const Eigen::Matrix<Type, Dimensions, Samples>& pos)
		{
			// Set centre coord estimates from means of components
			Eigen::Vector<Type, Dimensions> means = pos.rowwise().mean();

			Eigen::Matrix<Type, Samples, 1> ones;
			ones.resize(pos.cols(), 1);
			ones.setOnes();
			Eigen::Matrix<Type, Dimensions, Dimensions> cov =
				(pos - (means * ones.transpose())) *
					(pos - (means * ones.transpose())).transpose() /
					(pos.cols() - 1);

			return { means, cov };
		}

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
		template<typename Derived>
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
		template<typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>
		struct Functor
		{
			typedef _Scalar Scalar;

			enum
			{
				InputsAtCompileTime = NX, ValuesAtCompileTime = NY
			};

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

			int inputs() const
			{
				return m_inputs;
			}
			int values() const
			{
				return m_values;
			}
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

			int inputs() const
			{
				return 7;
			}

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
		 * @return parameters of fit gaussian
		 */
		static Gaussian2DParamVector Gaussian2DFit(Point2DVector data)
		{
			ugr::internal::Gaussian2DNumericalDiff functor;
			functor.points = data;
			Eigen::LevenbergMarquardt<ugr::internal::Gaussian2DNumericalDiff> lm(functor);

			// Gaussian2DParamVector params;
			Eigen::VectorXd params(7);
			params.setConstant(1e-20);
			params(0) = 1;

			// Set centre coord estimates from means of components
			Eigen::Vector2d centres =
				std::accumulate(data.begin(), data.end(), Eigen::Vector2d(1e-30, 1e-30)) / data.size();
			params(1) = centres(0);
			params(2) = centres(1);

			// Set std dev estimates to sample std dev of each component distribution
			Eigen::Vector2d sqMeanDiffs =
				std::accumulate(data.begin(), data.end(), Eigen::Vector2d(1e-30, 1e-30),
					[centres](const Eigen::Vector2d& acc, const Eigen::Vector2d& p) -> Eigen::Vector2d
					{
					  const Eigen::Vector2d diff = p - centres;
					  return acc + diff.cwiseProduct(diff);
					});
			Eigen::Vector2d stdDevs = sqrt(sqMeanDiffs.array() / (data.size() - 1));
			params(3) = stdDevs[0];
			params(4) = stdDevs[1];

			lm.parameters.xtol = 1e-28;
			lm.parameters.ftol = 1e-28;
			lm.minimize(params);

			return params;
		}
	} // namespace util
} // namespace ugr

#endif // UASGROUNDRISK_SRC_UTILS_MATHS_DATAFITTING_H_
