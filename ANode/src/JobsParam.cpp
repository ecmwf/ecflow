//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <iostream>
#include "JobsParam.hpp"
#include "Str.hpp"

using namespace ecf;

size_t JobsParam::start_profile()
{
   profiles_.push_back(std::make_pair(std::string(), 0));
   return (profiles_.size() - 1);
}

size_t JobsParam::last_profile_index() const {
   if (profiles_.empty()) return 0;
   return profiles_.size()-1;
}

void JobsParam::add_to_profile( size_t index, const std::string& s )
{
   if (index < profiles_.size()) {
      profiles_[index].first += s;
   }
}

void JobsParam::set_to_profile( size_t index, const std::string& s, int time_taken )
{
   if (index < profiles_.size()) {
      profiles_[index].first = s;
      profiles_[index].second = time_taken;
   }
}

const std::string& JobsParam::get_text_at_profile(size_t index) const
{
   if (index < profiles_.size()) {
      return profiles_[index].first;
   }
   return Str::EMPTY();
}

