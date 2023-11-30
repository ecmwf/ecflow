/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
        if (verbose_) {
            std::ostringstream os;
            os << "   " << location << " (" << qualifier << "): ";
            ((os << args), ...);
            store(os.str());
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
    { ecf::log::getTrace().add("error", location, __VA_ARGS__); }

#endif /* ecflow_udp_Trace_HPP */
