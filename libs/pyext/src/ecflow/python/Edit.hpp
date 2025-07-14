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

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/python/PythonBinding.hpp"

class Edit {
public:
    explicit Edit(const py::dict& dict);
    Edit(const py::dict& dict, const py::dict& dict2);
    const std::vector<Variable>& variables() const { return vec_; }
    static std::string to_string() { return "edit"; }
    static py::object init(py::tuple args, py::dict kw);

private:
    std::vector<Variable> vec_;
};

#endif /* ecflow_python_Edit_HPP */
