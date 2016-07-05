//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AbstractTextEditSearchInterface.hpp"

#include "VConfig.hpp"
#include "VProperty.hpp"

QColor AbstractTextEditSearchInterface::highlightColour_=QColor(200, 255, 200);

AbstractTextEditSearchInterface::AbstractTextEditSearchInterface()
{
	if(VProperty *p=VConfig::instance()->find("panel.search.highlightColour"))
	{
		highlightColour_=p->value().value<QColor>();
	}

	vpPerformAutomaticSearch_ = VConfig::instance()->find("panel.automaticOutputSearch.performSearch");
	vpAutomaticSearchMode_    = VConfig::instance()->find("panel.automaticOutputSearch.searchMode");
	vpAutomaticSearchText_    = VConfig::instance()->find("panel.automaticOutputSearch.searchText");
	vpAutomaticSearchFrom_    = VConfig::instance()->find("panel.automaticOutputSearch.searchFrom");
	vpAutomaticSearchCase_    = VConfig::instance()->find("panel.automaticOutputSearch.caseSensitive");

	// these should always exist - if they don't then there is a spelling error in the above lines
	assert(vpPerformAutomaticSearch_);
	assert(vpAutomaticSearchMode_);
	assert(vpAutomaticSearchText_);
	assert(vpAutomaticSearchFrom_);
	assert(vpAutomaticSearchCase_);
}

