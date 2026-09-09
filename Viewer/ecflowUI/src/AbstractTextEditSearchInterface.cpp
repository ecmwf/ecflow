/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AbstractTextEditSearchInterface.hpp"

#include "VConfig.hpp"
#include "VProperty.hpp"

QColor AbstractTextEditSearchInterface::highlightColour_ = QColor(200, 255, 200);

AbstractTextEditSearchInterface::AbstractTextEditSearchInterface() {
    if (VProperty* p = VConfig::instance()->find("panel.search.highlightColour")) {
        highlightColour_ = p->value().value<QColor>();
    }

    vpPerformAutomaticSearch_ = VConfig::instance()->find("panel.output.automaticSearch.performSearch");
    vpAutomaticSearchMode_    = VConfig::instance()->find("panel.output.automaticSearch.searchMode");
    vpAutomaticSearchText_    = VConfig::instance()->find("panel.output.automaticSearch.searchText");
    vpAutomaticSearchFrom_    = VConfig::instance()->find("panel.output.automaticSearch.searchFrom");
    vpAutomaticSearchCase_    = VConfig::instance()->find("panel.output.automaticSearch.caseSensitive");

    // these should always exist - if they don't then there is a spelling error in the above lines
    assert(vpPerformAutomaticSearch_);
    assert(vpAutomaticSearchMode_);
    assert(vpAutomaticSearchText_);
    assert(vpAutomaticSearchFrom_);
    assert(vpAutomaticSearchCase_);
}
