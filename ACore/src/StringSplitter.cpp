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
   // Skip delimiters at beginning. so for an empty string or string of just separators, we do not call next()
   // This mirrors the old split functionality
   if (finished_) return true;
   first_not_of_ = rem_.find_first_not_of( sep_);
   if (first_not_of_ == boost::string_view::npos)  finished_ = true;
   return finished_;
}

boost::string_view StringSplitter::next() const
{
   if (first_not_of_ != 0) rem_ = rem_.substr(first_not_of_);

   boost::string_view::size_type pos = rem_.find_first_of( sep_ );        // Find first "non-delimiter".
   if (pos != boost::string_view::npos) {
      boost::string_view ret = rem_.substr(0,pos);
      rem_ = rem_.substr(pos+1);   // skip over separator

      // if separator is at the end, lose the last empty tokens
      if ( rem_.find_first_not_of( sep_) == boost::string_view::npos) {
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

void StringSplitter::split(const std::string& str, std::vector< boost::string_view >& lineTokens, boost::string_view delimiters)
{
   StringSplitter string_splitter(str,delimiters);
   while( !string_splitter.finished() ) {
      lineTokens.push_back(string_splitter.next());
   }
}

void StringSplitter::split(const std::string& str,std::vector<std::string>& lineTokens, boost::string_view delimiters)
{
   StringSplitter string_splitter(str,delimiters);
   while(!string_splitter.finished()) {
      boost::string_view ref = string_splitter.next();
      lineTokens.push_back(std::string(ref.begin(),ref.end()));
   }
}

}
