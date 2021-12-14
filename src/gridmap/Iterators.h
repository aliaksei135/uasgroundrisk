#ifndef ITERATORS_H
#define ITERATORS_H

#include "../map_gen/GeospatialGridMap.h"
#include "../utils/GeometryOperations.h"
#include <memory>


namespace ugr
{
	namespace gridmap
	{
		class SubmapIterator
		{
		public:
			SubmapIterator(const GridMap& gridmap, const Index& start, const Size& submapSize) : start(start),
				currIndex(start), gridmapSize(gridmap.getSize()), submapSize(submapSize), _isPastEnd(false)
			{
				if (!gridmap.isInBounds(start))
				{
					_isPastEnd = true;
				}
			}

			const Index& operator *() const { return currIndex; }

			SubmapIterator& operator ++()
			{
				const auto outsideGlobalMap = currIndex + 1 >= gridmapSize;
				const auto outsideSubmap = currIndex - start > submapSize;
				const auto outside = outsideGlobalMap || outsideSubmap;
				if (outside[0])
				{
					currIndex[0] = 0;
					++currIndex[1];
				}
				else
				{
					++currIndex[0];
				}
				if (outside[1])
				{
					_isPastEnd = true;
				}
				return *this;
			}


			bool isPastEnd() const { return _isPastEnd; }
			Size getSize() const { return submapSize; }

		protected:
			Index start;
			Index currIndex;

			Size gridmapSize;
			Size submapSize;
			bool _isPastEnd;
		};

		class PolygonIterator
		{
		private:
			void setup(const GridMap& gridmap, const Polygon& poly)
			{
				polygon = poly;
				const auto polyBounds = util::getPolygonBounds<int>(polygon);
				boundSize = {polyBounds[3] - polyBounds[1], polyBounds[2] - polyBounds[0]};

				// ReSharper disable once CppSmartPointerVsMakeFunction
				// Can't forward args through make function
				boundsIter = std::shared_ptr<SubmapIterator>(
					new SubmapIterator(gridmap, {polyBounds[1], polyBounds[0]}, boundSize));
			}

		public:
			PolygonIterator(const GridMap& gridmap, const Polygon& polygon)
			{
				setup(gridmap, polygon);
			}


			PolygonIterator(const mapping::GeospatialGridMap& gridmap, const GeoPolygon& polygon)
			{
				Polygon localPolygon;
				localPolygon.reserve(polygon.size());
				for (const auto& point : polygon)
				{
					localPolygon.emplace_back(gridmap.world2Local(point));
				}
				setup(gridmap, localPolygon);
			}

			const Index& operator *() const { return *(*boundsIter); }

			PolygonIterator& operator ++()
			{
				do
				{
					++(*boundsIter);
					if (isPastEnd())
					{
						break;
					}
				}
				while (!util::isInsidePolygon(polygon, **boundsIter));

				return *this;
			}

			bool isPastEnd() const { return boundsIter->isPastEnd(); }

		protected:
			Polygon polygon;
			std::shared_ptr<SubmapIterator> boundsIter;
			Size boundSize;
		};
	}
}
#endif // ITERATORS_H
