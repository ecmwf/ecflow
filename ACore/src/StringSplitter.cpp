/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "StringSplitter.hpp"

namespace ecf {

bool StringSplitter::finished() const
{
   first_not_of_ = rem_.find_first_not_of( sep_); // Skip delimiters at beginning.
   if (first_not_of_ == boost::string_ref::npos)  finished_ = true;
   return finished_;
}

boost::string_ref StringSplitter::next() const
{
   if (first_not_of_ != 0) rem_ = rem_.substr(first_not_of_);

   boost::string_ref::size_type pos = rem_.find_first_of( sep_ );        // Find first "non-delimiter".
   if (pos != boost::string_ref::npos) {
      boost::string_ref ret = rem_.substr(0,pos);
      rem_ = rem_.substr(pos+1);   // skip over separator

      // if separator is at the end, lose the last empty tokens
      if ( rem_.find_first_not_of( sep_) == boost::string_ref::npos) {
         finished_ = true;
      }
      return ret;
   }

   finished_ = true;
   return rem_;
}

void StringSplitter::reset() {
   rem_ = src_;
   finished_ = false;
}


}
