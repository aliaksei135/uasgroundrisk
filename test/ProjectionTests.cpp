/*
 * GeometryTests.cpp
 *
 *  Created by A.Pilko on 23/04/2021.
 */

#include <geos_c.h>
#include <gtest/gtest.h>
#include <proj.h>

#include "../src/utils/DefaultGEOSMessageHandlers.h"
#include "../src/utils/GeometryProjectionUtils.h"

using namespace ugr::util;

class ProjectionTests : public ::testing::Test {
public:
  void SetUp() override {
    projCtx = proj_context_create();
    proj_context_set_enable_network(projCtx, true);
    reproj = proj_create_crs_to_crs(projCtx, "EPSG:4326", "EPSG:3395", nullptr);
  }

  PJ_CONTEXT *projCtx{};
  PJ *reproj{};
};

TEST_F(ProjectionTests, SingleCoordinateReprojectionTest) {
  // Boldrewood Campus,  University of Southampton, UK: EPSG4326 lon/lat =
  // -1.404611, 50.936923
  GEOSCoordSequence *coordSeq = GEOSCoordSeq_create(1, 2);
  GEOSCoordSeq_setOrdinate(coordSeq, 0, 0, 50.936923);
  GEOSCoordSeq_setOrdinate(coordSeq, 0, 1, -1.404611);

  GEOSCoordSequence *out = reprojectCoordinates(reproj, coordSeq);

  double reprojX, reprojY;
  GEOSCoordSeq_getOrdinate(out, 0, 0, &reprojX);
  GEOSCoordSeq_getOrdinate(out, 0, 1, &reprojY);

  ASSERT_NEAR(reprojX, -156360.60, 2);
  ASSERT_NEAR(reprojY, 6576946.28, 2);
}

TEST_F(ProjectionTests, MultipleCoordinateReprojectionTest) {
  // Boldrewood Campus,  University of Southampton, UK: EPSG4326 lon/lat =
  // -1.404611, 50.936923 Malaysia Campus,  University of Southampton, Malaysia:
  // EPSG4326 lon/lat = 103.611747, 1.430188 LC39A CCAFS, USA: EPSG4326 lon/lat
  // = -80.604146, 28.608259
  GEOSCoordSequence *coordSeq = GEOSCoordSeq_create(3, 2);

  GEOSCoordSeq_setOrdinate(coordSeq, 0, 0, 50.936923);
  GEOSCoordSeq_setOrdinate(coordSeq, 0, 1, -1.404611);

  GEOSCoordSeq_setOrdinate(coordSeq, 1, 0, 1.430188);
  GEOSCoordSeq_setOrdinate(coordSeq, 1, 1, 103.611747);

  GEOSCoordSeq_setOrdinate(coordSeq, 2, 0, 28.608259);
  GEOSCoordSeq_setOrdinate(coordSeq, 2, 1, -80.604146);

  GEOSCoordSequence *out = reprojectCoordinates(reproj, coordSeq);

  double reprojX[3];
  double reprojY[3];

  GEOSCoordSeq_getOrdinate(out, 0, 0, &reprojX[0]);
  GEOSCoordSeq_getOrdinate(out, 0, 1, &reprojY[0]);

  GEOSCoordSeq_getOrdinate(out, 1, 0, &reprojX[1]);
  GEOSCoordSeq_getOrdinate(out, 1, 1, &reprojY[1]);

  GEOSCoordSeq_getOrdinate(out, 2, 0, &reprojX[2]);
  GEOSCoordSeq_getOrdinate(out, 2, 1, &reprojY[2]);

  ASSERT_NEAR(reprojX[0], -156360.60, 2);
  ASSERT_NEAR(reprojY[0], 6576946.28, 2);

  ASSERT_NEAR(reprojX[1], 11534006.92, 2);
  ASSERT_NEAR(reprojY[1], 158158.65, 2);

  ASSERT_NEAR(reprojX[2], -8972812.49, 2);
  ASSERT_NEAR(reprojY[2], 3305425.14, 2);
}

TEST_F(ProjectionTests, SimplePolygonReprojectionTest) {
  // Boldrewood Campus,  University of Southampton, UK: EPSG4326 lon/lat =
  // -1.404611, 50.936923 Malaysia Campus,  University of Southampton, Malaysia:
  // EPSG4326 lon/lat = 103.611747, 1.430188 LC39A CCAFS, USA: EPSG4326 lon/lat
  // = -80.604146, 28.608259
  GEOSCoordSequence *coordSeq = GEOSCoordSeq_create(4, 2);

  GEOSCoordSeq_setOrdinate(coordSeq, 0, 0, 50.936923);
  GEOSCoordSeq_setOrdinate(coordSeq, 0, 1, -1.404611);

  GEOSCoordSeq_setOrdinate(coordSeq, 1, 0, 1.430188);
  GEOSCoordSeq_setOrdinate(coordSeq, 1, 1, 103.611747);

  GEOSCoordSeq_setOrdinate(coordSeq, 2, 0, 28.608259);
  GEOSCoordSeq_setOrdinate(coordSeq, 2, 1, -80.604146);

  GEOSCoordSeq_setOrdinate(coordSeq, 3, 0, 50.936923);
  GEOSCoordSeq_setOrdinate(coordSeq, 3, 1, -1.404611);

  GEOSGeometry *shell = GEOSGeom_createLinearRing(coordSeq);
  GEOSGeometry *polyIn = GEOSGeom_createPolygon(shell, nullptr, 0);

  GEOSGeometry *polyOut = reprojectPolygon(reproj, polyIn);

  // Ensure we get a polygon out as well
  ASSERT_STREQ("Polygon", GEOSGeomType(polyOut));

  // Ensure output coords are reprojected correctly
  auto outCoordSeq = GEOSGeom_getCoordSeq(GEOSGetExteriorRing(polyOut));

  double reprojX[4];
  double reprojY[4];

  GEOSCoordSeq_getOrdinate(outCoordSeq, 0, 0, &reprojX[0]);
  GEOSCoordSeq_getOrdinate(outCoordSeq, 0, 1, &reprojY[0]);

  GEOSCoordSeq_getOrdinate(outCoordSeq, 1, 0, &reprojX[1]);
  GEOSCoordSeq_getOrdinate(outCoordSeq, 1, 1, &reprojY[1]);

  GEOSCoordSeq_getOrdinate(outCoordSeq, 2, 0, &reprojX[2]);
  GEOSCoordSeq_getOrdinate(outCoordSeq, 2, 1, &reprojY[2]);

  GEOSCoordSeq_getOrdinate(outCoordSeq, 3, 0, &reprojX[3]);
  GEOSCoordSeq_getOrdinate(outCoordSeq, 3, 1, &reprojY[3]);

  ASSERT_NEAR(reprojX[0], -156360.60, 2);
  ASSERT_NEAR(reprojY[0], 6576946.28, 2);

  ASSERT_NEAR(reprojX[1], 11534006.92, 2);
  ASSERT_NEAR(reprojY[1], 158158.65, 2);

  ASSERT_NEAR(reprojX[2], -8972812.49, 2);
  ASSERT_NEAR(reprojY[2], 3305425.14, 2);

  ASSERT_NEAR(reprojX[3], -156360.60, 2);
  ASSERT_NEAR(reprojY[3], 6576946.28, 2);
}

TEST_F(ProjectionTests, SingleHolePolygonReprojectionTest) {
  // Create outer shell
  // Boldrewood Campus,  University of Southampton, UK: EPSG4326 lon/lat =
  // -1.404611, 50.936923 Malaysia Campus,  University of Southampton, Malaysia:
  // EPSG4326 lon/lat = 103.611747, 1.430188 LC39A CCAFS, USA: EPSG4326 lon/lat
  // = -80.604146, 28.608259
  GEOSCoordSequence *shellCoordSeq = GEOSCoordSeq_create(4, 2);

  GEOSCoordSeq_setOrdinate(shellCoordSeq, 0, 0, 50.936923);
  GEOSCoordSeq_setOrdinate(shellCoordSeq, 0, 1, -1.404611);

  GEOSCoordSeq_setOrdinate(shellCoordSeq, 1, 0, 1.430188);
  GEOSCoordSeq_setOrdinate(shellCoordSeq, 1, 1, 103.611747);

  GEOSCoordSeq_setOrdinate(shellCoordSeq, 2, 0, 28.608259);
  GEOSCoordSeq_setOrdinate(shellCoordSeq, 2, 1, -80.604146);

  GEOSCoordSeq_setOrdinate(shellCoordSeq, 3, 0, 50.936923);
  GEOSCoordSeq_setOrdinate(shellCoordSeq, 3, 1, -1.404611);

  GEOSGeometry *shell = GEOSGeom_createLinearRing(shellCoordSeq);

  // Create inner single hole
  // LC6, Vandenburg AFB, USA: EPSG4326 lon/lat = -120.626389, 34.581389
  // Pad1/5, Baikonur, Kazakhstan: EPSG4326 lon/lat = 63.342, 45.92
  // Main pad, Wenchang SLS, China: EPSG4326 lon/lat = 110.951133, 19.614492
  GEOSCoordSequence *holeCoordSeq = GEOSCoordSeq_create(4, 2);

  GEOSCoordSeq_setOrdinate(holeCoordSeq, 0, 0, 34.581389);
  GEOSCoordSeq_setOrdinate(holeCoordSeq, 0, 1, -120.626389);

  GEOSCoordSeq_setOrdinate(holeCoordSeq, 1, 0, 45.92);
  GEOSCoordSeq_setOrdinate(holeCoordSeq, 1, 1, 63.342);

  GEOSCoordSeq_setOrdinate(holeCoordSeq, 2, 0, 19.614492);
  GEOSCoordSeq_setOrdinate(holeCoordSeq, 2, 1, 110.951133);

  GEOSCoordSeq_setOrdinate(holeCoordSeq, 3, 0, 34.581389);
  GEOSCoordSeq_setOrdinate(holeCoordSeq, 3, 1, -120.626389);

  GEOSGeometry *holes[1];
  holes[0] = GEOSGeom_createLinearRing(holeCoordSeq);

  // Make the input polygon
  GEOSGeometry *polyIn = GEOSGeom_createPolygon(shell, holes, 1);

  // Repoject to new CRS
  GEOSGeometry *polyOut = reprojectPolygon(reproj, polyIn);

  // Ensure we get a polygon out as well
  ASSERT_STREQ("Polygon", GEOSGeomType(polyOut));

  // Ensure output coords are reprojected correctly
  auto outShellCoordSeq = GEOSGeom_getCoordSeq(GEOSGetExteriorRing(polyOut));

  double reprojShellX[4];
  double reprojShellY[4];

  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 0, 0, &reprojShellX[0]);
  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 0, 1, &reprojShellY[0]);

  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 1, 0, &reprojShellX[1]);
  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 1, 1, &reprojShellY[1]);

  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 2, 0, &reprojShellX[2]);
  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 2, 1, &reprojShellY[2]);

  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 3, 0, &reprojShellX[3]);
  GEOSCoordSeq_getOrdinate(outShellCoordSeq, 3, 1, &reprojShellY[3]);

  ASSERT_NEAR(reprojShellX[0], -156360.60, 2);
  ASSERT_NEAR(reprojShellY[0], 6576946.28, 2);

  ASSERT_NEAR(reprojShellX[1], 11534006.92, 2);
  ASSERT_NEAR(reprojShellY[1], 158158.65, 2);

  ASSERT_NEAR(reprojShellX[2], -8972812.49, 2);
  ASSERT_NEAR(reprojShellY[2], 3305425.14, 2);

  ASSERT_NEAR(reprojShellX[3], -156360.60, 2);
  ASSERT_NEAR(reprojShellY[3], 6576946.28, 2);

  double reprojHoleX[4];
  double reprojHoleY[4];

  // Ensure hole coords are reprojected correctly
  auto outHoleCoordSeq = GEOSGeom_getCoordSeq(GEOSGetInteriorRingN(polyOut, 0));

  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 0, 0, &reprojHoleX[0]);
  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 0, 1, &reprojHoleY[0]);

  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 1, 0, &reprojHoleX[1]);
  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 1, 1, &reprojHoleY[1]);

  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 2, 0, &reprojHoleX[2]);
  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 2, 1, &reprojHoleY[2]);

  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 3, 0, &reprojHoleX[3]);
  GEOSCoordSeq_getOrdinate(outHoleCoordSeq, 3, 1, &reprojHoleY[3]);

  ASSERT_NEAR(reprojHoleX[0], -13428068.20, 2);
  ASSERT_NEAR(reprojHoleY[0], 4082886.46, 2);

  ASSERT_NEAR(reprojHoleX[1], 7051199.19, 2);
  ASSERT_NEAR(reprojHoleY[1], 5736830.33, 2);

  ASSERT_NEAR(reprojHoleX[2], 12351023.63, 2);
  ASSERT_NEAR(reprojHoleY[2], 2213080.92, 2);

  ASSERT_NEAR(reprojHoleX[3], -13428068.20, 2);
  ASSERT_NEAR(reprojHoleY[3], 4082886.46, 2);
}

int main(int argc, char **argv) {
  initGEOS(notice, log_and_exit);

  ::testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();

  finishGEOS();

  return res;
}