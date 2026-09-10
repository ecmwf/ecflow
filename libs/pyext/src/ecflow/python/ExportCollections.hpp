/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_python_ExportCollections_HPP
#define ecflow_python_ExportCollections_HPP

#include <vector>

#include <pybind11/pybind11.h>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<Variable>)

PYBIND11_MAKE_OPAQUE(std::vector<node_ptr>)
PYBIND11_MAKE_OPAQUE(std::vector<suite_ptr>)
PYBIND11_MAKE_OPAQUE(std::vector<family_ptr>)
PYBIND11_MAKE_OPAQUE(std::vector<task_ptr>)

// Important!
//
// The following #include must appear after the use of PYBIND11_MAKE_OPAQUE.
// As such, this header does not include PythonBinding.hpp, and directly includes pybind11/pybind11.h and pybind11/stl.h
//
//
#include <pybind11/stl.h>

#endif /* ecflow_python_ExportCollections_HPP */
