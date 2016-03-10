/*
 * cgtpcoredb.cpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#include "cgtpcoredb.hpp"
#include "roflibs/gtpcore/cgtpcoredb_file.hpp"

using namespace roflibs::gtp;

/*static*/ std::map<std::string, cgtpcoredb *> cgtpcoredb::gtpcoredbs;

/*static*/
cgtpcoredb &cgtpcoredb::get_gtpcoredb(const std::string &name) {
  if (cgtpcoredb::gtpcoredbs.find(name) == cgtpcoredb::gtpcoredbs.end()) {
    if (name == std::string("file")) {
      cgtpcoredb::gtpcoredbs[name] = new cgtpcoredb_file();
    } else {
      throw eGtpDBNotFound("cgtpcoredb::get_gtpcoredb()");
    }
  }
  return *(cgtpcoredb::gtpcoredbs[name]);
}
