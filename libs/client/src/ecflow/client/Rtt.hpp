/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_Rtt_HPP
#define ecflow_client_Rtt_HPP

#include <fstream>
#include <sstream>
#include <string>

///
/// \brief Simple client based singleton for recording round trip times of all client based command
///

namespace ecf {

class Rtt {
public:
    Rtt(const Rtt&)                  = delete;
    const Rtt& operator=(const Rtt&) = delete;

    static void create(const std::string& filename);
    static void destroy();
    static Rtt* instance() { return instance_; }

    void log(const std::string& message);

    /// Open the file, and create average times for all client invoker round trip times
    static std::string analyis(const std::string& filename);

    /// Used in output and parsing, when computing averages
    static const char* tag() { return "rtt:"; }

private:
    ~Rtt();
    explicit Rtt(const std::string& filename);
    static Rtt* instance_;
    mutable std::ofstream file_;
};

void rtt(const std::string& message);

// allow user to do the following:
// RTT("this is " << path << " ok ");
//
// helper, see STRINGIZE() macro
template <typename Functor>
std::string stringize_rtt(Functor const& f) {
    std::ostringstream out;
    f(out);
    return out.str();
}
#define STRINGIZE_RTT(EXPRESSION) (ecf::stringize_rtt([](std::ostringstream& os) { os << EXPRESSION };))
#define RTT(EXPRESSION) ecf::rtt(STRINGIZE_RTT(EXPRESSION))
} // namespace ecf
#endif
