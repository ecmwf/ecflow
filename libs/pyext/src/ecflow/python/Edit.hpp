/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_python_Edit_HPP
#define ecflow_python_Edit_HPP

#include <vector>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/python/PythonBinding.hpp"

class Edit {
public:
    explicit Edit(const py::dict& dict);
    explicit Edit(const py::kwargs& kw);
    Edit(const py::dict& dict, const py::kwargs& kw);
    Edit(const py::dict& dict1, const py::dict& dict2);

    const std::vector<Variable>& variables() const { return vec_; }

    static std::string to_string() { return "edit"; }

private:
    std::vector<Variable> vec_;
};

#endif /* ecflow_python_Edit_HPP */
