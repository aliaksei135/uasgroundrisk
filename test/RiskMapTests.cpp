/*
 * RiskMapTests
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include "../src/risk_analysis/RiskMap.h"
#include <gtest/gtest.h>
#include "../src/utils/DataFitting.h"
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

TEST_F(RiskMapTests, UniformImpactRiskMapTest)
{
	ugr::mapping::PopulationMap population(bounds, resolution);
	population.eval();
	auto popSize = population.getSize();
	// PyObject *plot;
	// plt::title("Population Density");
	// plt::imshow(population.get("Population Density").data(), popSize.y(),
	//             popSize.x(), 1, {}, &plot);
	// plt::colorbar(plot);
	// plt::save("risk_map_population.png");
	// plt::close();

	WeatherMap weather(bounds, resolution);
	weather.addConstantWind(5, 90);
	weather.eval();

	RiskMap riskMap(population, descent, state, weather);
	auto impactMap = riskMap.generateMap({RiskType::IMPACT});

	auto layers = impactMap.getLayers();
	ASSERT_TRUE(std::find(layers.begin(), layers.end(), "Glide Impact Risk") !=
		layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(), "Glide Impact Angle") !=
		layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Glide Impact Velocity") != layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Ballistic Impact Risk") != layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Ballistic Impact Angle") != layers.end());
	ASSERT_TRUE(std::find(layers.begin(), layers.end(),
		"Ballistic Impact Velocity") != layers.end());

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	auto size = impactMap.getSize();
	for (const auto& layer : layers)
	{
		std::cout << layer << " max " << std::scientific << impactMap.get(layer).maxCoeff()
			<< std::endl;


		std::ofstream file("impact_map_" + layer + ".csv");
		if (file.is_open())
		{
			file << impactMap.get(layer).format(CSVFormat);
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

TEST_F(RiskMapTests, Gaussian2DFuncTest)
{
	ugr::util::Gaussian2DParamVector param;
	param << 1, 20, 20, 4, 4, 0, 0;
	const auto pdf = ugr::util::gaussian2D(20, 20, param);

	Eigen::Matrix<double, 40, 40> out;
	for (int x = 0; x < 40; ++x)
	{
		for (int y = 0; y < 40; ++y)
		{
			out(x, y) = ugr::util::gaussian2D(x, y, param);
		}
	}

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	std::ofstream file("gaussian_test.csv");
	if (file.is_open())
	{
		file << out.format(CSVFormat);
		file.close();
	}
}


TEST_F(RiskMapTests, Gaussian2DFitTest)
{
	std::vector<double> matrixEntries;

	// in this object we store the data from the matrix
	std::ifstream matrixDataFile("E:/rvs_test.csv");
	// this variable is used to store the row of the matrix that contains commas 
	std::string matrixRowString;
	// this variable is used to store the matrix entry;
	std::string matrixEntry;
	// this variable is used to track the number of rows
	int matrixRowNumber = 0;
	while (getline(matrixDataFile, matrixRowString))
		// here we read a row by row of matrixDataFile and store every line into the string variable matrixRowString
	{
		std::stringstream matrixRowStringStream(matrixRowString);
		//convert matrixRowString that is a string to a stream variable.

		while (getline(matrixRowStringStream, matrixEntry, ','))
			// here we read pieces of the stream matrixRowStringStream until every comma, and store the resulting character into the matrixEntry
		{
			matrixEntries.push_back(stod(matrixEntry));
			//here we convert the string to double and fill in the row vector storing all the matrix entries
		}
		matrixRowNumber++; //update the column numbers
	}

	// here we convet the vector variable into the matrix and return the resulting object, 
	// note that matrixEntries.data() is the pointer to the first memory location at which the entries of the vector matrixEntries are stored;
	MatrixXd dataMat = Map<Eigen::Matrix<double, Dynamic, Dynamic, RowMajor>>(
		matrixEntries.data(), matrixRowNumber, matrixEntries.size() / matrixRowNumber);

	ugr::util::Point2DVector data;
	data.reserve(matrixEntries.size() / 2);
	for (const auto& row : dataMat.rowwise())
	{
		data.emplace_back(row(0), row(1));
	}

	const auto param = ugr::util::Gaussian2DFit(data);
	Eigen::Matrix<double, 100, 100> out;
	out.setZero();
	for (int x = 0; x < out.cols(); ++x)
	{
		for (int y = 0; y < out.rows(); ++y)
		{
			out(x, y) = ugr::util::gaussian2D(x, y, param);
		}
	}

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	std::ofstream file("gaussian_test.csv");
	if (file.is_open())
	{
		file << out.format(CSVFormat);
		file.close();
	}
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
