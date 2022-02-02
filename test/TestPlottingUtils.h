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
static void outputMat(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& m, const std::string& titleStr)
{
    const auto min = m.minCoeff();
    const auto max = m.maxCoeff();

    const auto& vec = fromEigenMatrix<Scalar>(m);

    auto fig = figure(true);
    auto cax = fig->current_axes();
    auto img = image(cax, vec);
    title(cax, titleStr);
    xlabel(cax, "X");
    ylabel(cax, "Y");

    if (min != max)
    {
        colormap(cax, palette::viridis());
        colorbar(cax).limits({min, max});
    }

    // save(fig, "fig/" + titleStr + ".eps");
}

#endif // TESTPLOTTINGUTILS_H
