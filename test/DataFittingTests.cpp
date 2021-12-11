#include <fstream>
#include <gtest/gtest.h>
#include "../src/utils/DataFitting.h"
#include "TestData.h"
#include <Eigen/Dense>

using namespace Eigen;

TEST(DataFittingTests, Gaussian2DVectorisedFuncTest)
{
	ugr::util::Gaussian2DParamVector param;
	param << 1, 20, 20, 4, 4, 0, 0;

	constexpr int xSize = 40, ySize = 40;

	VectorXd xs(xSize * ySize), ys(xSize * ySize);

	int i = 0;
	for (int x = 0; x < xSize; ++x)
	{
		for (int y = 0; y < ySize; ++y)
		{
			xs[i] = x;
			ys[i] = y;
			++i;
		}
	}

	const MatrixXd out = ugr::util::gaussian2D(xs, ys, param).reshaped(xSize, ySize);

	// Assert that the max is where we expect
	double mx, my;
	const double mval = out.maxCoeff(&mx, &my);
	ASSERT_EQ(mx, 20);
	ASSERT_EQ(my, 20);
	ASSERT_NEAR(mval, 1, 1e-2);

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	std::ofstream file("gaussian_vectorised_test.csv");
	if (file.is_open())
	{
		file << out.format(CSVFormat);
		file.close();
	}
}

TEST(DataFittingTests, Gaussian2DFuncTest)
{
	ugr::util::Gaussian2DParamVector param;
	param << 1, 20, 20, 4, 4, 0, 0;

	Eigen::Matrix<double, 40, 40> out;
	for (int x = 0; x < 40; ++x)
	{
		for (int y = 0; y < 40; ++y)
		{
			out(x, y) = ugr::util::gaussian2D(x, y, param);
		}
	}

	// Assert that the max is where we expect
	double mx, my;
	const double mval = out.maxCoeff(&mx, &my);
	ASSERT_EQ(mx, 20);
	ASSERT_EQ(my, 20);
	ASSERT_NEAR(mval, 1, 1e-16);

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	std::ofstream file("gaussian_scalar_test.csv");
	if (file.is_open())
	{
		file << out.format(CSVFormat);
		file.close();
	}
}


TEST(DataFittingTests, Gaussian2DFitTest)
{
	ugr::util::Point2DVector data = testSamples;

	auto param = ugr::util::Gaussian2DFit(data);

	ASSERT_NEAR(param[0], 0, 1e-16);
	//Assert means
	ASSERT_NEAR(param[1], 50, 1);
	ASSERT_NEAR(param[2], 50, 1);
	//Assert std dev
	ASSERT_NEAR(param[3], 5, 0.1);
	ASSERT_NEAR(param[4], 4.7, 0.1);
	//Assert no offset
	ASSERT_NEAR(param[5], 0, 1e-16);
	//Assert rotation angle
	ASSERT_NEAR(param[6], 0, 1e-2);


	Eigen::Matrix<double, 100, 100> out;
	out.setZero();
	for (int x = 0; x < out.cols(); ++x)
	{
		for (int y = 0; y < out.rows(); ++y)
		{
			out(x, y) = ugr::util::gaussian2D(x, y, param);
		}
	}
	// Turn into PDF
	const auto quot = out.sum();
	out /= quot;

	// Assert that the max is where we expect
	double mx, my;
	out.maxCoeff(&mx, &my);
	ASSERT_EQ(mx, 50);
	ASSERT_EQ(my, 50);

	// PDFs should sum to 1 by definition
	ASSERT_NEAR(out.sum(), 1, 1e-2);


	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	std::ofstream file("gaussian_fit_test.csv");
	if (file.is_open())
	{
		file << out.format(CSVFormat);
		file.close();
	}
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
