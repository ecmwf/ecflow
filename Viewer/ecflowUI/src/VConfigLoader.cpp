/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VConfigLoader.hpp"

#include <map>
#include <memory>

using Map = std::multimap<std::string, VConfigLoader*>;

static std::unique_ptr<Map> makers = nullptr;

VConfigLoader::VConfigLoader(const std::string& name) {
    if (!makers) {
        makers = std::make_unique<Map>();
    }

    makers->insert(Map::value_type(name, this));
}

VConfigLoader::~VConfigLoader() {
    // Not called
}

bool VConfigLoader::process(const std::string& name, VProperty* prop) {
    Map::size_type entries = makers->count(name);
    auto it                = makers->find(name);

    bool retVal = false;
    for (Map::size_type cnt = 0; cnt != entries; ++cnt, ++it) {
        (*it).second->load(prop);
        retVal = true;
    }

    /* if(it != makers->end())
     {
         (*it).second->load(prop);
         return true;
     }*/
    return retVal;
}
