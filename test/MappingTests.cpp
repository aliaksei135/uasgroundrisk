/*
 * MappingTests
 *
 *  Created by A.Pilko on 11/04/2021.
 */

#include <gtest/gtest.h>
#include <Eigen/Dense>
// #include <matplotlibcpp.h>

// namespace mpl = matplotlibcpp;

TEST(MappingTests, BasicPlotTest)
{
	// mpl::plot({1, 3, 2, 4});
	// mpl::save("test_fig.png");
}

TEST(MappingTests, EigenSandbox)
{
	Eigen::MatrixXd mat(200, 200);
	mat.setZero();
	mat(100,100) = 1e-8;
	std::cout << "Max is: \n";
	std::cout << std::scientific << mat.maxCoeff();
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
