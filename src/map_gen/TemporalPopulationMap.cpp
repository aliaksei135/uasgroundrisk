#include "uasgroundrisk/map_gen/TemporalPopulationMap.h"
#include "uasgroundrisk/map_gen/osm/handlers/OSMTagGeometryHandler.h"
#include "../src/utils/GeometryOperations.h"
#include "../src/utils/GeometryProjectionUtils.h"
#include "../src/map_gen/census/Ingest.h"
#include "uasgroundrisk/gridmap/Iterators.h"

ugr::mapping::TemporalPopulationMap::TemporalPopulationMap(const std::array<float, 4>& bounds, const int resolution,
	const short defaultHour) :
	PopulationMap(bounds, resolution), hourOfDay(defaultHour), geosCtx(initGEOS_r(notice, log_and_exit))
{
	// We try to precompute/fetch as many of the steps and data dependencies that are invariant for the life of the object.
	// In practice, this means that as long as the bounds and resolution stay the same, this data will stay the same.

	// Get census data
	CensusIngest censusIngest(geosCtx);
	// TODO: use an STR tree (impl in GEOS) for faster search?
	auto geomDensityMap = censusIngest.makePopulationDensityMap<GridMapDataType>();
	auto boundedGeomDensityMap = util::boundGeometriesMap_r(geomDensityMap, bounds, geosCtx);
	popDensityGeomMap.swap(boundedGeomDensityMap);
	for (const auto& pair : popDensityGeomMap)
	{
		boundedGeometries.emplace_back(pair.first);
	}
	const auto censusAreas = calculateAreas(boundedGeometries);
	for (int i = 0; i < censusAreas.size(); ++i)
	{
		geomAreas.emplace(boundedGeometries[i], censusAreas[i]);
	}
	totalPopulation = std::accumulate(popDensityGeomMap.begin(), popDensityGeomMap.end(),
		static_cast<decltype(totalPopulation)>(0),
		[this](const decltype(totalPopulation) acc,
			const std::pair<GEOSGeometry*, GridMapDataType> p)
		{
			if (geomAreas.count(p.first) == 0)
			{
				return acc;
			}
			// Get the area of the geometry
			const auto area = geomAreas.at(p.first);

			//Work out absolute population from density and area then accumulate
			return static_cast<decltype(totalPopulation)>(acc + (area * p.
				second));
		}
	);

	// Get the NHAPS proportions
	nhapsProps = censusIngest.makeNHAPSProportions();

	// Get the relevant tagged geometries from OSM
	// Calculate area sum for each tag, so we can work out density down the line
	setOSMGeometries();
	for (const auto& pair : tagGeomMap)
	{
		const auto areas = calculateAreas(pair.second);
		const auto areaSum = std::accumulate(areas.begin(), areas.end(),
			static_cast<decltype(areas)::value_type>(0));
		tagAreas.emplace(pair.first, areaSum);
	}
	setHourOfDay(defaultHour);
}

ugr::mapping::TemporalPopulationMap::~TemporalPopulationMap()
{
	util::destroyGEOSGeoms(boundedGeometries);
	finishGEOS_r(geosCtx);
}

void ugr::mapping::TemporalPopulationMap::setHourOfDay(const short hourOfDay)
{
	if (hourOfDay < 0 || hourOfDay > 23)
	{
		throw std::out_of_range("Hour of Day must be 0<=h<=23");
	}
	this->hourOfDay = hourOfDay;
	isEvaluated = false;

	densityTagMap.clear();
	activeGeomDensityMap.clear();

	for (int i = 0; i < CensusNHAPSIngest::NHAPS_OSM_MAPPING.size(); ++i)
	{
		// Get the OSM tags mapped to this NHAPS category
		const auto groupOSMTags = CensusNHAPSIngest::NHAPS_OSM_MAPPING[i];
		// Lookup the proportion of this NHAPS category for this time
		const auto groupProportion = nhapsProps[hourOfDay][i];
		// Work out the absolute population we have assigned to this NHAPS category
		const auto groupPopulation = totalPopulation * groupProportion;
		// Residential population is special case as we already have densities for the wards
		// If we did this the same as other categories we would discard this density info and have uniform residential densities
		if (i == 0)
		{
			for (auto* geom : boundedGeometries)
			{
				const auto fullPopDensity = popDensityGeomMap.at(geom);
				const auto scaledPopDensity = groupProportion * fullPopDensity;
				activeGeomDensityMap.emplace(geom, scaledPopDensity);
			}
		}
		else
		{
			// Get the sum of all group geometry areas
			// Result in m^2
			const auto groupArea = std::accumulate(groupOSMTags.begin(), groupOSMTags.end(), 0.0,
				[this](const double& acc, const osm::OSMTag& tag)
				{
					if (tagAreas.count(tag) == 0) return acc;
					return acc + tagAreas.at(tag);
				});

			const auto groupDensity = groupPopulation / groupArea;
			for (const auto& tag : groupOSMTags)
			{
				densityTagMap.emplace(tag, groupDensity);
			}
		}
	}
}

std::vector<double> ugr::mapping::TemporalPopulationMap::calculateAreas(const std::vector<GEOSGeometry*>& geoms) const
{
	const auto projObjs = util::makeProjObject("EPSG:4326", "EPSG:3395");
	PJ* reproj = std::get<0>(projObjs);
	PJ_CONTEXT* projCtx = std::get<1>(projObjs);
	std::vector<double> areas;

	for (const auto* geom : geoms)
	{
		double area = 0;
		const auto nGeom = GEOSGetNumGeometries_r(geosCtx, geom);
		for (int i = 0; i < nGeom; ++i)
		{
			auto* g = GEOSGetGeometryN_r(geosCtx, geom, i);

			auto* reprojGeom = util::reprojectPolygon_r(reproj, g, geosCtx);

			if (reprojGeom != nullptr) {
				// Convert m^2 to km^2
				area += util::getGeometryArea_r<double>(reprojGeom, geosCtx) / 1e6;
			}
		}
		areas.emplace_back(area);
	}
	proj_context_destroy(projCtx);
	return areas;
}

void ugr::mapping::TemporalPopulationMap::intersectResidentialGeometries()
{
	const auto& residentialGeoms = tagGeomMap[{"landuse", "residential"}];
	std::map<GEOSGeometry*, GridMapDataType> intersectedPopDensityGeomMap;
	std::vector<GEOSGeometry*> intersectedBoundedGeometries;
	intersectedBoundedGeometries.reserve(boundedGeometries.size());

	for (const auto geom : boundedGeometries)
	{
		const auto* prepGeom = GEOSPrepare_r(geosCtx, geom);
		for (const auto& resGeom : residentialGeoms)
		{
			if (GEOSPreparedIntersects_r(geosCtx, prepGeom, resGeom))
			{
				auto* intersectGeom = GEOSIntersection_r(geosCtx, geom, resGeom);
				if (intersectGeom == nullptr || (GEOSGeomTypeId_r(geosCtx, intersectGeom) != GEOS_MULTIPOLYGON &&
					GEOSGeomTypeId_r(geosCtx, intersectGeom) != GEOS_POLYGON)) continue;
				auto geomDensity = popDensityGeomMap.at(geom);
				intersectedPopDensityGeomMap.emplace(intersectGeom, geomDensity);
				intersectedBoundedGeometries.emplace_back(intersectGeom);
			}
		}
	}
	popDensityGeomMap.swap(intersectedPopDensityGeomMap);
	boundedGeometries.swap(intersectedBoundedGeometries);
	tagGeomMap[{"landuse", "residential"}] = boundedGeometries;
}

void ugr::mapping::TemporalPopulationMap::setOSMGeometries()
{
	std::vector<osm::OSMTag> tags;
	for (const auto& tagVec : CensusNHAPSIngest::NHAPS_OSM_MAPPING)
	{
		std::copy(tagVec.begin(), tagVec.end(), std::back_inserter(tags));
	}

	for (auto& tag : tags)
	{
		tagLayerMap.emplace(tag, tag.to_string());
		add(tag.to_string(), 0);
	}
	osm::OSMTagGeometryHandler handler(tags, tagGeomMap, geosCtx);

	OSMMap::eval(handler);

	intersectResidentialGeometries();
}

void ugr::mapping::TemporalPopulationMap::fillGridMapPoly(const std::string& layerName, const GEOSGeometry* geom,
	const GridMapDataType& geomDensity)
{
	const auto poly = util::asGeoPolygon_r(geom, geosCtx);
	if (poly.size() < 3) return;
	for (PolygonIterator iter(*this, poly); !iter.isPastEnd();
		++iter)
	{
		const auto gridMapPoint = (*iter);
		at(layerName, gridMapPoint) = geomDensity;
	}
}

void ugr::mapping::TemporalPopulationMap::eval()
{
	if (isEvaluated) return;
	for (auto& pair : tagGeomMap)
	{
		GridMapDataType fallbackDensity = -1;
		if (densityTagMap.count(pair.first) == 1)
			fallbackDensity = densityTagMap.at(pair.first);
		std::string layerName = tagLayerMap.at(pair.first);
		for (auto& geom : pair.second)
		{
			auto geomDensity = fallbackDensity;
			if (fallbackDensity < 0)
			{
				if (activeGeomDensityMap.count(geom) == 1)
				{
					geomDensity = activeGeomDensityMap.at(geom);
				}
				else
				{
					const auto* prepGeom = GEOSPrepare_r(geosCtx, geom);
					// Iterate through population geometries to find the which one this
					// point is within
					for (const auto& populationGeomPair : activeGeomDensityMap)
					{
						// const auto gp = util::asGeoPolygon_r(populationGeomPair.first, geosCtx);
						if (GEOSPreparedIntersects_r(geosCtx, prepGeom, populationGeomPair.first))
						{
							geom = GEOSIntersection_r(geosCtx, geom, populationGeomPair.first);
							geomDensity = populationGeomPair.second;
							break;
						}
					}
					if (geom == nullptr || !GEOSisValid_r(geosCtx, geom)) continue;
				}
			}
			// const auto t = GEOSGeomTypeId_r(geosCtx, geom);
			// if (t != GEOS_POLYGON)
			// {
			const auto nGeom = GEOSGetNumGeometries_r(geosCtx, geom);
			for (int i = 0; i < nGeom; ++i)
			{
				const auto* g = GEOSGetGeometryN_r(geosCtx, geom, i);
				if (g != nullptr && GEOSisValid_r(geosCtx, g)) {
					fillGridMapPoly(layerName, g, geomDensity);
				}
				// }
				// else
				// {
				//     fillGridMapPoly(layerName, geom, geomDensity);
				// }
			}
		}

		//Combine layers to pop density;
		add("Population Density", 0);
		for (const auto& layerName : getLayers())
		{
			// GridMap coordinate frame convention has the y axis increasing to the
			// left, which flips everything around the x (vertical) axis. Here we flip
			// it back.
			get("Population Density") =
				get("Population Density").cwiseMax(get(layerName));
		}
		isEvaluated = true;
	}
}
