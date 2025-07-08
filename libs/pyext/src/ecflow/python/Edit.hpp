/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_Edit_HPP
#define ecflow_python_Edit_HPP

#include <vector>

#include <boost/python.hpp>

#include "ecflow/attribute/Variable.hpp"

namespace bp = boost::python;

class Edit {
public:
    explicit Edit(const bp::dict& dict);
    Edit(const bp::dict& dict, const bp::dict& dict2);
    const std::vector<Variable>& variables() const { return vec_; }
    static std::string to_string() { return "edit"; }
    static bp::object init(bp::tuple args, bp::dict kw);

private:
    std::vector<Variable> vec_;
};

#endif /* ecflow_python_Edit_HPP */
