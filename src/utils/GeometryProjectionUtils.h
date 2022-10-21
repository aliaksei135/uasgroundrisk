/*
 * GeometryProjectionUtils.h
 *
 *  Created by A.Pilko on 10/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_UTILS_GEOMETRYPROJECTIONUTILS_H_
#define UASGROUNDRISK_SRC_UTILS_GEOMETRYPROJECTIONUTILS_H_

#include <geos_c.h>
#include <proj.h>
#include <tuple>
#include <string>
#include <cstdlib>
#include "spdlog/spdlog.h"

 /**
  * None of these are templated to geos::geom::Geometry types as each geometry
  * type has a different composition and needs special handling to reconstruct to
  * the same type. The most basic shared concept is a CoordinateSequence and all
  * the other geometry type handlers call this is different combinations and
  * reconstruct the results.
  */

namespace ugr
{
	namespace util
	{
		/**
		 * \brief Create a PROJ context and projection object. Caller becomes responsible for their destruction.
		 * \param sourceCRS source CRS
		 * \param destCRS destination CRS
		 * \return a tuple of {projection object, context} pointers
		 */
		static std::tuple<PJ*, PJ_CONTEXT*> makeProjObject(const char* sourceCRS = "EPSG:4326",
			const char* destCRS = "EPSG:3395")
		{
			PJ_CONTEXT* projCtx = proj_context_create();
			spdlog::info("uasgroundrisk: Creating PROJ obj...");
			const auto* envDataDir = std::getenv("PROJ_LIB");
			if (envDataDir == nullptr)
			{
				spdlog::info("uasgroundrisk: PROJ_LIB not set. Falling back");
#ifdef PROJ_DATA_PATH
				const char* projDataPaths[1];
				projDataPaths[0] = PROJ_DATA_PATH;
				spdlog::info("uasgroundrisk: Using Internally set PROJ data dir: {0}", projDataPaths[0]);
				proj_context_set_search_paths(projCtx, 1, projDataPaths);
#endif
			}
			PJ* reproj = proj_create_crs_to_crs(projCtx, sourceCRS, destCRS, nullptr);
			return { reproj, projCtx };
		}

		static GEOSCoordSequence* swapCoordOrder(const GEOSCoordSequence* inCS)
		{
			unsigned nCoord, nDim;
			GEOSCoordSeq_getDimensions(inCS, &nDim);
			GEOSCoordSeq_getSize(inCS, &nCoord);
			auto* invertedCs = GEOSCoordSeq_create(nCoord, nDim);
			double a, z;
			for (int n = 0; n < nCoord; ++n)
			{
				GEOSCoordSeq_getOrdinate(inCS, n, 0, &a);
				GEOSCoordSeq_setOrdinate(invertedCs, n, 1, a);
				GEOSCoordSeq_getOrdinate(inCS, n, 1, &a);
				GEOSCoordSeq_setOrdinate(invertedCs, n, 0, a);
				if (nDim > 2)
				{
					GEOSCoordSeq_getOrdinate(inCS, n, 2, &z);
					GEOSCoordSeq_setOrdinate(invertedCs, n, 2, z);
				}
			}
			return invertedCs;
		}

		static GEOSCoordSequence* swapCoordOrder_r(const GEOSCoordSequence* inCS, const GEOSContextHandle_t& geosCtx)
		{
			unsigned nCoord, nDim;
			GEOSCoordSeq_getDimensions_r(geosCtx, inCS, &nDim);
			GEOSCoordSeq_getSize_r(geosCtx, inCS, &nCoord);
			auto* invertedCs = GEOSCoordSeq_create_r(geosCtx, nCoord, nDim);
			double a, z;
			for (int n = 0; n < nCoord; ++n)
			{
				GEOSCoordSeq_getOrdinate_r(geosCtx, inCS, n, 0, &a);
				GEOSCoordSeq_setOrdinate_r(geosCtx, invertedCs, n, 1, a);
				GEOSCoordSeq_getOrdinate_r(geosCtx, inCS, n, 1, &a);
				GEOSCoordSeq_setOrdinate_r(geosCtx, invertedCs, n, 0, a);
				if (nDim > 2)
				{
					GEOSCoordSeq_getOrdinate_r(geosCtx, inCS, n, 2, &z);
					GEOSCoordSeq_setOrdinate_r(geosCtx, invertedCs, n, 2, z);
				}
			}
			return invertedCs;
		}

		static GEOSGeometry* swapCoordOrder(const GEOSGeometry* inGeom)
		{
			const auto* er = GEOSGetExteriorRing(inGeom);
			const auto* erCs = GEOSGeom_getCoordSeq(inGeom);
			auto* invertedErCs = swapCoordOrder(erCs);
			auto* invertedErGeom = GEOSGeom_createLinearRing(invertedErCs);

			const auto nHoles = GEOSGetNumInteriorRings(inGeom);
			std::vector<GEOSGeometry*> invertedHoles(nHoles);
			for (int i = 0; i < nHoles; ++i)
			{
				const auto* irGeom = GEOSGetInteriorRingN(inGeom, i);
				const auto* irCs = GEOSGeom_getCoordSeq(irGeom);
				auto* invertedIrCs = swapCoordOrder(irCs);
				invertedHoles[i] = GEOSGeom_createLinearRing(invertedIrCs);
			}
			return GEOSGeom_createPolygon(invertedErGeom, invertedHoles.data(), nHoles);
		}

		static GEOSGeometry* swapCoordOrder_r(const GEOSGeometry* inGeom, const GEOSContextHandle_t& geosCtx)
		{
			const auto* er = GEOSGetExteriorRing_r(geosCtx, inGeom);
			const auto* erCs = GEOSGeom_getCoordSeq_r(geosCtx, er);
			auto* invertedErCs = swapCoordOrder_r(erCs, geosCtx);
			auto* invertedErGeom = GEOSGeom_createLinearRing_r(geosCtx, invertedErCs);

			const auto nHoles = GEOSGetNumInteriorRings_r(geosCtx, inGeom);
			std::vector<GEOSGeometry*> invertedHoles(nHoles);
			for (int i = 0; i < nHoles; ++i)
			{
				const auto* irGeom = GEOSGetInteriorRingN_r(geosCtx, inGeom, i);
				const auto* irCs = GEOSGeom_getCoordSeq_r(geosCtx, irGeom);
				auto* invertedIrCs = swapCoordOrder_r(irCs, geosCtx);
				invertedHoles[i] = GEOSGeom_createLinearRing_r(geosCtx, invertedIrCs);
			}
			return GEOSGeom_createPolygon_r(geosCtx, invertedErGeom, invertedHoles.data(), nHoles);
		}


		/**
		 * Reproject a GEOS CoordSequence using a PROJ reprojector.
		 * @param reprojector a pointer to the PROJ reprojector
		 * @param in a pointer to the coordinate sequence to reproject
		 * @return the reprojected coordinate sequence
		 */
		static GEOSCoordSequence* reprojectCoordinates(PJ* reprojector,
			const GEOSCoordSequence* in)
		{
			unsigned int nCoords, nDims;
			GEOSCoordSeq_getSize(in, &nCoords);
			GEOSCoordSeq_getDimensions(in, &nDims);

			// Write reprojected coords to CoordSeq
			// Coordinate ndims fixed for now
			auto* outCoordSeq = GEOSCoordSeq_create(nCoords, nDims);
			for (unsigned int i = 0; i < nCoords; i++)
			{
				// Read in coords
				double cx, cy, cz;
				GEOSCoordSeq_getOrdinate(in, i, 0, &cx);
				GEOSCoordSeq_getOrdinate(in, i, 1, &cy);
				if (nDims > 2)
				{
					GEOSCoordSeq_getOrdinate(in, i, 2, &cz);
				}
				else
				{
					cz = 0;
				}

				// Reproject coords
				PJ_COORD c = proj_trans(reprojector, PJ_FWD, proj_coord(cx, cy, cz, 0));

				// Write reprojected coords to output coord seq
				GEOSCoordSeq_setOrdinate(outCoordSeq, i, 0, c.xyz.x);
				GEOSCoordSeq_setOrdinate(outCoordSeq, i, 1, c.xyz.y);
				if (nDims > 2)
				{
					GEOSCoordSeq_setOrdinate(outCoordSeq, i, 2, c.xyz.z);
				}
			}
			return outCoordSeq;
		}

		/**
		 * \brief A reentrant version of above
		 */
		static GEOSCoordSequence* reprojectCoordinates_r(PJ* reprojector,
			const GEOSCoordSequence* in,
			const GEOSContextHandle_t& geosCtx)
		{
			unsigned int nCoords, nDims;
			GEOSCoordSeq_getSize_r(geosCtx, in, &nCoords);
			GEOSCoordSeq_getDimensions_r(geosCtx, in, &nDims);

			// Write reprojected coords to CoordSeq
			// Coordinate ndims fixed for now
			auto* outCoordSeq = GEOSCoordSeq_create_r(geosCtx, nCoords, nDims);
			for (unsigned int i = 0; i < nCoords; i++)
			{
				// Read in coords
				double cx, cy, cz;
				GEOSCoordSeq_getOrdinate_r(geosCtx, in, i, 0, &cx);
				GEOSCoordSeq_getOrdinate_r(geosCtx, in, i, 1, &cy);
				if (nDims > 2)
				{
					GEOSCoordSeq_getOrdinate_r(geosCtx, in, i, 2, &cz);
				}
				else
				{
					cz = 0;
				}

				// Reproject coords
				PJ_COORD c = proj_trans(reprojector, PJ_FWD, proj_coord(cx, cy, cz, 0));

				// Write reprojected coords to output coord seq
				GEOSCoordSeq_setOrdinate_r(geosCtx, outCoordSeq, i, 0, c.xyz.x);
				GEOSCoordSeq_setOrdinate_r(geosCtx, outCoordSeq, i, 1, c.xyz.y);
				if (nDims > 2)
				{
					GEOSCoordSeq_setOrdinate_r(geosCtx, outCoordSeq, i, 2, c.xyz.z);
				}
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
		static GEOSGeometry* reprojectPolygon(PJ* reprojector, const GEOSGeometry* in)
		{
			// Reproject outer ring/shell

			const GEOSGeometry* exteriorRing = GEOSGetExteriorRing(in);
			const auto reprojShellCoords = reprojectCoordinates(reprojector, GEOSGeom_getCoordSeq(exteriorRing));
			auto* shell = GEOSGeom_createLinearRing(reprojShellCoords);

			// Reproject each of the inner rings
			const int nHoles = GEOSGetNumInteriorRings(in);
			auto* innerGeometries = new GEOSGeom[nHoles];
			for (int i = 0; i < nHoles; ++i)
			{
				const auto innerGeom = GEOSGetInteriorRingN(in, i);
				const auto innerGeomCoordSeq = GEOSGeom_getCoordSeq(innerGeom);

				const auto reprojGeom = reprojectCoordinates(reprojector, innerGeomCoordSeq);
				innerGeometries[i] = GEOSGeom_createLinearRing(reprojGeom);
			}

			auto* poly = GEOSGeom_createPolygon(shell, innerGeometries, nHoles);
			delete[] innerGeometries;
			return poly;
		}

		/**
		 * \brief A reentrant version of above
		 */
		static GEOSGeometry* reprojectPolygon_r(PJ* reprojector, const GEOSGeometry* in,
			const GEOSContextHandle_t& geosCtx)
		{
			// Reproject outer ring/shell
			const GEOSGeometry* exteriorRing = GEOSGetExteriorRing_r(geosCtx, in);
			const auto n = GEOSGetNumCoordinates_r(geosCtx, exteriorRing);
			const auto reprojShellCoords = reprojectCoordinates_r(reprojector,
				GEOSGeom_getCoordSeq_r(geosCtx, exteriorRing),
				geosCtx);
			auto* shell = GEOSGeom_createLinearRing_r(geosCtx, reprojShellCoords);

			// Reproject each of the inner rings
			const int nHoles = GEOSGetNumInteriorRings_r(geosCtx, in);
			auto* innerGeometries = new GEOSGeom[nHoles];
			for (int i = 0; i < nHoles; ++i)
			{
				const auto innerGeom = GEOSGetInteriorRingN_r(geosCtx, in, i);
				const auto innerGeomCoordSeq = GEOSGeom_getCoordSeq_r(geosCtx, innerGeom);

				const auto reprojGeom = reprojectCoordinates_r(reprojector, innerGeomCoordSeq, geosCtx);
				innerGeometries[i] = GEOSGeom_createLinearRing_r(geosCtx, reprojGeom);
			}

			auto* poly = GEOSGeom_createPolygon_r(geosCtx, shell, innerGeometries, nHoles);
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
		static PJ_COORD reprojectCoordinate(const double coordX, const double coordY,
			const double coordZ = 0, const double coordT = 0,
			const char* sourceCRS = "EPSG:4326",
			const char* destCRS = "EPSG:3395")
		{
			auto projObjs = makeProjObject(sourceCRS, destCRS);
			auto* reproj = std::get<0>(projObjs);
			auto* projCtx = std::get<1>(projObjs);
			const PJ_COORD out =
				proj_trans(reproj, PJ_FWD, proj_coord(coordX, coordY, coordZ, coordT));
			proj_destroy(reproj);
			proj_context_destroy(projCtx);
			return out;
		}

		/**
		 * A reentrant version of above
				 */
		static PJ_COORD reprojectCoordinate_r(PJ* reproj,
			const double coordX, const double coordY,
			const double coordZ = 0, const double coordT = 0,
			const char* sourceCRS = "EPSG:4326",
			const char* destCRS = "EPSG:3395")
		{
			const PJ_COORD out =
				proj_trans(reproj, PJ_FWD, proj_coord(coordX, coordY, coordZ, coordT));
			return out;
		}
	} // namespace util
} // namespace ugr

#endif // UASGROUNDRISK_SRC_UTILS_GEOMETRYPROJECTIONUTILS_H_
