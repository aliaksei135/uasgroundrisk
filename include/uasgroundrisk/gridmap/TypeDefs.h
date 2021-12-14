#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <vector>
#include <Eigen/Dense>

namespace ugr
{
	namespace gridmap
	{
		// The Eigen Matrix type to use across all gridmap operations
		typedef Eigen::MatrixXf Matrix;
		// The Scalar data type of the Matrix
		typedef Matrix::Scalar GridMapDataType;

		// A Geospatial position in world coordinates in lon lat or xy order
		typedef Eigen::Vector2d Position;
		typedef Eigen::Vector2d Vector;

		// A 3D geospatial position in world coordinates in lon lat alt or xyz order
		typedef Eigen::Vector3d Position3;
		typedef Eigen::Vector3d Vector3;

		// An index directly into gridmap matrices in local indices in xy order
		typedef Eigen::Array2i Index;

		// The xy size
		typedef Eigen::Array2i Size;
		typedef Eigen::Array2d Length;
		typedef uint64_t Time;

		// A polygon in local coordinates directly into gridmap matrices
		typedef std::vector<Index> Polygon;
		// A polygon in world coordinates with points in lon lat or xy order
		typedef std::vector<Position> GeoPolygon;
	}
}
#endif // TYPEDEFS_H
