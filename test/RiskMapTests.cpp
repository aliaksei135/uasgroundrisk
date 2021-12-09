/*
 * RiskMapTests
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "../src/risk_analysis/RiskMap.h"
#include <gtest/gtest.h>
// #include <matplotlibcpp.h>

using namespace ugr::risk;
// namespace plt = matplotlibcpp;

class RiskMapTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		state.position << 0, 0, 120;
		state.velocity << 20, 1, 0;
	}

	std::array<float, 4> bounds{
		50.9065510f, -1.4500237f, 50.9517765f,
		-1.3419628f
	};
	int resolution = 100;
	AircraftStateModel state;
	AircraftDescentModel descent{90, 2.8, 3.2, 28, 0.6 * 0.6, 0.8, 21, 15};
};

TEST_F(RiskMapTests, ZeroStrikeRiskMapTest)
{
	ugr::mapping::PopulationMap population(bounds, resolution);
	population.eval();

	WeatherMap weather(bounds, resolution);
	weather.addConstantWind(5, 90);
	weather.eval();

	RiskMap riskMap(population, descent, state, weather);
	auto strikeMap = riskMap.generateMap({RiskType::STRIKE});

	auto layers = strikeMap.getLayers();
	ASSERT_TRUE(std::find(layers.begin(), layers.end(), "Glide Strike Risk") !=
		layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(), "Glide Impact Angle") !=
		layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Glide Impact Velocity") != layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Ballistic Strike Risk") != layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Ballistic Impact Angle") != layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Ballistic Impact Velocity") != layers.end());

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	auto size = strikeMap.getSize();
	for (const auto& layer : layers)
	{
		std::cout << layer << " max " << std::scientific << strikeMap.get(layer).maxCoeff()
			<< std::endl;


		std::ofstream file("impact_map_" + layer + ".csv");
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

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
