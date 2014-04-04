//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VIEWFILTER_HPP_
#define VIEWFILTER_HPP_

#include <set>
#include <vector>

#include "ViewFilterObserver.hpp"
#include "DState.hpp"

class ViewFilter
{
public:
	ViewFilter();
	const std::set<DState::State>& nodeState() const {return nodeState_;}
	void setNodeState(const std::set<DState::State>& ns);
	bool isNodeStateFiltered() const;
	void addObserver(ViewFilterObserver*);
	void removeObserver(ViewFilterObserver*);

private:
	void broadcastChange();

	std::set<DState::State> nodeState_;
	std::vector<ViewFilterObserver*> observers_;
};

#endif
