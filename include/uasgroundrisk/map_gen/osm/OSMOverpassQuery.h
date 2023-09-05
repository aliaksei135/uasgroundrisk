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

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/area/multipolygon_manager_legacy.hpp>

#include "handlers/DefaultNodeLocationsForWaysHandler.h"

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
                    90; // set a timeout in seconds for the query to return a response

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

                /*
 * Perform Overpass query, applying handlers.
 * @param ...handlers zero or more osmium::handler::Handler instances to be
 * applied to result
 */
                template <typename... THandlers>
                void makeQuery(THandlers&&...handlers)
                {
                    osmium::area::AssemblerConfig assemblerConfig;
                    assemblerConfig.ignore_invalid_locations = true;
                    assemblerConfig.create_way_polygons = false; // These are handled as normal ways in the handler
                    assemblerConfig.create_empty_areas = true;
                    osmium::area::MultipolygonManager<osmium::area::Assembler> multipolygonManager{
                        assemblerConfig
                    };

                    osm::DefaultNodeLocationsForWaysHandler n2wHandler;
                    n2wHandler.ignore_errors();

                    rawResponse();
                    const osmium::io::File f{responseFilepath};

                    read_relations(f, multipolygonManager);

                    osmium::io::Reader reader{f, osmium::osm_entity_bits::all};
                    osmium::apply(reader, n2wHandler, multipolygonManager.handler(),
                                  std::forward<THandlers>(handlers)...);
                    osmium::apply(multipolygonManager.buffer(), std::forward<THandlers>(handlers)...);
                }

                ~OSMOverpassQuery() { std::remove(responseFilepath); }

            private:
                const char *responseFilepath = strcat(std::tmpnam(nullptr), ".xml");

                const std::vector<std::string> OVERPASS_ENDPOINTS{
                        "https://overpass.kumi.systems/api/interpreter",
                        "https://overpass.openstreetmap.ru/api/interpreter",
                        "https://maps.mail.ru/osm/tools/overpass/api/interpreter",
                        "https://overpass-api.de/api/interpreter"
                };

                /**
 * Return a cpr::Url containing a valid Overpass instance.
 * @return Overpass instance cpr::Url
 */
                cpr::Url getOverpassEndpoint() const;

                std::string buildQueryStringQL() const;
                std::string buildQueryStringXML() const;
            };
        }
    }
}
#endif // UASGROUNDRISK_SRC_MAP_GEN_OSMOVERPASSQUERY_H_
