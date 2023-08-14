/*
 * GeospatialGridMap.cpp
 *
 *  Created by A.Pilko on 18/06/2021.
 */

#include "uasgroundrisk/map_gen/GeospatialGridMap.h"

#include "../utils/GeometryProjectionUtils.h"
#include <tuple>
#include <spdlog/spdlog.h>

using namespace ugr::gridmap;

ugr::mapping::GeospatialGridMap::GeospatialGridMap(
	const std::array<float, 4> bounds, const float resolution, const char* worldSrs,
	const char* projectionSrs) : bounds(bounds), xyRes(resolution)
{
	assert(bounds[0] < bounds[2]); // South < North
	assert(bounds[1] < bounds[3]); // West < East
	spdlog::debug("Constructing Geospatial gridmap");
	std::tie(reproj, projCtx) = util::makeProjObject(worldSrs, projectionSrs);
	setBounds(bounds, resolution);
}

ugr::mapping::GeospatialGridMap::~GeospatialGridMap()
{
	spdlog::debug("Destructing Geospatial gridmap");
	//proj_cleanup();
}

GridMapDataType ugr::mapping::GeospatialGridMap::atPosition(const std::string& layerName, const double lon,
	const double lat) const
{
	const auto localIdx = world2Local(lon, lat);
	return layers.at(layerName)(localIdx[0], localIdx[1]);
}

GridMapDataType& ugr::mapping::GeospatialGridMap::atPosition(const std::string& layerName, const double lon,
	const double lat)
{
	const auto localIdx = world2Local(lon, lat);
	return layers.at(layerName)(localIdx[0], localIdx[1]);
}

GridMapDataType ugr::mapping::GeospatialGridMap::atPosition(const std::string& layerName, const Position& pos) const
{
	return atPosition(layerName, pos[0], pos[1]);
}

GridMapDataType& ugr::mapping::GeospatialGridMap::atPosition(const std::string& layerName, const Position& pos)
{
	return atPosition(layerName, pos[0], pos[1]);
}

ugr::gridmap::Index ugr::mapping::GeospatialGridMap::world2Local(const Position& worldCoord) const
{
	return world2Local(worldCoord[0], worldCoord[1]);
}

ugr::gridmap::Index ugr::mapping::GeospatialGridMap::world2Local(const double lon, const double lat) const
{
	const auto reprojCoord = proj_trans(reproj, PJ_FWD, { lat, lon });
	return {
		(reprojCoord.enu.e - projectionOrigin[0]) / xyRes, (reprojCoord.enu.n - projectionOrigin[1]) / xyRes
	};
}

Position ugr::mapping::GeospatialGridMap::local2World(const int x, const int y) const
{
	const auto reprojCoord = proj_trans(reproj, PJ_INV, {
		static_cast<double>(x) * xyRes + projectionOrigin[0],
		static_cast<double>(y) * xyRes + projectionOrigin[1]
	});
	return { reprojCoord.enu.n, reprojCoord.enu.e };
}

Position ugr::mapping::GeospatialGridMap::local2World(const gridmap::Index& localCoord) const
{
	return local2World(localCoord[0], localCoord[1]);
}

void ugr::mapping::GeospatialGridMap::eval()
{
}

void ugr::mapping::GeospatialGridMap::setBounds(
	const std::array<float, 4> boundsArr, const float resolution)
{
	spdlog::debug("Setting Geospatial gridmap bounds");
	// This reprojects EPSG:4326 to EPSG:3395 by default
	const auto swProjPoint = util::reprojectCoordinate_r(reproj, boundsArr[0], boundsArr[1]);
	this->projectionOrigin = { swProjPoint.enu.e, swProjPoint.enu.n, 0 };
	//TODO: Should an altitude be set in the projection origin?
	const auto neProjPoint = util::reprojectCoordinate_r(reproj, boundsArr[2], boundsArr[3]);
	const auto dx = std::abs(swProjPoint.enu.e - neProjPoint.enu.e);
	const auto dy = std::abs(swProjPoint.enu.n - neProjPoint.enu.n);
	const int xLength = static_cast<int>(dx / static_cast<float>(resolution));
	const int yLength = static_cast<int>(dy / static_cast<float>(resolution));

	// x,y are swapped here to match the expected orientation of the matrix if plotted.
	// Eigen uses row,column indexing, which would map to lat, lon not the other way around
	setGeometry(xLength, yLength);
}
