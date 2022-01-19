/*
 * OSMOverpassQueryBuilder.cpp
 *
 *  Created by A.Pilko on 24/03/2021.
 */


#include "uasgroundrisk/map_gen/osm/OSMOverpassQueryBuilder.h"
#include "uasgroundrisk/map_gen/osm/OSMTag.h"

using namespace ugr::mapping::osm;

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withNodeTag(const OSMTag& tag)
{
	query.nodeTags.emplace_back(tag);
	return *this;
}

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withWayTag(const OSMTag& tag)
{
	query.wayTags.emplace_back(tag);
	return *this;
}

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withRelationTag(const OSMTag& tag)
{
	query.relationTags.emplace_back(tag);
	return *this;
}

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withCombinedTag(const OSMTag& tag)
{
	query.nodeTags.emplace_back(tag);
	query.wayTags.emplace_back(tag);
	query.relationTags.emplace_back(tag);
	return *this;
}

OSMOverpassQueryBuilder& OSMOverpassQueryBuilder::withTimeout(short timeout)
{
	query.timeout = timeout;
	return *this;
}

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withNodeTag(const std::string& key,
                                     const std::string& value)
{
	return withNodeTag(OSMTag(key, value));
}

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withWayTag(const std::string& key,
                                    const std::string& value)
{
	return withWayTag(OSMTag(key, value));
}

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withRelationTag(const std::string& key,
                                         const std::string& value)
{
	return withRelationTag(OSMTag(key, value));
}

OSMOverpassQueryBuilder&
OSMOverpassQueryBuilder::withCombinedTag(const std::string& key,
                                         const std::string& value)
{
	return withCombinedTag(OSMTag(key, value));
}
