/*
 * AircraftModelTests
 *
 *  Created by A.Pilko on 17/06/2021.
 */

#include <gtest/gtest.h>
#include <cmath>
#include "uasgroundrisk/risk_analysis/aircraft/AircraftDescentModel.h"

using namespace ugr::risk;

TEST(AircraftModelTests, GlideImpactSingleTest)
{
	const GlideDescentModel gm(90, 2.8, 3.2, 21, 15);
	const auto res = gm.impact(120, 0, 0);
	EXPECT_NEAR(res.impactDistance, 1800, 0.1);
	EXPECT_NEAR(res.impactVelocity, 21, 0.1);
	EXPECT_NEAR(res.impactTime, 85.90455, 0.1);
	EXPECT_NEAR(res.impactAngle, 3.81407, 0.1);
}

TEST(AircraftModelTests, GlideImpactMultipleTest)
{
	const GlideDescentModel gm(90, 2.8, 3.2, 21, 15);
	const auto resVect = gm.DescentModel::impact({20, 40, 60, 80}, {0, 0, 0, 0}, {0, 0, 0, 0});

	ASSERT_EQ(resVect.size(), 4);

	const std::vector<double> expDistances{300, 600, 900, 1200};
	const std::vector<double> expVelocities{20.95, 20.95, 20.95, 20.95};
	const std::vector<double> expAngles{3.81, 3.81, 3.81, 3.81};
	const std::vector<double> expTimes{14.32, 28.63, 42.95, 57.27};
	for (size_t i = 0; i < resVect.size(); ++i)
	{
		EXPECT_NEAR(resVect[i].impactDistance, expDistances[i], 0.1);
		EXPECT_NEAR(resVect[i].impactVelocity, expVelocities[i], 0.1);
		EXPECT_NEAR(resVect[i].impactTime, expTimes[i], 0.1);
		EXPECT_NEAR(resVect[i].impactAngle, expAngles[i], 0.1);
	}
}

TEST(AircraftModelTests, BallisticImpactSingleTest)
{
	const BallisticDescentModel bm(90, 2.8, 3.2, 0.6 * 0.6, 0.8);
	const auto res = bm.impact(120, 28, 1);
	EXPECT_NEAR(res.impactDistance, 117.0, 0.1);
	EXPECT_NEAR(res.impactVelocity, 48.1, 0.1);
	EXPECT_NEAR(res.impactTime, 5.14, 0.1);
	EXPECT_NEAR(res.impactAngle, 64.5, 0.1);
}

TEST(AircraftModelTests, BallisticImpactMultipleTest)
{
	const BallisticDescentModel bm(90, 2.8, 3.2, 0.6 * 0.6, 0.8);
	const auto resVect = bm.DescentModel::impact({20, 40, 60, 80}, {15, 20, 25, 30},
	                                             {-0.5, 0, 0.5, 1});

	ASSERT_EQ(resVect.size(), 4);

	const std::vector<double> expDistances{30.1, 51.9, 76.0, 99.9};
	const std::vector<double> expVelocities{24.0, 32.3, 38.6, 43.8};
	const std::vector<double> expAngles{54.1, 56.5, 57.0, 57.1};
	const std::vector<double> expTimes{2.1, 2.9, 3.5, 4.1};
	for (size_t i = 0; i < resVect.size(); ++i)
	{
		EXPECT_NEAR(resVect[i].impactDistance, expDistances[i], 0.1);
		EXPECT_NEAR(resVect[i].impactVelocity, expVelocities[i], 0.1);
		EXPECT_NEAR(resVect[i].impactTime, expTimes[i], 0.1);
		EXPECT_NEAR(resVect[i].impactAngle, expAngles[i], 0.1);
	}
}

TEST(AircraftModelTests, BallisticImpactLowTest)
{
	const BallisticDescentModel bm(30, 5, 5, 0.6 * 0.6, 1.1);
	const auto resVect = bm.DescentModel::impact({ 1, 2, 5, 10 }, { 5, 10, 15, 20 },
		{ 0.5, 0.5, 0.5, 0.5 });

	for (const auto impact : resVect)
	{
		ASSERT_FALSE(std::isnan(impact.impactAngle));
		ASSERT_FALSE(std::isnan(impact.impactDistance));
		ASSERT_FALSE(std::isnan(impact.impactTime));
		ASSERT_FALSE(std::isnan(impact.impactVelocity));
	}
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
