#ifndef STRING_SPLITTER_HPP_
#define STRING_SPLITTER_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <vector>
#include <boost/utility/string_ref.hpp>

namespace ecf {

// Will split a string. Will return a empty boost::string_ref if there a separator at the end.
// This shows the fastest split for a string.
//    Method:              time
//    boost::split:        4.06
//    Str::split:          2.33
//    make_split_iterator  4.07
//    boost::string_ref    1.42

class StringSplitter {
   boost::string_ref src_;
   mutable boost::string_ref rem_;
   boost::string_ref sep_;
   mutable bool finished_;
   mutable boost::string_ref::size_type first_not_of_;

public:
   StringSplitter(boost::string_ref src, boost::string_ref sep = " \t") : src_(src),rem_(src), sep_(sep),finished_(false),first_not_of_(0) {}
   boost::string_ref next() const;
   bool finished() const;
   void reset();

   static void split(const std::string& str,
                     std::vector< boost::string_ref >& lineTokens,
                     boost::string_ref delimiters = " \t");
};

}
#endif /* STRING_SPLITTER_HPP_ */
