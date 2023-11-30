/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/NOrder.hpp"

#include <cassert>

std::string NOrder::toString(NOrder::Order s) {
    switch (s) {
        case NOrder::TOP:
            return "top";
        case NOrder::BOTTOM:
            return "bottom";
        case NOrder::ALPHA:
            return "alpha";
        case NOrder::ORDER:
            return "order";
        case NOrder::UP:
            return "up";
        case NOrder::DOWN:
            return "down";
        case NOrder::RUNTIME:
            return "runtime";
        default:
            assert(false);
            break;
    }
    assert(false);
    return std::string();
}

NOrder::Order NOrder::toOrder(const std::string& str) {
    if (str == "top")
        return NOrder::TOP;
    if (str == "bottom")
        return NOrder::BOTTOM;
    if (str == "alpha")
        return NOrder::ALPHA;
    if (str == "order")
        return NOrder::ORDER;
    if (str == "up")
        return NOrder::UP;
    if (str == "down")
        return NOrder::DOWN;
    if (str == "runtime")
        return NOrder::RUNTIME;
    assert(false);
    return NOrder::TOP;
}

bool NOrder::isValid(const std::string& order) {
    if (order == "top")
        return true;
    if (order == "bottom")
        return true;
    if (order == "alpha")
        return true;
    if (order == "order")
        return true;
    if (order == "up")
        return true;
    if (order == "down")
        return true;
    if (order == "runtime")
        return true;
    return false;
}
