#include <gtest/gtest.h>
#include "uasgroundrisk/risk_analysis/weather/WeatherMap.h"

using namespace ugr::risk;

class WeatherMapTests : public ::testing::Test
{
protected:
	std::array<float, 4> bounds{
		50.9065510f, -1.4500237f, 50.9517765f,
		-1.3419628f
	};
	int resolution = 100;

	WeatherMap map = WeatherMap(bounds, resolution);
};

TEST_F(WeatherMapTests, WindFirstQuadrantTest)
{
	map.addConstantWind(5, 45);

	const ugr::gridmap::Matrix& velX = map.get("Wind VelX");
	const ugr::gridmap::Matrix& velY = map.get("Wind VelY");

	ASSERT_TRUE((velX.array() == -2.5 * sqrt(2)).all());
	ASSERT_TRUE((velY.array() == -2.5 * sqrt(2)).all());
}

TEST_F(WeatherMapTests, WindSecondQuadrantTest)
{
	map.addConstantWind(5, 135);

	const ugr::gridmap::Matrix& velX = map.get("Wind VelX");
	const ugr::gridmap::Matrix& velY = map.get("Wind VelY");

	ASSERT_TRUE((velX.array() == 2.5 * sqrt(2)).all());
	ASSERT_TRUE((velY.array() == -2.5 * sqrt(2)).all());
}

TEST_F(WeatherMapTests, WindThirdQuadrantTest)
{
	map.addConstantWind(5, 225);

	const ugr::gridmap::Matrix& velX = map.get("Wind VelX");
	const ugr::gridmap::Matrix& velY = map.get("Wind VelY");

	ASSERT_TRUE((velX.array() == 2.5 * sqrt(2)).all());
	ASSERT_TRUE((velY.array() == 2.5 * sqrt(2)).all());
}

TEST_F(WeatherMapTests, WindFourthQuadrantTest)
{
	map.addConstantWind(5, 315);

	const ugr::gridmap::Matrix& velX = map.get("Wind VelX");
	const ugr::gridmap::Matrix& velY = map.get("Wind VelY");

	ASSERT_TRUE((velX.array() == -2.5 * sqrt(2)).all());
	ASSERT_TRUE((velY.array() == 2.5 * sqrt(2)).all());
}

TEST_F(WeatherMapTests, WindWrapAroundTest)
{
	WeatherMap wrapMap(bounds, resolution);
	map.addConstantWind(5, 0);
	wrapMap.addConstantWind(5, 360);


	const ugr::gridmap::Matrix& velX = map.get("Wind VelX");
	const ugr::gridmap::Matrix& velY = map.get("Wind VelY");

	const ugr::gridmap::Matrix& wvelX = map.get("Wind VelX");
	const ugr::gridmap::Matrix& wvelY = map.get("Wind VelY");

	ASSERT_TRUE(velX == wvelX);
	ASSERT_TRUE(velY == wvelY);
}

TEST_F(WeatherMapTests, WindNegativeSpeedTest)
{
	ASSERT_THROW(map.addConstantWind(-5, 90), std::out_of_range);
}

TEST_F(WeatherMapTests, WindWrongDirectionTest)
{
	ASSERT_THROW(map.addConstantWind(5, 3000), std::out_of_range);
}
