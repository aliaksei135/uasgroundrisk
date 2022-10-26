#ifndef GEOMETRYOPERATIONS_H
#define GEOMETRYOPERATIONS_H
#include "uasgroundrisk/gridmap/TypeDefs.h"
#include <array>
#include <map>
#include <geos_c.h>
#include "DefaultGEOSMessageHandlers.h"
#include <vector>

namespace ugr
{
	namespace util
	{
		namespace detail
		{
			template <typename T = int_fast16_t>
			static std::vector<gridmap::Index, Eigen::aligned_allocator<gridmap::Index>> _bresenham2D_high(
				gridmap::Index i1, gridmap::Index i2)
			{
				std::vector<gridmap::Index, Eigen::aligned_allocator<gridmap::Index>> out;

				const T x1 = i1[0];
				const T x2 = i2[0];
				const T y1 = i1[1];
				const T y2 = i2[1];

				T dx = x2 - x1;
				T dy = y2 - y1;

				const T mx = std::max(x1, x2);
				const int_fast8_t xi = dx < 0 ? -1 : 1;
				if (dx < 0)
				{
					dx = -dx;
				}

				T d = 2 * dx - dy;
				T x = x1;
				T y = y1;
				out.reserve(dy + 1); // Over allocate
				const int_fast8_t sy = y1 < y2 ? 1 : -1;

				for (int i = 0; i < dy; ++i)
				{
					out.emplace_back(x, y);
					y += sy;
					if (d > 0)
					{
						x += xi;
						d += 2 * (dx - dy);
					}
					else
					{
						d += 2 * dx;
					}
				}
				out.emplace_back(i2);
				return out;
			}

			template <typename T = int_fast16_t>
			static std::vector<gridmap::Index, Eigen::aligned_allocator<gridmap::Index>> _bresenham2D_low(
				gridmap::Index i1, gridmap::Index i2)
			{
				std::vector<gridmap::Index, Eigen::aligned_allocator<gridmap::Index>> out;

				const T x1 = i1[0];
				const T x2 = i2[0];
				const T y1 = i1[1];
				const T y2 = i2[1];

				T dx = x2 - x1;
				T dy = y2 - y1;

				const T my = std::max(y1, y2);
				const int_fast8_t yi = dy < 0 ? -1 : 1;
				if (dy < 0)
				{
					dy = -dy;
				}

				T d = 2 * dy - dx;
				T x = x1;
				T y = y1;
				out.reserve(dx + 1); // Over allocate
				const int_fast8_t sx = x1 < x2 ? 1 : -1;

				for (int i = 0; i < dx; ++i)
				{
					out.emplace_back(x, y);
					x += sx;
					if (d > 0)
					{
						y += yi;
						d += 2 * (dy - dx);
					}
					else
					{
						d += 2 * dy;
					}
				}
				out.emplace_back(i2);
				return out;
			}
		}

		template <typename T = int_fast16_t>
		static std::vector<gridmap::Index, Eigen::aligned_allocator<gridmap::Index>> bresenham2D(
			gridmap::Index i1, gridmap::Index i2)
		{
			T dx = abs(i2[0] - i1[0]);
			T dy = abs(i2[1] - i1[1]);

			if (dy < dx)
			{
				if (i1[0] > i2[0])
					return detail::_bresenham2D_low<T>(i2, i1);
				return detail::_bresenham2D_low<T>(i1, i2);
			}
			if (i1[1] > i2[1])
				return detail::_bresenham2D_high<T>(i2, i1);
			return detail::_bresenham2D_high<T>(i1, i2);
		}

		template <typename T>
		static std::array<T, 4> getPolygonBounds(const gridmap::Polygon& polygon)
		{
			T minX = std::numeric_limits<T>::max();
			T maxX = std::numeric_limits<T>::min();
			T minY = std::numeric_limits<T>::max();
			T maxY = std::numeric_limits<T>::min();

			for (const auto& pos : polygon)
			{
				const auto x = pos[0];
				const auto y = pos[1];

				if (x > maxX)
				{
					maxX = x;
				}
				if (x < minX)
				{
					minX = x;
				}

				if (y > maxY)
				{
					maxY = y;
				}
				if (y < minY)
				{
					minY = y;
				}
			}

			return { minY, minX, maxY, maxX };
		}

		template <typename P, typename T>
		static bool isInsidePolygon(const P& polygon, const T& position)
		{
			unsigned cross = 0;
			for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
			{
				if (polygon[i].y() > position.y() != polygon[j].y() > position.y()
					&& position.x() < (polygon[j].x() - polygon[i].x()) * (position.y() - polygon[i].y()) /
					(polygon[j].y() - polygon[i].y()) + polygon[i].x())

					cross++;
			}
			return cross % 2;
		}

		template <typename T>
		static std::map<GEOSGeometry*, T> boundGeometriesMap(std::map<GEOSGeometry*, T>& geomMap,
			const std::array<float, 4>& bounds)
		{
			initGEOS(notice, log_and_exit);

			// Create bounding box as a GEOS Geometry
			auto* boundingCoordSeq = GEOSCoordSeq_create(5, 2); //alloc
			GEOSCoordSeq_setXY(boundingCoordSeq, 0, bounds[1], bounds[0]); //SW corner
			GEOSCoordSeq_setXY(boundingCoordSeq, 1, bounds[1], bounds[2]); //NW corner
			GEOSCoordSeq_setXY(boundingCoordSeq, 2, bounds[3], bounds[2]); //NE corner
			GEOSCoordSeq_setXY(boundingCoordSeq, 3, bounds[3], bounds[0]); //SE corner
			GEOSCoordSeq_setXY(boundingCoordSeq, 4, bounds[1], bounds[0]); //SW corner to close ring
			auto* boundingGeom = GEOSGeom_createLinearRing(boundingCoordSeq);

			std::map<GEOSGeometry*, T> outMap;

			for (auto& pair : geomMap)
			{
				auto* inGeom = pair.first;
				auto* intersection = GEOSIntersection(boundingGeom, inGeom);
				if (intersection != nullptr)
				{
					auto type = GEOSGeomTypeId(intersection);
					if (GEOSGeomTypeId(intersection) != GEOSGeomTypes::GEOS_POLYGON)
					{
						auto* cs = GEOSCoordSeq_clone(GEOSGeom_getCoordSeq(intersection));
						auto* lr = GEOSGeom_createLinearRing(cs);
						auto* poly = GEOSGeom_createPolygon(lr, nullptr, 0);
						auto t = GEOSGeomType(poly);
						outMap.emplace(poly, pair.second);
					}
					else
					{
						outMap.emplace(intersection, pair.second);
					}
				}


				// This is either intersected in which case a new geometry is produced,
				// or it out of bounds in which case we don't want it anyway
				GEOSGeom_destroy(inGeom);
			}

			finishGEOS();
			return outMap;
		}

		template <typename T>
		static std::map<GEOSGeometry*, T> boundGeometriesMap_r(std::map<GEOSGeometry*, T>& geomMap,
			const std::array<float, 4>& bounds,
			const GEOSContextHandle_t& geosCtx)
		{
			// Create bounding box as a GEOS Geometry
			auto* boundingCoordSeq = GEOSCoordSeq_create_r(geosCtx, 5, 2); //alloc
			GEOSCoordSeq_setXY_r(geosCtx, boundingCoordSeq, 0, bounds[1], bounds[0]); //SW corner
			GEOSCoordSeq_setXY_r(geosCtx, boundingCoordSeq, 1, bounds[1], bounds[2]); //NW corner
			GEOSCoordSeq_setXY_r(geosCtx, boundingCoordSeq, 2, bounds[3], bounds[2]); //NE corner
			GEOSCoordSeq_setXY_r(geosCtx, boundingCoordSeq, 3, bounds[3], bounds[0]); //SE corner
			GEOSCoordSeq_setXY_r(geosCtx, boundingCoordSeq, 4, bounds[1], bounds[0]); //SW corner to close ring
			auto* boundingLR = GEOSGeom_createLinearRing_r(geosCtx, boundingCoordSeq);
			auto* boundingPoly = GEOSGeom_createPolygon_r(geosCtx, boundingLR, nullptr, 0);
			auto* prepBoundingPoly = GEOSPrepare_r(geosCtx, boundingPoly);

			std::map<GEOSGeometry*, T> outMap;

			for (auto& pair : geomMap)
			{
				auto* inGeom = pair.first;
				auto isIntersecting = GEOSPreparedIntersects_r(geosCtx, prepBoundingPoly, inGeom);
				if (isIntersecting)
				{
					auto* intersection = GEOSIntersection_r(geosCtx, boundingPoly, inGeom);
					if (intersection != nullptr)
						outMap.emplace(intersection, pair.second);
				}
				// This is either intersected in which case a new geometry is produced,
				// or it out of bounds in which case we don't want it anyway
				GEOSGeom_destroy_r(geosCtx, inGeom);
			}
			GEOSPreparedGeom_destroy_r(geosCtx, prepBoundingPoly);
			GEOSGeom_destroy_r(geosCtx, boundingPoly);
			return outMap;
		}

		template <typename T>
		static T getGeometryArea(GEOSGeometry* geom)
		{
			double area;
			GEOSArea(geom, &area);
			return static_cast<T>(area);
		}

		template <typename T>
		static T getGeometryArea_r(GEOSGeometry* geom, const GEOSContextHandle_t& geosCtx)
		{
			double area;
			GEOSArea_r(geosCtx, geom, &area);
			return static_cast<T>(area);
		}

		static gridmap::GeoPolygon asGeoPolygon(const GEOSGeometry* geom)
		{
			const auto* er = GEOSGetExteriorRing(geom);
			const auto* cs = GEOSGeom_getCoordSeq(er);
			const auto nPoints = GEOSGetNumCoordinates(er);
			double x, y;

			gridmap::GeoPolygon geoPoly;
			geoPoly.reserve(nPoints);

			for (int i = 0; i < nPoints; ++i)
			{
				GEOSCoordSeq_getXY(cs, i, &x, &y);
				geoPoly.push_back({ x, y });
			}
			return geoPoly;
		}

		static gridmap::GeoPolygon asGeoPolygon_r(const GEOSGeometry* geom, const GEOSContextHandle_t& geosCtx)
		{
			const auto* er = GEOSGetExteriorRing_r(geosCtx, geom);
			if (er == nullptr) return {};
			const auto* cs = GEOSGeom_getCoordSeq_r(geosCtx, er);
			const auto nPoints = GEOSGetNumCoordinates_r(geosCtx, er);
			double x, y;

			gridmap::GeoPolygon geoPoly;
			geoPoly.reserve(nPoints);

			for (int i = 0; i < nPoints; ++i)
			{
				GEOSCoordSeq_getXY_r(geosCtx, cs, i, &x, &y);
				geoPoly.push_back({ x, y });
			}
			return geoPoly;
		}

		template <typename C, typename T = typename C::value_type>
		static void destroyGEOSGeoms(const C& container)
		{
			for (GEOSGeometry* geom : container)
			{
				if (geom != nullptr)
					GEOSGeom_destroy(geom);
			}
		}

		template <typename C, typename T = typename C::value_type>
		static void destroyGEOSGeoms_r(const C& container, const GEOSContextHandle_t& geosCtx)
		{
			for (GEOSGeometry* geom : container)
			{
				GEOSGeom_destroy_r(geosCtx, geom);
			}
		}
	}
}

#endif // GEOMETRYOPERATIONS_H
