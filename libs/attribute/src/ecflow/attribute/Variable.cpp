/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/Variable.hpp"

#include <sstream>
#include <stdexcept>

#include "ecflow/core/Message.hpp"
#include "ecflow/core/Serialization.hpp"

using namespace ecf;

bool Variable::operator==(const Variable& rhs) const {
    if (v_ != rhs.v_) {
        return false;
    }
    if (n_ != rhs.n_) {
        return false;
    }
    return true;
}

std::string Variable::toString() const {
    std::string ret;
    ret.reserve(n_.size() + v_.size() + 8);
    write(ret);
    return ret;
}

std::string Variable::dump() const {
    return MESSAGE(toString() << " value(" << value() << ")");
}

void Variable::write(std::string& ret) const {
    ret += "edit ";
    ret += n_;
    ret += " '";
    if (v_.find("\n") == std::string::npos) {
        ret += v_;
    }
    else {
        // replace \n, otherwise re-parse will fail
        std::string value = v_;
        ecf::algorithm::replace_all(value, "\n", "\\n");
        ret += value;
    }
    ret += "'";
}

const Variable& Variable::EMPTY() {
    static const Variable VARIABLE = Variable();
    return VARIABLE;
}

template <class Archive>
void Variable::serialize(Archive& ar) {
    ar(CEREAL_NVP(n_), CEREAL_NVP(v_));
}
CEREAL_TEMPLATE_SPECIALIZE(Variable);

void VariableMap::set_value(const std::string& value) {
    for (auto& variable : variables_) {
        variable.set_value(value);
    }
}

Variable& VariableMap::operator[](const std::string& name) {
    if (auto found = index_.find(name); found != std::end(index_)) {
        return variables_[found->second];
    }
    throw std::runtime_error(MESSAGE("Variable '" << name << "' not found"));
}
