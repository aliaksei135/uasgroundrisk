/*
 * GeospatialGridMap.h
 *
 *  Created by A.Pilko on 18/06/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_GEOSPATIALGRIDMAP_H_
#define UASGROUNDRISK_SRC_MAP_GEN_GEOSPATIALGRIDMAP_H_

#include <array>
#include <proj.h>

#include "../gridmap/GridMap.h"

namespace ugr
{
	namespace mapping
	{
		using namespace Eigen;
		using namespace gridmap;

		class GeospatialGridMap : public GridMap
		{
		public:
			/**
			 * @brief A GridMap with specified in terms of a geospatial projection
			 * @param bounds the (S,W,N,E) bounds in the world projection
			 * @param resolution the resolution of the grid in metres
			 * @param worldSrs the world projection to use
			 * @param projectionSrs the local projection that has UoM in metres
			*/
			GeospatialGridMap(std::array<float, 4> bounds, float resolution, const char* worldSrs = "EPSG:4326",
			                  const char* projectionSrs = "EPSG:3395");

			~GeospatialGridMap();

			std::array<float, 4> getBounds() const { return bounds; }
			float getResolution() const { return xyRes; }

			/**
			 * @brief Return the value of the coefficient at x,y on the given layer
			 * @param layerName layer to use
			 * @param lon longitude
			 * @param lat latitude
			 * @return the coefficient value
			*/
			GridMapDataType atPosition(const std::string& layerName, double lon, double lat) const;
			GridMapDataType& atPosition(const std::string& layerName, double lon, double lat);
			GridMapDataType atPosition(const std::string& layerName, const Position& pos) const;
			GridMapDataType& atPosition(const std::string& layerName, const Position& pos);

			/**
			* @brief Reproject world (EPSG:4326) coordinates to local indices
			* @param worldCoord the world coordinates to reproject
			* @return the local indices
			*/
			Index world2Local(const Position& worldCoord) const;

			/**
			 * @brief Reproject world (EPSG:4326) coordinates to local indices
			 * @param lon longitude
			 * @param lat latitude
			 * @return vector of local indices
			*/
			Index world2Local(double lon, double lat) const;


			/**
			 * @brief Reproject local indices to world (EPSG:4326) coordinates
			 * @param x local x
			 * @param y local y
			 * @return a vector of world coordinates
			*/
			Position local2World(int x, int y) const;

			/**
			 * @brief Reproject local indices to world (EPSG:4326) coordinates
			 * @param localCoord a vector of local indices
			 * @return a vector of world coordinates
			*/
			Position local2World(const Index& localCoord) const;

			void eval()
			{
			};

		protected:
			void setBounds(std::array<float, 4> boundsArr, float resolution);

			std::array<float, 4> bounds;

			float xyRes;

			Vector3d projectionOrigin; // The origin in local projection coords

			PJ* reproj;
			PJ_CONTEXT* projCtx;
		};
	} // namespace mapping
} // namespace ugr
#endif // UASGROUNDRISK_SRC_MAP_GEN_GEOSPATIALGRIDMAP_H_
