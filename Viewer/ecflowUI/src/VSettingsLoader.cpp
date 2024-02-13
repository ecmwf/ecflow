/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VSettingsLoader.hpp"

#include <vector>

using Vec = std::vector<VSettingsLoader*>;

static Vec* makers = nullptr;

VSettingsLoader::VSettingsLoader() {
    if (makers == nullptr)
        makers = new Vec();

    makers->push_back(Vec::value_type(this));
}

void VSettingsLoader::process() {
    if (!makers)
        return;

    for (auto& maker : *makers) {
        maker->loadSettings();
    }
}
