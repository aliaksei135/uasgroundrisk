#include "GridMap.h"

using namespace ugr::gridmap;


Vector2i GridMap::getSize() const
{
	return {sizeX, sizeY};
}

void GridMap::setGeometry(const int sizeX, const int sizeY)
{
	this->sizeX = sizeX;
	this->sizeY = sizeY;
	geometrySet = true;
}

std::vector<std::string> GridMap::getLayers() const
{
	std::vector<std::string> keys;
	keys.reserve(layers.size());
	for (const auto& pair : layers)
	{
		keys.emplace_back(pair.first);
	}
	return keys;
}

void GridMap::add(const std::string& layerName, const Matrix& layerData)
{
	if (geometrySet)
	{
		layers.insert({layerName, layerData});
	}
	else
	{
		throw std::out_of_range("GridMap Geometry not set");
	}
}

void GridMap::add(const std::string& layerName, const double constValue)
{
	if (geometrySet)
	{
		Matrix layerData(sizeX, sizeY);
		layers.emplace(layerName, layerData);
	}
	else
	{
		throw std::out_of_range("GridMap Geometry not set");
	}
}

::Matrix GridMap::get(const std::string& layerName) const
{
	return layers.at(layerName);
}

::Matrix& GridMap::get(const std::string& layerName)
{
	return layers.at(layerName);
}

const ::Matrix& GridMap::operator[](const std::string& layerName) const
{
	return layers.at(layerName);
}

::Matrix& GridMap::operator[](const std::string& layerName)
{
	return layers.at(layerName);
}

GridMapDataType GridMap::at(const std::string& layerName, const int i, const int j) const
{
	return layers.at(layerName)(i, j);
}

GridMapDataType& GridMap::at(const std::string& layerName, const int i, const int j)
{
	return layers.at(layerName)(i, j);
}

GridMapDataType GridMap::at(const std::string& layerName, const Index& idx) const
{
	return layers.at(layerName)(idx(0), idx(1));
}

GridMapDataType& GridMap::at(const std::string& layerName, const Index& idx)
{
	return layers.at(layerName)(idx(0), idx(1));
}

bool GridMap::isInBounds(const Index& localCoord) const
{
	if (geometrySet)
	{
		return localCoord(0) >= 0 && localCoord(1) >= 0 && localCoord(0) < sizeX && localCoord(1) < sizeY;
	}
	throw std::out_of_range("GridMap Geometry not set");
}

void GridMap::writeToNetCDF(const std::string& path) const
{
}
