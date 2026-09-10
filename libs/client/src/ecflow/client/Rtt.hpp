/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
private:
    explicit Rtt(const std::string& filename);

public:
    Rtt() = delete;

    Rtt(const Rtt&)                  = delete;
    const Rtt& operator=(const Rtt&) = delete;
    Rtt(Rtt&&)                       = delete;
    Rtt& operator=(Rtt&&)            = delete;

    ~Rtt();

    static void create(const std::string& filename);
    static void destroy();
    static Rtt* instance() { return instance_; }

    void log(const std::string& message);

    /// Open the file, and create average times for all client invoker round trip times
    static std::string analysis(const std::string& filename);

    /// Used in output and parsing, when computing averages
    static const char* tag() { return "rtt:"; }

private:
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
    std::ostringstream ss;
    f(ss);
    return ss.str();
}

// NOLINTBEGIN(bugprone-macro-parentheses)
#define STRINGIZE_RTT(EXPRESSION) (ecf::stringize_rtt([](std::ostringstream& os) { os << EXPRESSION };))
// NOLINTEND(bugprone-macro-parentheses)
#define RTT(EXPRESSION) ecf::rtt(STRINGIZE_RTT(EXPRESSION))

} // namespace ecf

#endif /* ecflow_client_Rtt_HPP */
