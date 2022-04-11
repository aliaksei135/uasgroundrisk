#include "Ingest.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <cstdio>
#include <shapefil.h>
#include <geos_c.h>
#include <proj.h>
#include <iostream>
#include <csv.h>
#include <fstream>

#include "../../utils/DefaultGEOSMessageHandlers.h"


const std::vector<std::vector<int>> CensusNHAPSIngest::NHAPS_GROUPING = {
	{0, 1},
	{5, 9},
	{7},
	{6, 8}
};

const std::vector<std::vector<ugr::mapping::osm::OSMTag>> CensusNHAPSIngest::NHAPS_OSM_MAPPING = {
	{{"landuse", "residential"}},
	{{"landuse", "industrial"}, {"landuse", "commercial"}, {"building", "office"}},
	{
		{"building", "school"}, {"building", "college"}, {"building", "university"}, {"building", "public"},
		{"building", "government"}, {"building", "civic"}, {"building", "hospital"}, {"landuse", "education"},
		{"amenity", "courthouse"}, {"amenity", "townhall"}, {"amenity", "police"}, {"amenity", "fire_station"},
		{"amenity", "clinic"}, {"amenity", "courthouse"}, {"building", "transportation"}, {"landuse", "religious"}
	},
	{{"landuse", "retail"}, {"building", "supermarket"}, {"building", "retail"}}
};


std::map<std::string, GEOSGeometry*> CensusGeometryIngest::readFile(const std::string& file)
{
	std::map<std::string, GEOSGeometry*> outMap;

	// Remove the extension and assume the .shp, .shx and .dbf files all share a common path and name
	const auto extIdx = file.find_last_of('.');
	const auto basePath = file.substr(0, extIdx);

	const std::ifstream f(basePath.c_str());
	if (!f.good())
	{
		throw std::ios_base::failure("Cannot locate census geometry .shp file at: " + basePath);
	}

	// Open vertex (shp), index (shx) and attribute (dbf) files and keep the handles
	const auto shpHandle = SHPOpen(basePath.c_str(), "rb");
	const auto dbfHandle = DBFOpen(basePath.c_str(), "rb");

	// Get the number of shapes we are extracting
	int nEntities;
	SHPGetInfo(shpHandle, &nEntities, nullptr, nullptr, nullptr);

	for (int i = 0; i < nEntities; ++i)
	{
		auto* obj = SHPReadObject(shpHandle, i); //alloc
		if (obj == nullptr)
		{
			std::cerr << "SHP file header gave wrong number of entities";
			break;
		}

		GEOSGeometry* outerRing;
		std::vector<GEOSGeometry*> innerRings;

		for (int j = 0; j < obj->nParts; ++j)
		{
			const unsigned int nVert = obj->nVertices;

			// Allocate a GEOS coord sequence
			// ugr is only 2D and so is the grid map, so only 2 dimensions used, even if data has more
			auto* objGeosCoordSeq = GEOSCoordSeq_create_r(geosCtx, nVert, 2);

			const auto startVertIdx = obj->panPartStart[j];
			const double startX = obj->padfX[startVertIdx], startY = obj->padfY[startVertIdx];

			GEOSCoordSeq_setOrdinate_r(geosCtx, objGeosCoordSeq, 0, 0, startX);
			GEOSCoordSeq_setOrdinate_r(geosCtx, objGeosCoordSeq, 0, 1, startY);

			for (unsigned k = startVertIdx + 1; k < nVert; ++k)
			{
				GEOSCoordSeq_setOrdinate_r(geosCtx, objGeosCoordSeq, k, 0, obj->padfX[k]);
				GEOSCoordSeq_setOrdinate_r(geosCtx, objGeosCoordSeq, k, 1, obj->padfY[k]);
			}

			// Ensure a closed ring is formed
			double endX, endY;
			GEOSCoordSeq_getOrdinate_r(geosCtx, objGeosCoordSeq, nVert - 1, 0, &endX);
			GEOSCoordSeq_getOrdinate_r(geosCtx, objGeosCoordSeq, nVert - 1, 1, &endY);
			if (startX != endX || startY != endY)
			{
				GEOSCoordSeq_setOrdinate_r(geosCtx, objGeosCoordSeq, nVert - 1, 0, startX);
				GEOSCoordSeq_setOrdinate_r(geosCtx, objGeosCoordSeq, nVert - 1, 1, startY);
			}


			switch (obj->panPartType[j])
			{
			case SHPP_FIRSTRING:
			case SHPP_OUTERRING:
			case SHPP_RING:
				outerRing = GEOSGeom_createLinearRing_r(geosCtx, objGeosCoordSeq);
				break;
			case SHPP_INNERRING:
				innerRings.emplace_back(GEOSGeom_createLinearRing_r(geosCtx, objGeosCoordSeq));
				break;
			default:
				break;
			}
		}

		GEOSGeometry* geom;
		switch (obj->nSHPType)
		{
		case SHPT_POINT:
		case SHPT_POINTZ:
		case SHPT_POINTM:
			{
				auto* coordSeq = GEOSGeom_getCoordSeq_r(geosCtx, outerRing);
				double x, y;
				GEOSCoordSeq_getXY_r(geosCtx, coordSeq, 0, &x, &y);
				geom = GEOSGeom_createPointFromXY_r(geosCtx, x, y);
			}
			break;
		case SHPT_POLYGON:
		case SHPT_POLYGONZ:
		case SHPT_POLYGONM:
			{
				geom = GEOSGeom_createPolygon_r(geosCtx, outerRing, innerRings.data(), innerRings.size());
				auto v = GEOSisValid_r(geosCtx, geom);
				if (v != 1)
				{
					auto vr = GEOSisValidReason_r(geosCtx, geom);
					auto t = GEOSGeomType_r(geosCtx, geom);
					SHPDestroyObject(obj);
					continue;
				}
			}
			break;
		default:
			break;
		}
		auto* code = DBFReadStringAttribute(dbfHandle, i, 0);
		if (geom != nullptr)
			outMap.emplace(code, geom);

		SHPDestroyObject(obj); //dealloc
	}
	return outMap;
}

std::map<std::string, double> CensusDensityIngest::readFile(const std::string& file)
{
	const std::ifstream f(file.c_str());
	if (!f.good())
	{
		throw std::ios_base::failure("Cannot locate census density .csv file at: " + file);
	}

	std::map<std::string, double> outMap;
	using CsvParser = io::CSVReader<4, io::trim_chars<' ', '\t'>, io::no_quote_escape<','>, io::ignore_overflow,
	                                io::no_comment>;

	CsvParser reader(file);
	reader.read_header(io::ignore_extra_column, "code", "population", "area", "density");
	std::string code;
	int population;
	double area, density;
	while (reader.read_row(code, population, area, density))
	{
		// Scale density from per hectare to /km^2
		outMap.emplace(code, density / 0.01);
	}
	return outMap;
}

std::vector<std::vector<float>> CensusNHAPSIngest::readFile(const std::string& file)
{
	const std::ifstream fs(file.c_str());
	if (!fs.good())
	{
		throw std::ios_base::failure("Cannot locate NHAPS .json file at: " + file);
	}

#ifdef _WIN32
	const auto mode = "rb";
#else
        const auto mode = "r";
#endif
	FILE* f = fopen(file.c_str(), mode);

	char readBuffer[16384];
	rapidjson::FileReadStream is(f, readBuffer, sizeof(readBuffer));
	rapidjson::Document doc;
	doc.ParseStream(is);
	std::vector<std::vector<float>> nhapsProportions;
	for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin();
	     itr != doc.MemberEnd(); ++itr)
	{
		std::vector<float> timeProps;
		for (rapidjson::Value::ConstMemberIterator subitr = itr->value.MemberBegin();
		     subitr != itr->value.MemberEnd(); ++subitr)
		{
			timeProps.emplace_back(subitr->value.GetFloat());
		}
		std::vector<float> groupedTimeProps;
		groupedTimeProps.reserve(NHAPS_GROUPING.size());
		for (const auto& indices : NHAPS_GROUPING)
		{
			float groupSum = 0;
			for (const auto index : indices)
			{
				groupSum += timeProps[index];
			}
			groupedTimeProps.emplace_back(groupSum);
		}

		nhapsProportions.emplace_back(groupedTimeProps);
	}
	fclose(f);
	return nhapsProportions;
}

std::vector<std::vector<float>> CensusIngest::makeNHAPSProportions()
{
	CensusNHAPSIngest nhapsIngest;
	auto nhapsProps = nhapsIngest.readFile(UGR_DATA_DIR "/nhaps.json");
	return nhapsProps;
}
