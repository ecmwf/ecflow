/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/http/BasicAuth.hpp"

#include "ecflow/core/Base64.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/http/HttpServerException.hpp"

namespace ecf::http {

std::pair<std::string, std::string> BasicAuth::get_credentials(const std::string& token) {
    std::vector<std::string> elems;
    ecf::algorithm::split_at(elems, decode_base64(token), ":");

    return std::make_pair(elems[0], PasswordEncryption::encrypt(elems[1], elems[0]));
}

} // namespace ecf::http
