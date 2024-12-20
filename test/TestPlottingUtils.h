#ifndef TESTPLOTTINGUTILS_H
#define TESTPLOTTINGUTILS_H
#include <Eigen/Dense>
#include <cmath>
#include "uasgroundrisk/gridmap/TypeDefs.h"
#ifdef UGR_PLOT_TESTS
#include <matplot/matplot.h>
using namespace matplot;
#endif

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

static void outputMat(const ugr::gridmap::Matrix& m,
                      const std::string& titleStr)
{
#ifdef UGR_PLOT_TESTS
    const auto min = m.minCoeff();
    const auto max = m.maxCoeff();

    const auto& vec = fromEigenMatrix<ugr::gridmap::GridMapDataType>(m);

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
#endif
}

#endif // TESTPLOTTINGUTILS_H
