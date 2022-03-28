#include <memory>

#include "uasgroundrisk/risk_analysis/RiskMap.h"
#include <gtest/gtest.h>
#include <fstream>

#include "uasgroundrisk/map_gen/osm/OSMTag.h"
#include "TestPlottingUtils.h"
#include "uasgroundrisk/map_gen/TemporalPopulationMap.h"

using namespace ugr::risk;
using namespace ugr::mapping::osm;

class TemporalRiskMapTests : public testing::Test
{
protected:
    void SetUp() override
    {
        aircraft.state.position << 0, 0, 120;
        aircraft.state.velocity << 20, 0, 0;

        aircraft.mass = 30;
        aircraft.length = 5;
        aircraft.width = 5;

        aircraft.addDescentModel<GlideDescentModel>(21, 15);
        aircraft.addDescentModel<BallisticDescentModel>(25 * 0.3, 0.8);
    }

    std::array<float, 4> bounds{
        50.9065510f, -1.4500237f, 50.9517765f,
        -1.3419628f
    };
    // std::array<float, 4> bounds{
    // 50.689f, -1.5f, 51.0f,
    // -0.88f
    // };
    int resolution = 60;
    AircraftModel aircraft;
};

TEST_F(TemporalRiskMapTests, SchoolsStrikeRiskMapTest)
{
    // ugr::mapping::TemporalPopulationMap population(bounds, resolution);
    // population.setHourOfDay(12);
    // population.eval();

    // Assert the population map actually generated something otherwise
    // this test is pointless and equivalent to the zero* tests
    // ASSERT_NE(population.get("Population Density").maxCoeff(), 0);

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    ObstacleMap obstacleMap(bounds, resolution);
    obstacleMap.addBuildingHeights();
    obstacleMap.eval();

    RiskMap riskMap(new ugr::mapping::TemporalPopulationMap(bounds, resolution), aircraft, obstacleMap,
                    weather);
    riskMap.SetAnyHeading(true);
    auto strikeMap = riskMap.generateMap({RiskType::FATALITY});

    ugr::gridmap::Matrix& glideRisk = strikeMap.get("Glide Strike Risk");
    ugr::gridmap::Matrix& ballisticRisk = strikeMap.get("Ballistic Strike Risk");

    const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

    const auto layers = strikeMap.getLayers();
    for (const auto& layer : layers)
    {
        std::cout << layer << " max " << std::scientific << strikeMap.get(layer).maxCoeff()
            << std::endl;

        std::ofstream file("temporal_fatality_map_" + layer + ".csv");
        if (file.is_open())
        {
            file << strikeMap.get(layer).format(CSVFormat);
            file.close();
        }

        outputMat(strikeMap.get(layer), testing::UnitTest::GetInstance()->current_test_info()->name() + layer);
    }

    // As the population map is zero, this must all be zero as well
    ASSERT_NE(glideRisk.maxCoeff(), 0);
    ASSERT_NE(ballisticRisk.maxCoeff(), 0);
}

TEST_F(TemporalRiskMapTests, ResidentialStrikeRiskMapTest)
{
    auto population = std::unique_ptr<ugr::mapping::TemporalPopulationMap>(
        new ugr::mapping::TemporalPopulationMap(bounds, resolution));
    population->setHourOfDay(12);
    population->eval();

    // Assert the population map actually generated something otherwise
    // this test is pointless and equivalent to the zero* tests
    ASSERT_NE(population->get("Population Density").maxCoeff(), 0);

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    ObstacleMap obstacleMap(bounds, resolution);
    obstacleMap.addBuildingHeights();
    obstacleMap.eval();

    RiskMap riskMap(population.get(), aircraft, obstacleMap, weather);
    auto strikeMap = riskMap.generateMap({RiskType::STRIKE});

    ugr::gridmap::Matrix& glideRisk = strikeMap.get("Glide Strike Risk");
    ugr::gridmap::Matrix& ballisticRisk = strikeMap.get("Ballistic Strike Risk");

    const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

    const auto layers = strikeMap.getLayers();
    for (const auto& layer : layers)
    {
        std::cout << layer << " max " << std::scientific << strikeMap.get(layer).maxCoeff()
            << std::endl;

        std::ofstream file("strike_map_" + layer + ".csv");
        if (file.is_open())
        {
            file << strikeMap.get(layer).format(CSVFormat);
            file.close();
        }
        outputMat(strikeMap.get(layer), testing::UnitTest::GetInstance()->current_test_info()->name() + layer);
    }

    // As the population map is zero, this must all be zero as well
    ASSERT_NE(glideRisk.maxCoeff(), 0);
    ASSERT_NE(ballisticRisk.maxCoeff(), 0);
}
