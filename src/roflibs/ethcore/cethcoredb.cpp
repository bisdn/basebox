/*
 * cethcoredb.cpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#include "cethcoredb.hpp"
#include "roflibs/ethcore/cethcoredb_file.hpp"

using namespace roflibs::eth;

/*static*/ std::map<std::string, cethcoredb *> cethcoredb::portdbs;
/*static*/ const uint16_t cethcoredb::DEFAULT_PVID = 1;

/*static*/
cethcoredb &cethcoredb::get_ethcoredb(const std::string &name) {
  if (cethcoredb::portdbs.find(name) == cethcoredb::portdbs.end()) {
    if (name == std::string("file")) {
      cethcoredb::portdbs[name] = new cethcoredb_file();
    } else {
      throw ePortDBNotFound("cethcoredb::get_ethcoredb()");
    }
  }
  return *(cethcoredb::portdbs[name]);
}
