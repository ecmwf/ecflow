/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Extract.hpp"

#include <stdexcept>

#include "ecflow/core/Chrono.hpp"

bool Extract::pathAndName(const std::string& token, std::string& path, std::string& name) {
    if (token.empty()) {
        return false;
    }

    size_t colonPos = token.find_first_of(':');
    if (colonPos == std::string::npos) {
        if (token[0] == '/') {
            path = token; // token of the form /a/b/c, ie no name
        }
        else {
            name = token;
        }
    }
    else {
        path = token.substr(0, colonPos);
        name = token.substr(colonPos + 1);
    }

    return true;
}

bool Extract::split_get_second(const std::string& str, std::string& ret, char separator) {
    // HH:MM
    // return MM;
    size_t colonPos = str.find_first_of(separator);
    if (colonPos == std::string::npos) {
        return false;
    }
    ret = str.substr(colonPos + 1);
    return true;
}

/// extract YMD, integer of the form yyyymmdd
int Extract::ymd(const std::string& ymdToken, std::string& errorMsg) {
    if (ymdToken.size() != 8) {
        throw std::runtime_error(errorMsg + " YMD must be 8 characters i.e yyyymmdd");
    }

    // Use date lib to check YMD
    try {
        (void)boost::gregorian::date(boost::gregorian::from_undelimited_string(ymdToken));
    }
    catch (std::exception& e) {
        errorMsg += "\n";
        errorMsg += e.what();
        throw std::runtime_error(errorMsg + " YMD is not a valid date");
    }

    return value<int>(ymdToken, errorMsg);
}
