#include <gtest/gtest.h>
#include <uasgroundrisk/map_gen/GeospatialGridMap.h>

TEST(GeospatialGridMapTests, ProjectionTest)
{
	std::array<float, 4> bounds{
		50.703057f, -1.973112f, 50.820251f,
		-1.667941f
	};
	float res = 80;

	ugr::mapping::GeospatialGridMap gm(bounds, res);
	const auto& size = gm.getSize();

	const ugr::gridmap::Position pos1{ -1.973112f, 50.703057f };
	const auto& idx1 = gm.world2Local({ -1.973112f, 50.703057f });
	const auto& pos1t = gm.local2World(idx1);
	ASSERT_NEAR(pos1(0), pos1t(0), 0.001);
	ASSERT_NEAR(pos1(1), pos1t(1), 0.001);

	const ugr::gridmap::Index idx2{ 0, 0 };
	const auto& pos2 = gm.local2World(idx2);
	const auto& idx2t = gm.world2Local(pos2);
	ASSERT_NEAR(idx2(0), idx2t(0), 1.1);
	ASSERT_NEAR(idx2(1), idx2t(1), 1.1);

	const ugr::gridmap::Index idx3{ size[0] - 1, size[1] - 1 };
	const auto& pos3 = gm.local2World(idx3);
	const auto& idx3t = gm.world2Local(pos3);
	ASSERT_NEAR(idx3(0), idx3t(0), 1.1);
	ASSERT_NEAR(idx3(1), idx3t(1), 1.1);

	const ugr::gridmap::Index idx4{ size[0] - 1, 0 };
	const auto& pos4 = gm.local2World(idx4);
	const auto& idx4t = gm.world2Local(pos4);
	ASSERT_NEAR(idx4(0), idx4t(0), 1.1);
	ASSERT_NEAR(idx4(1), idx4t(1), 1.1);

	const ugr::gridmap::Index idx5{ 0, size[1] - 1 };
	const auto& pos5 = gm.local2World(idx5);
	const auto& idx5t = gm.world2Local(pos5);
	ASSERT_NEAR(idx5(0), idx5t(0), 1.1);
	ASSERT_NEAR(idx5(1), idx5t(1), 1.1);

	const ugr::gridmap::Position pos6{ -1.973112f, 50.820f };
	const auto& idx6 = gm.world2Local(pos6);
	const auto& pos6t = gm.local2World(idx6);
	ASSERT_NEAR(pos6(0), pos6t(0), 0.001);
	ASSERT_NEAR(pos6(1), pos6t(1), 0.001);

	const ugr::gridmap::Position pos7{ -1.7689f, 50.820f };
	const auto& idx7 = gm.world2Local(pos7);
	const auto& pos7t = gm.local2World(idx7);
	ASSERT_NEAR(pos7(0), pos7t(0), 0.001);
	ASSERT_NEAR(pos7(1), pos7t(1), 0.001);

	const ugr::gridmap::Position pos8{ -1.7689f, 50.703057f };
	const auto& idx8 = gm.world2Local(pos8);
	const auto& pos8t = gm.local2World(idx8);
	ASSERT_NEAR(pos8(0), pos8t(0), 0.001);
	ASSERT_NEAR(pos8(1), pos8t(1), 0.001);
};

TEST(GeospatialGridMapTests, IndexingTest)
{
	std::array<float, 4> bounds{
		50.703057f, -1.973112f, 50.820251f,
		-1.767941f
	};
	float res = 80;

	ugr::mapping::GeospatialGridMap gm(bounds, res);
	const auto& size = gm.getSize();

	ugr::gridmap::Matrix testGrid(size.x(), size.y());
	testGrid.setConstant(0);
	for (int i = 0; i < size.x(); ++i)
	{
		for (int j = 0; j < size.y(); ++j)
		{
			testGrid(i, j) = static_cast<ugr::gridmap::Matrix::Scalar>(i + j);
		}
	}

	gm.add("test", testGrid);

	const ugr::gridmap::Position pos1{ -1.973112f, 50.703057f };
	const auto& val1 = gm.atPosition("test", pos1);

	const ugr::gridmap::Index idx2{ 20, 20 };
	const auto& val2 = gm.at("test", idx2);
	ASSERT_EQ(val2, 40);

	const ugr::gridmap::Position pos3{ -1.8705265f, 50.7616505f };
	const auto& val3 = gm.atPosition("test", pos3);
	ASSERT_NEAR(val3, (size[0] / 2) + (size[1] / 2), 1.1);

	const ugr::gridmap::Position pos4{ -1.7689f, 50.820f };
	const auto& val4 = gm.atPosition("test", pos4);
	ASSERT_NEAR(val4, size[0] + size[1] - 2, 2);

	const ugr::gridmap::Position pos5{ -1.973112f, 50.820f };
	const auto& val5 = gm.atPosition("test", pos5);
	ASSERT_NEAR(val5, size[1] - 1, 1);

	const ugr::gridmap::Position pos6{ -1.7689f, 50.703057f };
	const auto& val6 = gm.atPosition("test", pos6);
	ASSERT_NEAR(val6, size[0] - 1, 1);
}