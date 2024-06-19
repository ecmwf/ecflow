/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/JSON.hpp"

#include "ecflow/core/Converter.hpp"

namespace ecf::http {

std::string json_type_to_string(const ojson& j) {
    switch (j.type()) {
        case ojson::value_t::null:
            return "null";
        case ojson::value_t::boolean:
            return (j.get<bool>()) ? "true" : "false";
        case ojson::value_t::string:
            return j.get<std::string>();
        case ojson::value_t::binary:
            return j.dump();
        case ojson::value_t::array:
        case ojson::value_t::object:
            return j.dump();
        case ojson::value_t::discarded:
            return "discarded";
        case ojson::value_t::number_integer:
            return ecf::convert_to<std::string>(j.get<int>());
        case ojson::value_t::number_unsigned:
            return ecf::convert_to<std::string>(j.get<unsigned int>());
        case ojson::value_t::number_float:
            return ecf::convert_to<std::string>(j.get<double>());
        default:
            return std::string();
    }
}

} // namespace ecf::http
