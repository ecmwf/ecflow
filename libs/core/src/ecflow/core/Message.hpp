/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Message_HPP
#define ecflow_core_Message_HPP

#include <sstream>
#include <string>

namespace ecf {

///
/// \brief Message is a utility class that easily creates a string-based Message from an arbitrary set of inputs
///
/// The goal is to facilitate the construction/formatting of messages for error handling and logging.
/// A Message implicitly allows conversion to std::string, and provides the streaming operator<<.
///
class Message {
public:
    template <typename... ARGS>
    explicit Message(ARGS&&... args) {
        ((buffer << std::forward<ARGS>(args)), ...);
    }

    [[nodiscard]] std::string str() const { return buffer.str(); }
    [[nodiscard]] operator std::string() const { return buffer.str(); }

private:
    std::ostringstream buffer;
};

inline std::ostream& operator<<(std::ostream& o, const Message& m) {
    o << std::string(m);
    return o;
}

} // namespace ecf

#endif /* ecflow_core_Message_HPP */
