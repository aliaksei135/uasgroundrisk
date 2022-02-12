/*
 * RiskMapTests
 *
 *  Created by A.Pilko on 17/06/2021.
 */
#include <memory>

#include "uasgroundrisk/risk_analysis/RiskMap.h"
#include <gtest/gtest.h>
#include <fstream>

#include "uasgroundrisk/map_gen/osm/OSMTag.h"
// #include <matplotlibcpp.h>

using namespace ugr::risk;
using namespace ugr::mapping::osm;
// namespace plt = matplotlibcpp;

class RiskMapTests : public testing::Test
{
protected:
    void SetUp() override
    {
        aircraft.state.position << 0, 0, 120;
        aircraft.state.velocity << 20, 0, 0;

        aircraft.descents.emplace_back(
            std::unique_ptr<GlideDescentModel>(new GlideDescentModel(90, 2.8, 3.2, 21, 15)));
        aircraft.descents.emplace_back(
            std::unique_ptr<BallisticDescentModel>(new BallisticDescentModel(90, 2.8, 3.2, 0.6 * 0.6, 0.8)));
        aircraft.descents.emplace_back(
            std::unique_ptr<ParachuteDescentModel>(new ParachuteDescentModel(90, 2.8, 3.2, 1.2, 12.5, 2)));
    }

    std::array<float, 4> bounds{
        50.9065510f, -1.4500237f, 50.9517765f,
        -1.3419628f
    };
    int resolution = 60;
    AircraftModel aircraft;
};

TEST_F(RiskMapTests, EmptyMapLayerConstructionTest)
{
    ugr::mapping::PopulationMap population(bounds, resolution);
    population.eval();

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    const RiskMap riskMap(population, aircraft, weather);

    auto layers = riskMap.getLayers();
    // Check all the layers are there
    ASSERT_TRUE(std::find(layers.begin(), layers.end(), "Glide Strike Risk") !=
        layers.end());
    ASSERT_TRUE(std::find(layers.begin(), layers.end(), "Glide Fatality Risk") !=
        layers.end());
    ASSERT_TRUE(std::find(layers.begin(), layers.end(), "Glide Impact Angle") !=
        layers.end());
    ASSERT_TRUE(std::find(layers.begin(), layers.end(),
        "Glide Impact Velocity") != layers.end());
    ASSERT_TRUE(std::find(layers.begin(), layers.end(),
        "Ballistic Strike Risk") != layers.end());
    ASSERT_TRUE(std::find(layers.begin(), layers.end(),
        "Ballistic Fatality Risk") != layers.end());
    ASSERT_TRUE(std::find(layers.begin(), layers.end(),
        "Ballistic Impact Angle") != layers.end());
    ASSERT_TRUE(std::find(layers.begin(), layers.end(),
        "Ballistic Impact Velocity") != layers.end());
}

TEST_F(RiskMapTests, ZeroStrikeRiskMapTest)
{
    ugr::mapping::PopulationMap population(bounds, resolution);
    population.eval();

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    RiskMap riskMap(population, aircraft, weather);
    auto strikeMap = riskMap.generateMap({RiskType::STRIKE});

    ugr::gridmap::Matrix& glideRisk = strikeMap.get("Glide Strike Risk");
    ugr::gridmap::Matrix& ballisticRisk = strikeMap.get("Ballistic Strike Risk");

    // As the population map is zero, this must all be zero as well
    ASSERT_TRUE(glideRisk.all() == 0);
    ASSERT_TRUE(ballisticRisk.all() == 0);


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
    }
}

TEST_F(RiskMapTests, ZeroFatalityRiskMapTest)
{
    ugr::mapping::PopulationMap population(bounds, resolution);
    population.eval();

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    RiskMap riskMap(population, aircraft, weather);
    auto fatalityMap = riskMap.generateMap({RiskType::FATALITY});

    ugr::gridmap::Matrix& glideRisk = fatalityMap.get("Glide Fatality Risk");
    ugr::gridmap::Matrix& ballisticRisk = fatalityMap.get("Ballistic Fatality Risk");

    // As the population map is zero, this must all be zero as well
    ASSERT_TRUE(glideRisk.all() == 0);
    ASSERT_TRUE(ballisticRisk.all() == 0);


    const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

    const auto layers = fatalityMap.getLayers();
    for (const auto& layer : layers)
    {
        std::cout << layer << " max " << std::scientific << fatalityMap.get(layer).maxCoeff()
            << std::endl;

        std::ofstream file("fatality_map_" + layer + ".csv");
        if (file.is_open())
        {
            file << fatalityMap.get(layer).format(CSVFormat);
            file.close();
        }
    }
}

TEST_F(RiskMapTests, SchoolsStrikeRiskMapTest)
{
    ugr::mapping::PopulationMap population(bounds, resolution);
    population.addOSMLayer("Schools", {OSMTag("amenity", "school")}, 100);
    population.eval();

    // Assert the population map actually generated something otherwise
    // this test is pointless and equivalent to the zero* tests
    ASSERT_NE(population.get("Population Density").maxCoeff(), 0);

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    RiskMap riskMap(population, aircraft, weather);
    auto strikeMap = riskMap.generateMap({RiskType::STRIKE});

    ugr::gridmap::Matrix& glideRisk = strikeMap.get("Glide Strike Risk");
    ugr::gridmap::Matrix& ballisticRisk = strikeMap.get("Ballistic Strike Risk");

    ASSERT_NE(glideRisk.maxCoeff(), 0);
    ASSERT_NE(ballisticRisk.maxCoeff(), 0);


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
        //    PyObject *layerPlot;
        //    plt::title(layer);
        //    plt::imshow(impactMap.get(layer).data(), size.y(), size.x(), 1);
        //    plt::colorbar(layerPlot);
        //    plt::save("risk_map_" + layer + "_test.png");
        //    plt::close();
        //    delete layerPlot;
    }
}

TEST_F(RiskMapTests, ResidentialStrikeRiskMapTest)
{
    ugr::mapping::PopulationMap population(bounds, resolution);
    population.addOSMLayer("Residential", {{"landuse", "residential"}}, 100);
    population.eval();

    // Assert the population map actually generated something otherwise
    // this test is pointless and equivalent to the zero* tests
    ASSERT_NE(population.get("Population Density").maxCoeff(), 0);

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    RiskMap riskMap(population, aircraft, weather);
    auto strikeMap = riskMap.generateMap({RiskType::STRIKE});

    ugr::gridmap::Matrix& glideRisk = strikeMap.get("Glide Strike Risk");
    ugr::gridmap::Matrix& ballisticRisk = strikeMap.get("Ballistic Strike Risk");

    // As the population map is zero, this must all be zero as well
    ASSERT_NE(glideRisk.maxCoeff(), 0);
    ASSERT_NE(ballisticRisk.maxCoeff(), 0);


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
        //    PyObject *layerPlot;
        //    plt::title(layer);
        //    plt::imshow(impactMap.get(layer).data(), size.y(), size.x(), 1);
        //    plt::colorbar(layerPlot);
        //    plt::save("risk_map_" + layer + "_test.png");
        //    plt::close();
        //    delete layerPlot;
    }
}

TEST_F(RiskMapTests, NilWindPointImpactMapTest)
{
    ugr::mapping::PopulationMap population(bounds, resolution);
    population.eval();

    WeatherMap weather(bounds, resolution);
    weather.eval();

    RiskMap riskMap(population, aircraft, weather);

    const auto size = riskMap.getSize();

    std::vector<GridMapDataType> impactAngles, impactVelocities;
    std::vector<ugr::gridmap::Matrix, aligned_allocator<GridMapDataType>> impactPDFs;
    ugr::gridmap::Index idx{20, 20};
    riskMap.makePointImpactMap(idx, impactPDFs, impactAngles, impactVelocities);


    // These are added in known order, so we can skip a step checking which model is which
    const auto& glideImpact = impactPDFs[0];
    const auto& ballisticImpact = impactPDFs[1];

    // Make sure they are actually PDFs
    ASSERT_NEAR(glideImpact.sum(), 1, 1e-3);
    ASSERT_NEAR(ballisticImpact.sum(), 1, 1e-3);

    int gmx, gmy, bmx, bmy;
    glideImpact.maxCoeff(&gmx, &gmy);
    ballisticImpact.maxCoeff(&bmx, &bmy);

    // Invert x axis to stay with the axes convention here
    gmx = size[0] - gmx;
    bmx = size[0] - bmx;

    // Velocity is (20,0) xy and there is no wind
    // therefore we expect to impact PDF max to be
    // in the direction of the velocity vector,
    // so the x position should be greater than the LoC x
    // and the y remain roughly the same
    EXPECT_GE(gmx, idx[0]);
    EXPECT_NEAR(gmy, idx[1], ceil(20/resolution));
    EXPECT_GE(bmx, idx[0]);
    EXPECT_NEAR(bmy, idx[1], ceil(20/resolution));

    // Additionally we would expect the uncontrolled glide
    // descent to go further than the ballistic descent
    EXPECT_GT(gmx, bmx);

    const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

    std::ofstream file(
        "combined_impact_map_x" + std::to_string(idx[0]) + "_y" + std::to_string(idx[1]) + "_nil_wind.csv");
    if (file.is_open())
    {
        file << (glideImpact + ballisticImpact).format(CSVFormat);
        file.close();
    }
    //    PyObject *layerPlot;
    //    plt::title(layer);
    //    plt::imshow(impactMap.get(layer).data(), size.y(), size.x(), 1);
    //    plt::colorbar(layerPlot);
    //    plt::save("risk_map_" + layer + "_test.png");
    //    plt::close();
    //    delete layerPlot;
}

TEST_F(RiskMapTests, WindPointImpactMapTest)
{
    ugr::mapping::PopulationMap population(bounds, resolution);
    population.eval();

    WeatherMap weather(bounds, resolution);
    weather.addConstantWind(5, 90);
    weather.eval();

    RiskMap riskMap(population, aircraft, weather);

    const auto size = riskMap.getSize();

    std::vector<GridMapDataType> impactAngles, impactVelocities;
    std::vector<ugr::gridmap::Matrix, aligned_allocator<GridMapDataType>> impactPDFs;
    ugr::gridmap::Index idx{20, 20};
    riskMap.makePointImpactMap(idx, impactPDFs, impactAngles, impactVelocities);

    // These are added in known order, so we can skip a step checking which model is which
    const auto& glideImpact = impactPDFs[0];
    const auto& ballisticImpact = impactPDFs[1];

    // Make sure they are actually PDFs
    ASSERT_NEAR(glideImpact.sum(), 1, 1e-3);
    ASSERT_NEAR(ballisticImpact.sum(), 1, 1e-3);

    int gmx, gmy, bmx, bmy;
    glideImpact.maxCoeff(&gmx, &gmy);
    ballisticImpact.maxCoeff(&bmx, &bmy);

    // Invert x axis to stay with the axes convention here
    gmx = size[0] - gmx;
    bmx = size[0] - bmx;

    // Velocity is (20,0) xy therefore we expect to impact
    // PDF max to be mostly in the direction of the velocity vector
    // as the aircraft momentum/PE should still carry more than the wind
    // so the x position should be greater than the LoC x.
    // The wind is right to left in the aircraft body frame, therefore
    // the impact position in both descents should be to the left
    // ie y should decrease relative to LoC y.
    EXPECT_GE(gmx, idx[0]);
    EXPECT_LE(gmy, idx[1]);
    EXPECT_GE(bmx, idx[0]);
    EXPECT_LE(bmy, idx[1]);

    // Additionally we would expect the uncontrolled glide
    // descent to go further than the ballistic descent
    EXPECT_GT(gmx, bmx);

    const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

    std::ofstream file("combined_impact_map_x" + std::to_string(idx[0]) + "_y" + std::to_string(idx[1]) + "_wind.csv");
    if (file.is_open())
    {
        file << (glideImpact + ballisticImpact).format(CSVFormat);
        file.close();
    }
    //    PyObject *layerPlot;
    //    plt::title(layer);
    //    plt::imshow(impactMap.get(layer).data(), size.y(), size.x(), 1);
    //    plt::colorbar(layerPlot);
    //    plt::save("risk_map_" + layer + "_test.png");
    //    plt::close();
    //    delete layerPlot;
}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
