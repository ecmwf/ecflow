/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Indentor_HPP
#define ecflow_core_Indentor_HPP

///
/// \brief This class is used as a helper class to produce indented output
///

#include <iosfwd>
#include <string>

namespace ecf {

class Indentor {
public:
    Indentor() { ++index_; }
    ~Indentor() { --index_; }

    static void indent(std::string& os, int char_spaces = 2);

    template <typename Stream>
    static Stream& indent(Stream& s, int char_spaces = 2) {
        s << (indent_ ? std::string(index_ * char_spaces, ' ') : std::string(""));
        return s;
    }

private:
    static int index_;

private:
    friend class DisableIndentor;
    static void disable_indent() { indent_ = false; }
    static void enable_indent() { indent_ = true; }
    static bool indent_;
};

class DisableIndentor {
public:
    DisableIndentor() { Indentor::disable_indent(); }
    ~DisableIndentor() { Indentor::enable_indent(); }
};

} // namespace ecf

#endif /* ecflow_core_Indentor_HPP */
