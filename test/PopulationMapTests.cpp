/*
 * PopulationMapTests
 *
 *  Created by A.Pilko on 15/06/2021.
 */

#include "../src/utils/GeometryProjectionUtils.h"
#include <../src/map_gen/PopulationMap.h>
#include <Eigen/Dense>
#include <gtest/gtest.h>
// #include <matplotlibcpp.h>

using namespace ugr::mapping;
// namespace plt = matplotlibcpp;

class PopulationMapTests : public ::testing::Test
{
protected:
	std::array<float, 4> bounds{
		50.9065510f, -1.4500237f, 50.9517765f,
		-1.3419628f
	};
	int resolution = 20;
};

TEST_F(PopulationMapTests, EmptyMapTest)
{
	PopulationMap popMap(bounds, resolution);
	popMap.eval();

	ASSERT_EQ(popMap.getLayers().size(), 1);

	auto size = popMap.getSize();
	ASSERT_EQ(size.x(), 601);
	ASSERT_EQ(size.y(), 398);

	Position outPos(-1.338997, 50.933289);
	ASSERT_FALSE(popMap.isInBounds(popMap.world2Local(outPos)));

	Position inPos(-1.404258, 50.922982);
	ASSERT_TRUE(popMap.isInBounds(popMap.world2Local(inPos)));
}

TEST_F(PopulationMapTests, SingleLayerTest)
{
	PopulationMap popMap(bounds, resolution);
	popMap.addOSMLayer("Schools", {{"amenity", "school"}}, 10);
	popMap.eval();

	// Inside Bitterne Park School, Southampton
	const Position testPos(-1.369220, 50.929116);

	ASSERT_EQ(popMap.getLayers().size(), 2);
	ASSERT_EQ(popMap.get("Schools").maxCoeff(), 10);
	EXPECT_EQ(popMap.atPosition("Schools", testPos), 10);

	auto size = popMap.getSize();

	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, ", ", "\n");

	std::ofstream file("schools_pop_mat.csv");
	if (file.is_open())
	{
		file << popMap.get("Schools").format(CSVFormat);
		file.close();
	}

	// PyObject* plot;
	// plt::imshow(popMap.get("Schools").data(), size.y(), size.x(), 1, {}, &plot);
	// plt::colorbar(plot);
	// plt::save("single_layer_test.png");
	// plt::close();
}

TEST_F(PopulationMapTests, MultiLayerTest)
{
	PopulationMap popMap(bounds, resolution);
	popMap.addOSMLayer("Schools", {{"amenity", "school"}}, 10);
	popMap.addOSMLayer("Retail", {{"landuse", "retail"}}, 20);
	popMap.eval();

	ASSERT_EQ(popMap.getLayers().size(), 3);

	// Inside Bitterne Park School, Southampton
	const Position testSchoolPos(-1.369220, 50.929116);

	ASSERT_EQ(popMap.get("Schools").maxCoeff(), 10);
	EXPECT_EQ(popMap.atPosition("Schools", testSchoolPos), 10);

	// Portswood shops, Southampton
	const Position testRetailPos(-1.393254, 50.925358);

	ASSERT_EQ(popMap.get("Retail").maxCoeff(), 20);
	EXPECT_EQ(popMap.atPosition("Retail", testRetailPos), 20);

	auto size = popMap.getSize();
	// PyObject* plot;
	// plt::title("Schools");
	// plt::imshow(popMap.get("Schools").data(), size.y(), size.x(), 1, {}, &plot);
	// plt::colorbar(plot);
	// plt::save("multi_layer_school_test.png");
	// plt::close();
	// plt::title("Retail");
	// plt::imshow(popMap.get("Retail").data(), size.y(), size.x(), 1, {}, &plot);
	// plt::colorbar(plot);
	// plt::save("multi_layer_retail_test.png");
	// plt::close();
	// plt::title("Combined");
	// plt::imshow(popMap.get("Population Density").data(), size.y(), size.x(), 1,
	//             {}, &plot);
	// plt::colorbar(plot);
	// plt::save("multi_layer_combined_test.png");
	// plt::close();
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
