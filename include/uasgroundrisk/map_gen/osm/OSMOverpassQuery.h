/*
 * OSMOverpassQuery.h
 *
 *  Created by A.Pilko on 24/03/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_OSMOVERPASSQUERY_H_
#define UASGROUNDRISK_SRC_MAP_GEN_OSMOVERPASSQUERY_H_

#include <ostream>
#include <cpr/cpr.h>
#include <osmium/geom/coordinates.hpp>
#include <osmium/handler.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/visitor.hpp>
#include "uasgroundrisk/map_gen/osm/OSMTag.h"

using namespace osmium::geom;

namespace ugr
{
	namespace mapping
	{
		namespace osm
		{
			class OSMOverpassQueryBuilder;

			class OSMOverpassQuery
			{
				Coordinates southWestCoord;
				Coordinates northEastCoord;

				std::string outputFormat = "xml"; // xml or json. libOsmium uses xml as input,
				// so this is fixed for the time being
				std::vector<OSMTag> nodeTags;
				std::vector<OSMTag> wayTags;
				std::vector<OSMTag> relationTags;

				short int timeout =
					30; // set a timeout in seconds for the query to return a response

				OSMOverpassQuery(const Coordinates& southWestCoord,
				                 const Coordinates& northEastCoord)
					: southWestCoord(southWestCoord), northEastCoord(northEastCoord)
				{
				}

			public:
				friend class OSMOverpassQueryBuilder;

				/**
		 * Create a OSMOverpassQueryBuilder object to construct a query.
		 *
		 * Caller is responsible for ensuring coordinates are valid.
		 *
		 * @param southWestCoord south west most coordinate of the bounding box
		 * @param northEastCoord north east most coordinate of the bounding box
		 * @return an OSMOverpassQueryBuilder object
		 */
				static OSMOverpassQueryBuilder create(const Coordinates& southWestCoord,
				                                      const Coordinates& northEastCoord);

				/**
		 * Execute a GET request to an Overpass instance
		 * @return XML response text
		 */
				std::string rawResponse(short int maxRetries = 3) const;

				/**
		 * Build an Overpass bbox query string from the current object state
		 * @return overpass query string
		 */
				std::string buildQueryString(bool xmlQuery = true) const;

				/**
		 * Perform Overpass query and parse into an osmium::memory::Buffer.
		 * Caller assumes ownership of the returned buffer.
		 * @return osmium::memory::Buffer of response object
		 */
				osmium::memory::Buffer rawBuffer() const;

				/**
		 * Perform Overpass query, applying handlers.
		 * @param ...handlers zero or more osmium::handler::Handler instances to be
		 * applied to result
		 */
				template <typename... THandlers>
				void makeQuery(THandlers&&...handlers)
				{
					rawResponse();
					osmium::io::Reader reader{responseFilepath, osmium::osm_entity_bits::all};
					osmium::apply(reader, std::forward<THandlers>(handlers)...);
				}

				~OSMOverpassQuery() { std::remove(responseFilepath); }

			private:
				const char* responseFilepath = strcat(std::tmpnam(nullptr), ".xml");

				/**
		 * Return a cpr::Url containing a valid Overpass instance.
		 * @return Overpass instance cpr::Url
		 */
				static cpr::Url getOverpassEndpoint();

				std::string buildQueryStringQL() const;
				std::string buildQueryStringXML() const;
			};
		}
	}
}
#endif // UASGROUNDRISK_SRC_MAP_GEN_OSMOVERPASSQUERY_H_
