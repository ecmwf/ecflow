/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VSettingsLoader.hpp"

#include <vector>

using Vec = std::vector<VSettingsLoader*>;

static Vec* makers = nullptr;

VSettingsLoader::VSettingsLoader() {
    if (makers == nullptr) {
        makers = new Vec();
    }

    makers->push_back(Vec::value_type(this));
}

void VSettingsLoader::process() {
    if (!makers) {
        return;
    }

    for (auto& maker : *makers) {
        maker->loadSettings();
    }
}
