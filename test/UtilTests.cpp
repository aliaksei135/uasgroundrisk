#include <gtest/gtest.h>

#include "uasgroundrisk/gridmap/TypeDefs.h"
#include "../src/utils/GeometryOperations.h"

TEST(UtilTests, Bresenham2D1QTest)
{
	const ugr::gridmap::Index start(0, 1);
	const ugr::gridmap::Index end(6, 4);

	const auto line = ugr::util::bresenham2D(start, end);

	const std::vector<ugr::gridmap::Index, Eigen::aligned_allocator<ugr::gridmap::Index>> expected
	{
		{0, 1},
		{1, 1},
		{2, 2},
		{3, 2},
		{4, 3},
		{5, 3},
		{6, 4}
	};

	ASSERT_EQ(line.size(), expected.size());

	// Points can be out of order as long as they are there
	for (int i = 0; i < expected.size(); ++i)
	{
		for (int j = 0; j < line.size(); ++j)
		{
			if (line[i].isApprox(expected[i]))
			{
				break;
			}
			if (j == line.size() - 1)
			{
				FAIL();
			}
		}
	}
}

TEST(UtilTests, Bresenham2D2QTest)
{
	const ugr::gridmap::Index start(0, 1);
	const ugr::gridmap::Index end(-6, 4);

	const auto line = ugr::util::bresenham2D(start, end);

	const std::vector<ugr::gridmap::Index, Eigen::aligned_allocator<ugr::gridmap::Index>> expected
	{
		{0, 1},
		{-1, 2},
		{-2, 2},
		{-3, 3},
		{-4, 3},
		{-5, 4},
		{-6, 4}
	};

	ASSERT_EQ(line.size(), expected.size());

	// Points can be out of order as long as they are there
	for (int i = 0; i < expected.size(); ++i)
	{
		for (int j = 0; j < line.size(); ++j)
		{
			if (line[i].isApprox(expected[j]))
			{
				break;
			}
			if (j == line.size() - 1)
			{
				FAIL();
			}
		}
	}
}

TEST(UtilTests, Bresenham2D3QTest)
{
	const ugr::gridmap::Index start(0, -1);
	const ugr::gridmap::Index end(-6, -4);

	const auto line = ugr::util::bresenham2D(start, end);

	const std::vector<ugr::gridmap::Index, Eigen::aligned_allocator<ugr::gridmap::Index>> expected
	{
		{0, -1},
		{-1, -2},
		{-2, -2},
		{-3, -3},
		{-4, -3},
		{-5, -4},
		{-6, -4}
	};

	ASSERT_EQ(line.size(), expected.size());

	// Points can be out of order as long as they are there
	for (int i = 0; i < expected.size(); ++i)
	{
		for (int j = 0; j < line.size(); ++j)
		{
			if (line[i].isApprox(expected[j]))
			{
				break;
			}
			if (j == line.size() - 1)
			{
				FAIL();
			}
		}
	}
}

TEST(UtilTests, Bresenham2D4QTest)
{
	const ugr::gridmap::Index start(0, -1);
	const ugr::gridmap::Index end(6, -4);

	const auto line = ugr::util::bresenham2D(start, end);

	const std::vector<ugr::gridmap::Index, Eigen::aligned_allocator<ugr::gridmap::Index>> expected
	{
		{0, -1},
		{1, -1},
		{2, -2},
		{3, -2},
		{4, -3},
		{5, -3},
		{6, -4}
	};

	ASSERT_EQ(line.size(), expected.size());

	// Points can be out of order as long as they are there
	for (int i = 0; i < expected.size(); ++i)
	{
		for (int j = 0; j < line.size(); ++j)
		{
			if (line[i].isApprox(expected[j]))
			{
				break;
			}
			if (j == line.size() - 1)
			{
				FAIL();
			}
		}
	}
}
