/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Base64_HPP
#define ecflow_core_Base64_HPP

#include <string>
#include <string_view>

namespace ecf {

/**
 * @brief Decode a base64 encoded string.
 *
 * @param value the base64 encoded string
 * @return the decoded string
 */
std::string decode_base64(const std::string& value);

/**
 * @brief Encode a string to base64.
 *
 * @param value the string to encode
 * @return the base64 encoded string
 */
std::string encode_base64(const std::string& value);

/**
 * @brief Simple validation of base64 encoded string.
 *
 * Checks the following conditions:
 * - length has to be multiple of 4
 * - valid characters are  A-Za-z0-9+/
 * padding of 0-2 characters at the end of string can be =
 *
 * @param value the base64 encoded string
 * @return true if valid, false otherwise
 */

bool validate_base64(const std::string& value);

} // namespace ecf

#endif // ecflow_core_Base64_HPP
