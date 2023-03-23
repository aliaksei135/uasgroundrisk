/*
 * IncrementalIncrementalRiskMapTests
 *
 *  Created by A.Pilko on 22/03/2023.
 */
#include <memory>

#include "uasgroundrisk/risk_analysis/IncrementalRiskMap.h"
#include <gtest/gtest.h>
#include <fstream>

#include "uasgroundrisk/map_gen/osm/OSMTag.h"
#include "TestPlottingUtils.h"
#include "uasgroundrisk/map_gen/TemporalPopulationMap.h"

using namespace ugr::risk;
using namespace ugr::mapping::osm;

class IncrementalRiskMapTests : public testing::Test
{
 protected:
	void SetUp() override
	{
		aircraft.state.position << 0, 0, 120;
		aircraft.state.velocity << 20, 0, 0;

		aircraft.mass = 50;
		aircraft.length = 5;
		aircraft.width = 5;
		aircraft.failureProb = 8e-3;

		aircraft.addDescentModel<GlideDescentModel>(21, 15);
		aircraft.addDescentModel<BallisticDescentModel>(25 * 0.3, 0.8);
	}

	std::array<float, 4> bounds{
		50.9065510f, -1.4500237f, 50.9517765f,
		-1.3419628f
	};
	int resolution = 60;
	AircraftModel aircraft;

	const Position3 testSchoolPosition{ -1.398618, 50.941705, 80 };
};

TEST_F(IncrementalRiskMapTests, EmptyMapLayerConstructionTest)
{
	ugr::mapping::PopulationMap population(bounds, resolution);
	population.eval();

	WeatherMap weather(bounds, resolution);
	// weather.addConstantWind(5, 90);
	weather.eval();

	ObstacleMap obstacleMap(bounds, resolution);
	// obstacleMap.addBuildingHeights();
	obstacleMap.eval();

	const IncrementalRiskMap riskMap(population, aircraft, obstacleMap, weather);
}

TEST_F(IncrementalRiskMapTests, ZeroIncrementalRiskMapTest)
{
	ugr::mapping::PopulationMap population(bounds, resolution);
	// population.eval();

	WeatherMap weather(bounds, resolution);
	weather.addConstantWind(5, 90);
	weather.eval();

	ObstacleMap obstacleMap(bounds, resolution);
	obstacleMap.addBuildingHeights();
	obstacleMap.eval();

	IncrementalRiskMap riskMap(population, aircraft, obstacleMap, weather);

	const auto& strikeRisk = riskMap.getPointStrikeProbability(testSchoolPosition, 90);
	const auto& fatalityRisk = riskMap.getPointFatalityProbability(testSchoolPosition, 90);

	// As the population map is zero, this must all be zero as well
	ASSERT_EQ(strikeRisk, 0);
	ASSERT_EQ(fatalityRisk, 0);
}

TEST_F(IncrementalRiskMapTests, SchoolsStrikeIncrementalRiskMapTest)
{
	ugr::mapping::PopulationMap population(bounds, resolution);
	population.addOSMLayer("Schools", { OSMTag("amenity", "school") }, 100);
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

	IncrementalRiskMap riskMap(population, aircraft, obstacleMap, weather);
	const auto& strikeRisk = riskMap.getPointStrikeProbability(testSchoolPosition, 90);
	const auto& fatalityRisk = riskMap.getPointFatalityProbability(testSchoolPosition, 90);

	ASSERT_NE(strikeRisk, 0);
	ASSERT_NE(fatalityRisk, 0);
}

TEST_F(IncrementalRiskMapTests, TemporalIncrementalRiskMapTest)
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

	IncrementalRiskMap riskMap(population, aircraft, obstacleMap, weather);
	const auto& strikeRisk = riskMap.getPointStrikeProbability(testSchoolPosition, 90);
	const auto& fatalityRisk = riskMap.getPointFatalityProbability(testSchoolPosition, 90);

	ASSERT_NE(strikeRisk, 0);
	ASSERT_NE(fatalityRisk, 0);
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
