#include <gtest/gtest.h>
#include <uasgroundrisk/map_gen/GeospatialGridMap.h>

TEST(GeospatialGridMapTests, ProjectionTest)
{
	std::array<float, 4> bounds{
		50.703057f, -1.973112f, 50.820251f,
		-1.767941f
	};
	float res = 80;

	ugr::mapping::GeospatialGridMap gm(bounds, res);

	const ugr::gridmap::Position pos1{ -1.973112f, 50.703057f };
	const auto& idx1 = gm.world2Local({ -1.973112f, 50.703057f });
	const auto& pos1t = gm.local2World(idx1);

	ASSERT_NEAR(pos1(0), pos1t(0), 0.001);
	ASSERT_NEAR(pos1(1), pos1t(1), 0.001);

	const ugr::gridmap::Index idx2{ 20, 20 };
	const auto& pos2 = gm.local2World(idx2);
	const auto& idx2t = gm.world2Local(pos2);

	ASSERT_NEAR(idx2(0), idx2t(0), 1.1);
	ASSERT_NEAR(idx2(1), idx2t(1), 1.1);
}