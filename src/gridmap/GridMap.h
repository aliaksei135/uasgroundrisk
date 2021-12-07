#ifndef GRIDMAP_H
#define GRIDMAP_H
#include <unordered_map>
#include <Eigen/Dense>
#include "TypeDefs.h"


namespace ugr
{
	namespace gridmap
	{
		using namespace Eigen;

		/**
		 * @brief A container for multiple labelled matrices of the same size
		 *
		 * Loosely based on grid_map: https://github.com/ANYbotics/grid_map
		 * But the catkin build system is atrocious when not already using ROS...
		*/
		class GridMap
		{
		public:
			EIGEN_MAKE_ALIGNED_OPERATOR_NEW

			GridMap() = default;

			GridMap(const GridMap& other) = default;

			GridMap(GridMap&& other) noexcept
				: geometrySet(other.geometrySet),
				  sizeX(other.sizeX),
				  sizeY(other.sizeY),
				  layers(std::move(other.layers))
			{
			}

			GridMap& operator=(const GridMap& other)
			{
				if (this == &other)
					return *this;
				geometrySet = other.geometrySet;
				sizeX = other.sizeX;
				sizeY = other.sizeY;
				layers = other.layers;
				return *this;
			}

			GridMap& operator=(GridMap&& other) noexcept
			{
				if (this == &other)
					return *this;
				geometrySet = other.geometrySet;
				sizeX = other.sizeX;
				sizeY = other.sizeY;
				layers = std::move(other.layers);
				return *this;
			}

			~GridMap() = default;

			/**
			 * @brief Set the geometry of the gridmap
			 * @param sizeX the x size
			 * @param sizeY the y size 
			*/
			void setGeometry(const int sizeX, const int sizeY);


			/**
			 * @brief Return a vector of layer names
			 * @return vector of layer names
			*/
			std::vector<std::string> getLayers() const;


			/**
			 * @brief Add a layer to the gridmap and set it to the provided matrix.
			 * The provided matrix must match the size of the gridmap.
			 * @param layerName name of the layer to add
			 * @param layerData matrix of value, matching size of gridmap
			*/
			void add(const std::string& layerName, const Matrix& layerData);


			/**
			 * @brief Add a layer to the gridmap and fill it with a constant value
			 * @param layerName name of layer to add
			 * @param constValue value to fill the layer
			*/
			void add(const std::string& layerName, const double constValue);


			/**
			 * @brief Return the data for a layer
			 * @param layerName name of layer to get
			 * @return the matrix of layer values
			*/
			Matrix get(const std::string& layerName) const;
			Matrix& get(const std::string& layerName);
			const Matrix& operator[](const std::string& layerName) const;
			Matrix& operator[](const std::string& layerName);

			/**
			 * @brief Return the value of the coefficient at x,y on the given layer
			 * @param layerName layer to use
			 * @param i x index
			 * @param j y index
			 * @return the coefficient value
			*/
			GridMapDataType at(const std::string& layerName, int i, int j) const;
			GridMapDataType& at(const std::string& layerName, int i, int j);
			GridMapDataType at(const std::string& layerName, const Index& idx) const;
			GridMapDataType& at(const std::string& layerName, const Index& idx);

			Vector2i getSize() const;


			/**
			* @brief Test if the given local coord is within bounds
			* @param localCoord the coord to test
			* @return whether the coord is in bounds
			*/
			bool isInBounds(const Index& localCoord) const;


			void writeToNetCDF(const std::string& path) const;

		protected:
			bool geometrySet = false;
			unsigned int sizeX, sizeY;

			std::unordered_map<std::string, Matrix> layers;
		};
	}
}
#endif // GRIDMAP_H
