/*
 * MappingTests
 *
 *  Created by A.Pilko on 11/04/2021.
 */

#include <TestPlottingUtils.h>
#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <cmath>
#include <matplot/matplot.h>

using namespace matplot;

// template<typename Scalar, typename Matrix>
// static std::vector< std::vector<Scalar> > fromEigenMatrix( const Matrix & M ){
// 	std::vector< std::vector<Scalar> > m;
// 	m.resize(M.rows(), std::vector<Scalar>(M.cols(), 0));
// 	for(size_t i = 0; i < m.size(); i++)
// 		for(size_t j = 0; j < m.front().size(); j++)
// 			m[i][j] = M(i,j);
// 	return m;
// }

TEST(MappingTests, BasicPlotTest)
{
    Eigen::MatrixXd mat(700,500);
    mat.setRandom();

    plotMat(mat);

    // const auto& vec = fromEigenMatrix<double, Eigen::MatrixXd>(mat);
    // // fmesh([](double x, double y) { return sin(x) + cos(y); });
    // image(vec);
    // show();
}

TEST(MappingTests, EigenSandbox)
{
    Eigen::MatrixXd mat(200, 200);
    mat.setZero();
    mat(100, 100) = 1e-8;
    std::cout << "Max is: \n";
    std::cout << std::scientific << mat.maxCoeff();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
