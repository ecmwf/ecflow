//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "FilterData.hpp"

#include <algorithm>

FilterData::FilterData()
{
		nodeState_.insert(DState::UNKNOWN);
		nodeState_.insert(DState::SUSPENDED);
		nodeState_.insert(DState::COMPLETE);
		nodeState_.insert(DState::QUEUED);
		nodeState_.insert(DState::SUBMITTED);
		nodeState_.insert(DState::ACTIVE);
		nodeState_.insert(DState::ABORTED);
}

void FilterData::setNodeState(const std::set<DState::State>& ns)
{
		nodeState_=ns;
		broadcastChange();
}

bool FilterData::isNodeStateFiltered() const
{
	return (nodeState_.size() != 7);
}

void FilterData::broadcastChange()
{
	for(std::vector<FilterDataObserver*>::iterator it=observers_.begin(); it != observers_.end(); it++)
	{
		(*it)->notifyFilterChanged();
	}
}

void  FilterData::addObserver(FilterDataObserver* obs)
{
	observers_.push_back(obs);
}

void  FilterData::removeObserver(FilterDataObserver* obs)
{
	std::vector<FilterDataObserver*>::iterator it=std::find(observers_.begin(),observers_.end(),obs);
	if(it != observers_.end())
		observers_.erase(it);
}
