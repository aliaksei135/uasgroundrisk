#ifndef GEOMETRYOPERATIONS_H
#define GEOMETRYOPERATIONS_H
#include "../gridmap/TypeDefs.h"
#include <array>

namespace ugr
{
	namespace util
	{
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

			return {minY, minX, maxY, maxX};
		}

		template <typename T>
		static bool isInsidePolygon(const gridmap::Polygon& polygon, const T& position)
		{
			unsigned cross = 0;
			for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
			{
				if (polygon[i].y() > position.y() != polygon[j].y() > position.y()
					&& position.x() < (polygon[j].x() - polygon[i].x()) * (position.y() - polygon[i].y()) /
					(polygon[j].y() - polygon[i].y()) + polygon[i].x())
				{
					cross++;
				}
			}
			return cross % 2;
		}
	}
}
#endif // GEOMETRYOPERATIONS_H
