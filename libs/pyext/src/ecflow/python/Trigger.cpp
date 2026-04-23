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

static void construct_expr(std::vector<PartExpression>& vec, const py::list& list) {
    int the_list_size = len(list);
    for (int i = 0; i < the_list_size; ++i) {
        std::string part_expr;
        if (auto extracted = py::extract<std::string>(list[i]); extracted.check()) {
            part_expr = extracted();
            if (ecf::algorithm::is_valid_name(part_expr)) {
                part_expr += " == complete";
            }
        }
        else if (auto extracted = py::extract<node_ptr>(list[i]); extracted.check()) {
            node_ptr node = extracted();
            if (node->parent()) {
                part_expr = node->absNodePath();
            }
            else {
                part_expr = node->name();
            }
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
