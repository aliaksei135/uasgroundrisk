/*
 * OverpassExceptions.h
 *
 *  Created by A.Pilko on 28/03/2021.
 */

#ifndef UASGROUNDRISK_SRC_MAP_GEN_OSM_OVERPASSEXCEPTIONS_H_
#define UASGROUNDRISK_SRC_MAP_GEN_OSM_OVERPASSEXCEPTIONS_H_

#include <exception>

class overpass_network_exception : public std::exception {
public:
  const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
    return "The Overpass API endpoint is unreachable or has timed out";
  }
};

#endif // UASGROUNDRISK_SRC_MAP_GEN_OSM_OVERPASSEXCEPTIONS_H_
