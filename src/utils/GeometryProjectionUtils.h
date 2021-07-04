/*
 * GeometryProjectionUtils.h
 *
 *  Created by A.Pilko on 10/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_UTILS_GEOMETRYPROJECTIONUTILS_H_
#define UASGROUNDRISK_SRC_UTILS_GEOMETRYPROJECTIONUTILS_H_

#include <geos_c.h>
#include <memory>
#include <proj.h>

/**
 * None of these are templated to geos::geom::Geometry types as each geometry
 * type has a different composition and needs special handling to reconstruct to
 * the same type. The most basic shared concept is a CoordinateSequence and all
 * the other geometry type handlers call this is different combinations and
 * reconstruct the results.
 */

namespace ugr {
namespace util {
/**
 * Reproject a GEOS CoordSequence using a PROJ reprojector.
 * @param reprojector a pointer to the PROJ reprojector
 * @param in a pointer to the coordinate sequence to reproject
 * @return the reprojected coordinate sequence
 */
static GEOSCoordSequence *reprojectCoordinates(PJ *reprojector,
                                               const GEOSCoordSequence *in) {
  unsigned int nCoords;
  GEOSCoordSeq_getSize(in, &nCoords);

  // Write reprojected coords to CoordSeq
  // Coordinate ndims fixed for now
  auto *outCoordSeq = GEOSCoordSeq_create(nCoords, 3);
  for (unsigned int i = 0; i < nCoords; i++) {
    // Read in coords
    double cx, cy, cz;
    GEOSCoordSeq_getOrdinate(in, i, 0, &cx);
    GEOSCoordSeq_getOrdinate(in, i, 1, &cy);
    GEOSCoordSeq_getOrdinate(in, i, 2, &cz);

    // Reproject coords
    PJ_COORD c = proj_trans(reprojector, PJ_FWD, proj_coord(cx, cy, cz, 0));

    // Write reprojected coords to output coord seq
    GEOSCoordSeq_setOrdinate(outCoordSeq, i, 0, c.xyz.x);
    GEOSCoordSeq_setOrdinate(outCoordSeq, i, 1, c.xyz.y);
    GEOSCoordSeq_setOrdinate(outCoordSeq, i, 2, c.xyz.z);
  }
  return outCoordSeq;
}

/**
 * Reproject a GEOS Polygon using a PROJ reprojector
 * @param reprojector a pointer to the PROJ reprojector
 * @param in a pointer to the GEOSGeometry to reproject. This must be of type
 * POLYGON.
 * @return a pointer to the reprojected polygon
 */
static GEOSGeometry *reprojectPolygon(PJ *reprojector, const GEOSGeometry *in) {
  // Reproject outer ring/shell
  const GEOSGeometry *exteriorRing = GEOSGetExteriorRing(in);
  auto reprojShellCoords = reinterpret_cast<GEOSCoordSequence *>(
      reprojectCoordinates(reprojector, GEOSGeom_getCoordSeq(exteriorRing)));
  auto *shell = GEOSGeom_createLinearRing(reprojShellCoords);

  // Reproject each of the inner rings
  int nHoles = GEOSGetNumInteriorRings(in);
  GEOSGeom *innerGeometries = new GEOSGeom[nHoles];
  for (int i = 0; i < nHoles; ++i) {
    auto innerGeom = GEOSGetInteriorRingN(in, i);
    auto innerGeomCoordSeq = GEOSGeom_getCoordSeq(innerGeom);

    auto reprojGeom = reprojectCoordinates(reprojector, innerGeomCoordSeq);
    innerGeometries[i] = GEOSGeom_createLinearRing(reprojGeom);
  }

  auto *poly = GEOSGeom_createPolygon(shell, innerGeometries, nHoles);
  delete[] innerGeometries;
  return poly;
}

/**
 * Reproject a single coordinate point.
 * This recreates a PROJ context on every call so other methods should be used
 * if repeated reprojection is required.
 *
 * @param coordX coordinate X value
 * @param coordY coordinate Y value
 * @param coordZ coordinate Z value
 * @param coordT coordinate T value
 * @param sourceCRS the CRS the coordinate is currently in
 * @param destCRS the desired output CRS
 * @return the reprojected coordinate
 */
static PJ_COORD reprojectCoordinate(double coordX, double coordY,
                                    double coordZ = 0, double coordT = 0,
                                    const char *sourceCRS = "EPSG:4326",
                                    const char *destCRS = "EPSG:3395") {
  auto *projCtx = proj_context_create();
#ifdef PROJ_DATA_PATH
  const char *projDataPaths[1];
  projDataPaths[0] = PROJ_DATA_PATH;
  proj_context_set_search_paths(projCtx, 1, projDataPaths);
#endif
  auto *reproj = proj_create_crs_to_crs(projCtx, sourceCRS, destCRS, nullptr);
  PJ_COORD out =
      proj_trans(reproj, PJ_FWD, proj_coord(coordX, coordY, coordZ, coordT));
  proj_destroy(reproj);
  proj_context_destroy(projCtx);
  return out;
}

} // namespace util
} // namespace ugr

#endif // UASGROUNDRISK_SRC_UTILS_GEOMETRYPROJECTIONUTILS_H_
