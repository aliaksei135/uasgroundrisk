#include <gtest/gtest.h>

#include "uasgroundrisk/map_gen/TemporalPopulationMap.h"
#include "uasgroundrisk/risk_analysis/RiskMap.h"
#include "uasgroundrisk/risk_analysis/aircraft/AircraftDescentModel.h"
#include "uasgroundrisk/risk_analysis/obstacles/ObstacleMap.h"
#include "uasgroundrisk/risk_analysis/weather/WeatherMap.h"
using namespace Eigen;

MatrixXf load_csv(const std::string& path)
{
    std::ifstream indata;
    indata.open(path);
    std::string line;
    std::vector<float> values;
    unsigned rows = 0;
    while (std::getline(indata, line))
    {
        std::stringstream lineStream(line);
        std::string cell;
        while (std::getline(lineStream, cell, ','))
        {
            values.push_back(std::stod(cell));
        }
        ++rows;
    }
    return Map<Matrix<float, Dynamic, Dynamic, RowMajor>>(values.data(), rows, values.size() / rows);
}

class GradientDescentTests : public ::testing::Test
{
public:
    GradientDescentTests()
    {
        std::ifstream file("pregen.csv");
        if (!file.is_open())
        {
            std::cout << "File not found" << std::endl;

            aircraft.state.position << 0, 0, 120;
            aircraft.state.velocity << 20, 0, 0;

            aircraft.mass = 50;
            aircraft.length = 5;
            aircraft.width = 5;
			aircraft.failureProb = 8e-3;

            aircraft.addDescentModel<ugr::risk::GlideDescentModel>(21, 15);
            aircraft.addDescentModel<ugr::risk::BallisticDescentModel>(25 * 0.3, 0.8);

            ugr::mapping::TemporalPopulationMap population(bounds, resolution);
            population.setHourOfDay(12);
            population.eval();

            ugr::risk::WeatherMap weather(bounds, resolution);
            weather.addConstantWind(5, 90);
            weather.eval();

            ugr::risk::ObstacleMap obstacleMap(bounds, resolution);
            obstacleMap.addBuildingHeights();
            obstacleMap.eval();

            ugr::risk::RiskMap riskMap(population, aircraft, obstacleMap, weather);
            riskMap.SetAnyHeading(true);
            riskMap.generateMap({ugr::risk::RiskType::FATALITY});
            fatalityMat = riskMap.get("Fatality Risk");

            const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");
            std::ofstream file("pregen.csv");
            if (file.is_open())
            {
                file << fatalityMat.format(CSVFormat);
                file.close();
            }
        }
        else
        {
            fatalityMat = load_csv("pregen.csv");
        }
    }

    std::array<float, 4> bounds{
        50.9065510f, -1.4500237f, 50.9517765f,
        -1.3419628f
    };
    int resolution = 60;
    ugr::risk::AircraftModel aircraft;

    // ugr::risk::RiskMap* riskMap;
    ugr::gridmap::Matrix fatalityMat;
};

TEST_F(GradientDescentTests, GradTest)
{
    // Size 132,200
    const int x = 20, y = 20;
    const auto lateralStep = resolution;
    const auto diagonalStep = std::sqrt(2 * resolution);

    const auto mVal = fatalityMat(x, y);

    //Rooks
    const auto tVal = fatalityMat(x, y + 1);
    const auto bVal = fatalityMat(x, y - 1);
    const auto lVal = fatalityMat(x - 1, y);
    const auto rVal = fatalityMat(x + 1, y);


    //Queens
    const auto trVal = fatalityMat(x + 1, y + 1);
    const auto brVal = fatalityMat(x + 1, y - 1);
    const auto blVal = fatalityMat(x - 1, y - 1);
    const auto tlVal = fatalityMat(x - 1, y + 1);


    auto sum = fatalityMat.sum();
    int i;
}
