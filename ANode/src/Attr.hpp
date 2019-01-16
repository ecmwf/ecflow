#ifndef ATTR_HPP_
#define ATTR_HPP_
// Name        :
//============================================================================
// Author      : Avi
// Revision    : $Revision: #16 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <boost/noncopyable.hpp>
#include <string>
#include <vector>

namespace ecf {
class Attr : private boost::noncopyable {
public:
   enum Type { UNKNOWN=0, EVENT=1, METER=2, LABEL=3, LIMIT=4, VARIABLE=5, ALL=6 };

    static const char* to_string(Attr::Type s);
    static Attr::Type to_attr(const std::string& attr);
    static bool is_valid(const std::string& state);
    static std::vector<std::string> all_attrs();
    static std::vector<Attr::Type> attrs();
};
}
#endif
