/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/GenericAttr.hpp"

#include <stdexcept>

#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"

using namespace ecf;
using namespace boost;
using namespace std;

const GenericAttr& GenericAttr::EMPTY() {
    static const GenericAttr GENERICATTR = GenericAttr();
    return GENERICATTR;
}

GenericAttr::GenericAttr(const std::string& name, const std::vector<std::string>& values)
    : name_(name),
      values_(values) {
    string msg;
    if (!Str::valid_name(name, msg)) {
        throw std::runtime_error("GenericAttr::GenericAttr : Invalid generic name : " + msg);
    }
}

GenericAttr::GenericAttr(const std::string& name) : name_(name) {
    string msg;
    if (!Str::valid_name(name, msg)) {
        throw std::runtime_error("GenericAttr::GenericAttr : Invalid generic name : " + msg);
    }
}

bool GenericAttr::operator==(const GenericAttr& rhs) const {
    if (name_ != rhs.name_) {
        return false;
    }
    if (values_ != rhs.values_) {
        return false;
    }
    return true;
}

std::string GenericAttr::to_string() const {
    std::string ret;
    write(ret);
    return ret;
}

void GenericAttr::write(std::string& ret) const {
    ret += "generic ";
    ret += name_;
    for (const auto& value : values_) {
        ret += " ";
        ret += value;
    }
}

template <class Archive>
void GenericAttr::serialize(Archive& ar) {
    ar(CEREAL_NVP(name_), CEREAL_NVP(values_));
}
CEREAL_TEMPLATE_SPECIALIZE(GenericAttr);
