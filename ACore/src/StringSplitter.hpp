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
#include <boost/utility/string_view.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace ecf {

// ****************************************************************************
// IMPORTANT: boost::string_view is a *READ ONLY* REFERENCE to an existing string
// HENCE:     the reference string *MUST* not change, and its lifetime must EXCEED string_view
//
// Will split a string. Will return a empty boost::string_view if there a separator at the end.
// This shows the fastest split for a string. **** Based on boost 1.64 ****
//    Method:              time
//    boost::split:        4.06
//    Str::split:          2.33
//    make_split_iterator  4.07
//    boost::string_view    1.42

class StringSplitter {
   boost::string_view src_;
   mutable boost::string_view rem_;
   boost::string_view sep_;
   mutable bool finished_;
   mutable boost::string_view::size_type first_not_of_;

public:
   StringSplitter(boost::string_view src, boost::string_view sep = " \t") : src_(src),rem_(src), sep_(sep),finished_(false),first_not_of_(0) {}
   //#if C++11
      // this rules out temp strings, it also rules out char * because of two available overloads
      //StringSplitter(const std::string&& src, boost::string_view sep) = delete;

      // this re-enables support for string literals (which are never temp)
      // it even handles correctly char arrays that contain a null terminated string
      // because string_view does not have a char array constructor!
      // template<std::size_t N>
      //StringSplitter(const char (&sz)[N], boost::string_view sep) : src_(sz),rem_(sz),sep_(sep),finished_(false),first_not_of_(0) {}
   //
   boost::string_view next() const;
   bool finished() const;
   void reset();

   /// The preferred splitter as it does not create strings
   static void split(const std::string& str,
                     std::vector<boost::string_view >& lineTokens,
                     boost::string_view delimiters = " \t");

   static void split2(boost::string_view str ,
                     std::vector<boost::string_view>& lineTokens,
                     const char* delimiters = " \t");

   /// This was added to maintain compatibility, slightly faster than original Str::split
   static void split(const std::string& str,
                     std::vector< std::string >& lineTokens,
                     boost::string_view delimiters = " \t");


//   class const_iterator : public boost::iterator_facade<const_iterator, boost::string_view, boost::single_pass_traversal_tag>
//   {
//         const StringSplitter* splitter_;
//         boost::string_view value_;
//      public:
//         explicit const_iterator(const StringSplitter  * sp) : splitter_(sp) {}
//
//         const_iterator & operator ++() {
//            if (! splitter_->finished())
//               value_ = splitter_->next();
//            else
//               splitter_ = nullptr;
//            return *this;
//         }
//
//         reference operator *() { return value_; }
//
//         bool operator !=(const const_iterator & other) const { return splitter_ != other.splitter_; }
//
//
//         friend class boost::iterator_core_access;
//         void increment() {
//            value_ = splitter_->next();
//         }
//   };

};

}
#endif /* STRING_SPLITTER_HPP_ */
