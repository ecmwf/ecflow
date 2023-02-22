/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : BasicAuth
// Author      : partio
// Revision    : $Revision$
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "BasicAuth.hpp"

#include "Base64.hpp"
#include "HttpServerException.hpp"
#include "PasswordEncryption.hpp"
#include "Str.hpp"

std::pair<std::string, std::string> BasicAuth::get_credentials(const std::string& token) {
    const std::string decoded = base64_decode(token);
    std::vector<std::string> elems;
    ecf::Str::split(decoded, elems, ":");

    return std::make_pair(elems[0], PasswordEncryption::encrypt(elems[1], elems[0]));
}
