/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/BasicAuth.hpp"

#include "ecflow/core/Base64.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/http/HttpServerException.hpp"

namespace ecf::http {

std::pair<std::string, std::string> BasicAuth::get_credentials(const std::string& token) {
    std::vector<std::string> elems;
    ecf::Str::split(decode_base64(token), elems, ":");

    return std::make_pair(elems[0], PasswordEncryption::encrypt(elems[1], elems[0]));
}

} // namespace ecf::http
