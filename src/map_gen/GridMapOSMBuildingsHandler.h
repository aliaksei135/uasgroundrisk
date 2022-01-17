#ifndef GRIDMAPOSMBUILDINGSHANDLER_H
#define GRIDMAPOSMBUILDINGSHANDLER_H
#include <string>
#include <osmium/handler.hpp>

namespace ugr
{
	namespace mapping
	{
		class GeospatialGridMap;
	}
}

class GridMapOSMBuildingsHandler : public osmium::handler::Handler
{
public:
	GridMapOSMBuildingsHandler(ugr::mapping::GeospatialGridMap* gridMap, float levelHeight = 3.048f, std::string gridCRS = "EPSG:3395");
	~GridMapOSMBuildingsHandler() = default;
	void way(const osmium::Way& way) const noexcept;
protected:
	ugr::mapping::GeospatialGridMap* gridMap;
	float buildingLevelHeight;

	std::string gridCRS;
};
#endif // GRIDMAPOSMBUILDINGSHANDLER_H
