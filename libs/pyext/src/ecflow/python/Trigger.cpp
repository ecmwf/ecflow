/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/python/Trigger.hpp"

#include <stdexcept>

#include "ecflow/core/Str.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Task.hpp"

static void construct_expr(std::vector<PartExpression>& vec, const py::list& list) {
    for (const auto& entry : list) {
        std::string part_expr;
        if (auto found = py_extract<py::str>(entry); found) {
            part_expr = found.value();
            if (ecf::algorithm::is_valid_name(part_expr)) {
                part_expr += " == complete";
            }
        }
        else if (auto found = py_extract<node_ptr>(entry); found) {
            node_ptr node = found.value();
            if (node->parent()) {
                part_expr = node->absNodePath();
            }
            else {
                part_expr = node->name();
            }
            part_expr += " == complete";
        }
        else if (auto found = py_extract<Task>(entry); found) {
            const auto& node = found.value();
            part_expr        = node.parent() ? node.absNodePath() : node.name();
            part_expr += " == complete";
        }
        else {
            throw std::runtime_error("Trigger: Expects string, or list(strings or nodes)");
        }

        if (vec.empty()) {
            vec.emplace_back(part_expr);
        }
        else {
            vec.emplace_back(part_expr, true /*AND*/);
        }
    }
}

Trigger::Trigger(const py::list& list) {
    construct_expr(vec_, list);
}
Complete::Complete(const py::list& list) {
    construct_expr(vec_, list);
}
