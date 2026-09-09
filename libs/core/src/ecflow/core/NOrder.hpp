/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_NOrder_HPP
#define ecflow_core_NOrder_HPP

#include <string>

class NOrder {
    NOrder() = default;

public:
    enum Order { TOP, BOTTOM, ALPHA, ORDER, UP, DOWN, RUNTIME /* state change runtime */ };

    static std::string toString(NOrder::Order);
    static NOrder::Order toOrder(const std::string&);
    static bool isValid(const std::string& order);
};

#endif /* ecflow_core_NOrder_HPP */
