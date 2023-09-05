/*
 * OSMOverpassQuery.cpp
 *
 *  Created by A.Pilko on 24/03/2021.
 */

#include <cpr/cpr.h>
#include <cstdio>
#include <iostream>
#include <stack>

#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/xml_input.hpp>

#ifdef UGR_IO_COMPRESSION_BZ2
#include <osmium/io/any_compression.hpp>
#endif

#include "OverpassExceptions.h"
#include "uasgroundrisk/map_gen/osm/OSMOverpassQueryBuilder.h"

using namespace osmium::io;
using namespace ugr::mapping::osm;

OSMOverpassQueryBuilder
OSMOverpassQuery::create(const Coordinates& southWestCoord,
                         const Coordinates& northEastCoord)
{
    return OSMOverpassQueryBuilder{southWestCoord, northEastCoord};
}

std::string OSMOverpassQuery::rawResponse(const short int maxRetries) const
{
    cpr::Url url = getOverpassEndpoint();
    cpr::Body params = cpr::Body{buildQueryString()};
    cpr::Response response = cpr::Post(url, params, cpr::VerifySsl(false));

    if (response.status_code != 200) {
        if (maxRetries > 0) {
            std::cout << "Overpass query failed with status code " << response.status_code << ". Retrying..."
                      << std::endl;
            return rawResponse(maxRetries - 1);
        } else {
            std::cerr << "Overpass query terminally failed with status code " << std::to_string(response.status_code)
                      << " for query: " << params.str() << std::endl;
        }
    }

    std::string response_txt = response.text;
    std::ofstream out;
    out.open(responseFilepath);
    out << response_txt;
    out.close();
    return std::move(response_txt);
}

cpr::Url OSMOverpassQuery::getOverpassEndpoint() const
{
    // Get a random Overpass endpoint from OVERPASS_ENDPOINTS
    return OVERPASS_ENDPOINTS[std::rand() % OVERPASS_ENDPOINTS.size()];
}

std::string OSMOverpassQuery::buildQueryString(const bool xmlQuery) const
{
    if (xmlQuery)
        return buildQueryStringXML();
    return buildQueryStringQL();
}

osmium::memory::Buffer OSMOverpassQuery::rawBuffer() const
{
    rawResponse();
    Reader reader{responseFilepath, osmium::osm_entity_bits::all};
    osmium::memory::Buffer buffer = reader.read();
    buffer.commit();
    reader.close();
    return buffer;
}

std::string OSMOverpassQuery::buildQueryStringQL() const
{
    std::string queryString;
    // Start with output format params
    queryString += "[out:" + outputFormat + "]";
    // Timeout next
    queryString += "[timeout:" + std::to_string(timeout) + "]";
    // Set bounding box in s,w,n,e order
    queryString += "[bbox: " + std::to_string(southWestCoord.y) + "," +
        std::to_string(southWestCoord.x) + "," +
        std::to_string(northEastCoord.y) + "," +
        std::to_string(northEastCoord.x) + "];";
    // Wrap tag queries in bbox to limit
    queryString += "(";
    for (const auto& tag : nodeTags)
    {
        queryString += "node[" + tag.to_string() + "];";
    }
    for (const auto& tag : wayTags)
    {
        queryString += "way[" + tag.to_string() + "];";
    }
    for (const auto& tag : relationTags)
    {
        queryString += "rel[" + tag.to_string() + "];";
    }
    queryString += "way(r); node(w);"; // Recurse relation-way and way-node
    queryString += ");";

    // Output and recurse through objects
    // Output using quad tiles (qt) for faster response
    queryString += "out body; >; out qt;";

    return queryString;
}

std::string OSMOverpassQuery::buildQueryStringXML() const
{
    std::ostringstream qss;
    // LIFO co
    std::stack<std::string> closingTags;

    const std::string bboxQuery = "<bbox-query s=\"" +
        std::to_string(southWestCoord.y) + "\" n=\"" +
        std::to_string(northEastCoord.y) + "\" w=\"" +
        std::to_string(southWestCoord.x) + "\" e=\"" +
        std::to_string(northEastCoord.x) + "\"/>";

    // Add preamble
    // Output as XML for osmium to be able to read in
    qss << "<osm-script output=\"xml\">";
    closingTags.emplace("</osm-script>");

    // Print output
    closingTags.emplace("<print/>");

    // TODO Allow better designed clauses
    qss << "<union>";
    closingTags.emplace("</union>");

    if (!nodeTags.empty())
    {
        for (const auto& tag : nodeTags)
        {
            qss << "<query type=\"node\">";

            qss << "<has-kv k=\"" << tag.key << "\" ";
            if (!tag.value.empty())
                qss << "v=\"" + tag.value + "\"";
            qss << "/>";

            qss << bboxQuery;
            qss << "</query>";
        }
    }
    if (!wayTags.empty())
    {
        for (const auto& tag : wayTags)
        {
            qss << "<query type=\"way\">";

            qss << "<has-kv k=\"" << tag.key << "\" ";
            if (!tag.value.empty())
                qss << "v=\"" + tag.value + "\"";
            qss << "/>";

            qss << bboxQuery;
            qss << "</query>";
            qss << "<recurse type=\"down\" />";
        }
    }
    if (!relationTags.empty())
    {
        for (const auto& tag : relationTags)
        {
            qss << "<query type=\"relation\">";
            qss << "<has-kv k=\"" << tag.key << "\" ";
            if (!tag.value.empty())
                qss << "v=\"" + tag.value + "\"";
            qss << "/>";

            qss << bboxQuery;
            qss << "</query>";
            qss << "<recurse type=\"down\" />";
        }
    }

    // Close all dangling tags in LIFO order
    while (!closingTags.empty())
    {
        std::string ct = closingTags.top();
        closingTags.pop();
        qss << ct;
    }
    return qss.str();
}
