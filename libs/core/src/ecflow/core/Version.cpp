/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Version.hpp"

#include <sstream>

#include <boost/version.hpp>
#include <cereal/version.hpp>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/ecflow_version.h"

namespace ecf {

std::string Version::major() {
    return ECFLOW_VERSION_MAJOR;
}

std::string Version::minor() {
    return ECFLOW_VERSION_MINOR;
}

std::string Version::patch() {
    return ECFLOW_VERSION_PATCH;
}

std::string Version::suffix() {
    return ECFLOW_VERSION_SUFFIX;
}

std::string Version::description() {
    std::stringstream ss;
    ss << "Ecflow ";
#ifdef DEBUG
    ss << "(debug) ";
#endif
    ss << "version(" << Version::full() << ") ";
    ss << "boost(" << Version::boost() << ") ";
    ss << "compiler(" << Version::compiler() << ") ";
    ss << "protocol(JSON cereal " << Version::cereal() << ") ";
#ifdef ECF_OPENSSL
    ss << "openssl(enabled) ";
#endif
    ss << "Compiled on " << __DATE__ << " " << __TIME__;
    return ss.str();
}

std::string Version::base() {
    std::string ret = major();
    ret += ".";
    ret += minor();
    ret += ".";
    ret += patch();
    return ret;
}

std::string Version::full() {
    return Version::base() + Version::suffix();
}

std::string Version::boost() {
    std::stringstream ss;
    ss << BOOST_VERSION / 100000 << "."     // major version
       << BOOST_VERSION / 100 % 1000 << "." // minor version
       << BOOST_VERSION % 100;              // patch level
    return ss.str();
}

std::string Version::cereal() {
    std::stringstream ss;
    ss << CEREAL_VERSION_MAJOR         // major version
       << "." << CEREAL_VERSION_MINOR  // minor version
       << "." << CEREAL_VERSION_PATCH; // patch level
    return ss.str();
}

std::string Version::compiler() {
    std::stringstream ss;
#if defined(_AIX)
    ss << "aix " << __IBMCPP__;
#elif defined(HPUX)
    ss << "aCC " << __HP_aCC; // type aCC +help, this will show compiler manual, search for Predefined Macros
#else
    #if defined(__clang__)
    //  To find the list of defines for clang use:
    //  echo | /usr/local/apps/clang/current/bin/clang++ -dM -E -
    ss << "clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
    #elif defined(__INTEL_COMPILER)
    ss << "intel " << __INTEL_COMPILER;
    #elif defined(_CRAYC)
    ss << "cray " << _CRAYC;
    #else
    ss << "gcc " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
    #endif
#endif
    auto version = ss.str();

    return version.empty() ? "unknown" : version;
}

} // namespace ecf
