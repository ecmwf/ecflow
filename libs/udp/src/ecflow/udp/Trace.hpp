/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_udp_Trace_HPP
#define ecflow_udp_Trace_HPP

#include <sstream>
#include <string>

namespace ecf::log {

class Trace {
public:
    Trace();
    ~Trace() = default;

    inline void setVerbose(bool verbose) { verbose_ = verbose; }

    template <typename... ARGS>
    void add(const std::string& qualifier, const std::string& location, ARGS... args) const {
        if (verbose_ || qualifier == "error" || qualifier == "fatal") {
            std::ostringstream ss;
            ss << "   " << location << " (" << qualifier << "): ";
            ((ss << args), ...);
            store(ss.str());
        }
    }

private:
    void store(const std::string& entry) const;

    std::ostream& output_;
    bool verbose_;
};

Trace& getTrace();

} // namespace ecf::log

#define TRACE_VERBOSE(verbose) \
    { ecf::log::getTrace().setVerbose(verbose); }

#define TRACE_NFO(location, ...) \
    { ecf::log::getTrace().add("info", location, __VA_ARGS__); }

#define TRACE_ERR(location, ...) \
    { ecf::log::getTrace().add("error", location, __VA_ARGS__); }

#define TRACE_FATAL(location, ...) \
    { ecf::log::getTrace().add("fatal", location, __VA_ARGS__); }

#endif /* ecflow_udp_Trace_HPP */
