/*
 * OSMOverpassQueryBuilder.h
 *
 *  Created by A.Pilko on 24/03/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_OSMOVERPASSQUERYBUILDER_H_
#define UASGROUNDRISK_SRC_MAP_GEN_OSMOVERPASSQUERYBUILDER_H_

#include "OSMOverpassQuery.h"
#include "OSMTag.h"

namespace ugr
{
	namespace mapping
	{
		namespace osm
		{
			class OSMOverpassQueryBuilder
			{
				OSMOverpassQuery query;

			public:
				OSMOverpassQueryBuilder(const Coordinates& southWestCoord,
				                        const Coordinates& northEastCoord)
					: query(southWestCoord, northEastCoord)
				{
				}

				explicit operator OSMOverpassQuery() const { return query; }

				OSMOverpassQuery build() const { return query; }

				OSMOverpassQueryBuilder& withNodeTag(const OSMTag& tag);
				OSMOverpassQueryBuilder& withNodeTag(const std::string& key,
				                                     const std::string& value);
				OSMOverpassQueryBuilder& withWayTag(const OSMTag& tag);
				OSMOverpassQueryBuilder& withWayTag(const std::string& key,
				                                    const std::string& value);
				OSMOverpassQueryBuilder& withRelationTag(const OSMTag& tag);
				OSMOverpassQueryBuilder& withRelationTag(const std::string& key,
				                                         const std::string& value);
				OSMOverpassQueryBuilder& withCombinedTag(const OSMTag& tag);
				OSMOverpassQueryBuilder& withCombinedTag(const std::string& key,
				                                         const std::string& value);

				OSMOverpassQueryBuilder& withTimeout(short int timeout);
			};
		}
	}
}
#endif // UASGROUNDRISK_SRC_MAP_GEN_OSMOVERPASSQUERYBUILDER_H_
