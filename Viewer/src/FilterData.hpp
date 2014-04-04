//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef FILTERDATA_HPP_
#define FILTERDATA_HPP_

#include <set>
#include <vector>

#include "FilterDataObserver.hpp"
#include "DState.hpp"

class FilterData
{
public:
	FilterData();
	const std::set<DState::State>& nodeState() const {return nodeState_;}
	void setNodeState(const std::set<DState::State>& ns);
	bool isNodeStateFiltered() const;
	void addObserver(FilterDataObserver*);
	void removeObserver(FilterDataObserver*);

private:
	void broadcastChange();

	std::set<DState::State> nodeState_;
	std::vector<FilterDataObserver*> observers_;
};

#endif
