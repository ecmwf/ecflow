#ifndef STRING_SPLITTER_HPP_
#define STRING_SPLITTER_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <string>
#include <vector>

#include <string_view>

namespace ecf {

// ****************************************************************************
// IMPORTANT: std::string_view is a *READ ONLY* REFERENCE to an existing string
// HENCE:     the reference string *MUST* not change, and its lifetime must EXCEED string_view
//
// Will split a string. Will return a empty std::string_view if there a separator at the end.
// This shows the fastest split for a string. **** Based on boost 1.64 ****
//    Method:              time
//    boost::split:        4.06
//    Str::split:          2.33
//    make_split_iterator  4.07
//    std::string_view    1.42

class StringSplitter {
    std::string_view src_;
    mutable std::string_view rem_;
    std::string_view sep_;
    mutable std::string_view::size_type first_not_of_;
    mutable bool finished_;

public:
    explicit StringSplitter(std::string_view src, std::string_view sep = " \t")
        : src_(src),
          rem_(src),
          sep_(sep),
          first_not_of_(0),
          finished_(false) {}

    // this rules out temp strings, it also rules out char * because of two available overloads
    StringSplitter(const std::string&& src, std::string_view sep) = delete;

    // this re-enables support for string literals (which are never temp)
    // it even handles correctly char arrays that contain a null terminated string
    // because string_view does not have a char array constructor!
    template <std::size_t N>
    explicit StringSplitter(const char (&sz)[N], std::string_view sep = " \t")
        : src_(sz),
          rem_(sz),
          sep_(sep),
          first_not_of_(0),
          finished_(false) {}

    std::string_view next() const;
    bool finished() const;
    bool last() const { return finished_; }
    void reset();

    /// The preferred splitter as it does not create strings
    static void
    split(const std::string& str, std::vector<std::string_view>& lineTokens, std::string_view delimiters = " \t");

    // This the fastest splitter at the moment
    static void split2(std::string_view str, std::vector<std::string_view>& lineTokens, const char* delimiters = " \t");

    /// This was added to maintain compatibility, slightly faster than original Str::split
    static void
    split(const std::string& str, std::vector<std::string>& lineTokens, std::string_view delimiters = " \t");

    /// return the token at pos, otherwise returns false.
    static bool get_token(std::string_view line, size_t pos, std::string& token, std::string_view sep = " \t");
};

} // namespace ecf
#endif /* STRING_SPLITTER_HPP_ */
