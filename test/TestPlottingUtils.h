#ifndef TESTPLOTTINGUTILS_H
#define TESTPLOTTINGUTILS_H
#include <Eigen/Dense>
#include <cmath>
#include <matplot/matplot.h>

using namespace matplot;

template <typename Scalar, typename Matrix>
static std::vector<std::vector<Scalar>> fromEigenMatrix(const Matrix& M)
{
    std::vector<std::vector<Scalar>> m;
    m.resize(M.rows(), std::vector<Scalar>(M.cols(), 0));
    for (size_t i = 0; i < m.size(); i++)
        for (size_t j = 0; j < m.front().size(); j++)
            m[i][j] = M(i, j);
    return m;
}

template <typename Scalar>
static void plotMat(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& m)
{
    const auto& vec = fromEigenMatrix<Scalar>(m);
    // fmesh([](double x, double y) { return sin(x) + cos(y); });
    image(vec);
    colormap(gca(), palette::viridis());
    colorbar().limits({m.minCoeff(), m.maxCoeff()});
    show();
}
#endif // TESTPLOTTINGUTILS_H
