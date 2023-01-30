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
//		aircraft.state.position << 0, 0, 120;
//		aircraft.state.velocity << 20, 0, 0;
//
//		aircraft.mass = 30;
//		aircraft.length = 5;
//		aircraft.width = 5;
//
//		aircraft.addDescentModel<GlideDescentModel>(21, 15);
//		aircraft.addDescentModel<BallisticDescentModel>(25 * 0.3, 0.8);

		aircraft.state.position << 0, 0, 120;
		aircraft.state.velocity << 25, 0, 0;

		aircraft.mass = 35;
		aircraft.length = 4;
		aircraft.width = 3.5;
		aircraft.failureProb  = 8e-3;

		aircraft.addDescentModel<GlideDescentModel>(25, 10);
		aircraft.addDescentModel<BallisticDescentModel>(0.7, 0.65);
	}

//	std::array<float, 4> bounds{
//		50.703057f, -1.973112f, 50.820251f,
//		-1.767941f
//	};
	 std::array<float, 4> bounds{
		52.01199f, -0.71306f, 52.13621f,
		-0.5221f
	 };
	int resolution = 50;
	AircraftModel aircraft;
};

TEST_F(TemporalRiskMapTests, SchoolsStrikeRiskMapTest)
{
	ugr::mapping::TemporalPopulationMap population(bounds, resolution);
	population.setHourOfDay(12);
	population.eval();

	// Assert the population map actually generated something otherwise
	// this test is pointless and equivalent to the zero* tests
	// ASSERT_NE(population.get("Population Density").maxCoeff(), 0);

	WeatherMap weather(bounds, resolution);
	weather.addConstantWind(2, 60);
	weather.eval();

	ObstacleMap obstacleMap(bounds, resolution);
	obstacleMap.addBuildingHeights();
	obstacleMap.eval();

	RiskMap riskMap(population, aircraft, obstacleMap,
		weather);
	riskMap.SetAnyHeading(true);
	std::cout << riskMap.getSize() << "\n";
	auto strikeMap = riskMap.generateMap({ RiskType::FATALITY });

	ugr::gridmap::Matrix& glideRisk = strikeMap.get("Glide Strike Risk");
	ugr::gridmap::Matrix& ballisticRisk = strikeMap.get("Ballistic Strike Risk");

	const auto& grNan = glideRisk.hasNaN();
	const auto& brNan = ballisticRisk.hasNaN();

	ASSERT_FALSE(grNan);
	ASSERT_FALSE(brNan);

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
	ugr::mapping::TemporalPopulationMap population(bounds, resolution);
	population.setHourOfDay(12);
	population.eval();

	// Assert the population map actually generated something otherwise
	// this test is pointless and equivalent to the zero* tests
	ASSERT_NE(population.get("Population Density").maxCoeff(), 0);

	WeatherMap weather(bounds, resolution);
	weather.addConstantWind(5, 90);
	weather.eval();

	ObstacleMap obstacleMap(bounds, resolution);
	obstacleMap.addBuildingHeights();
	obstacleMap.eval();

	RiskMap riskMap(population, aircraft, obstacleMap, weather);
	auto strikeMap = riskMap.generateMap({ RiskType::STRIKE });

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
