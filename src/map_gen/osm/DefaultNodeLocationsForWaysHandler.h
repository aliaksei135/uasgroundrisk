/*
 * DefaultNodeLocationsForWaysHandler.h
 *
 *  Created by A.Pilko on 22/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_OSM_DEFAULTNODELOCATIONSFORWAYSHANDLER_H_
#define UASGROUNDRISK_SRC_MAP_GEN_OSM_DEFAULTNODELOCATIONSFORWAYSHANDLER_H_

#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/visitor.hpp>

namespace map = osmium::index::map;
using index_type =
    map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

/**
 * A simple wrapper around NodeLocationsForWays handler (included with osmium)
 * that handles all storage setup. Uses a SparseMemArray for storage.
 */
class DefaultNodeLocationsForWaysHandler : public location_handler_type {
public:
  DefaultNodeLocationsForWaysHandler() : NodeLocationsForWays(getStorage()) {}

private:
  static index_type &getStorage() {
    static index_type storage;
    return storage;
  }
};
#endif // UASGROUNDRISK_SRC_MAP_GEN_OSM_DEFAULTNODELOCATIONSFORWAYSHANDLER_H_
