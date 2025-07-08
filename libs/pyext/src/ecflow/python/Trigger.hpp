/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_Trigger_HPP
#define ecflow_python_Trigger_HPP

#include "ecflow/node/Expression.hpp"
#include "ecflow/python/PythonBinding.hpp"

////////////////////////////////////////////////////////////////////////////////////////
// Trigger & Complete thin wrapper over Expression, allows us to call:
//  Task("a").add(Trigger("a == 1"),Complete("b == 1"))
///////////////////////////////////////////////////////////////////////////////////

class Trigger {
public:
    explicit Trigger(const std::string& expression) { add(PartExpression(expression)); }
    Trigger(const std::string& expression, bool and_type) { add(PartExpression(expression, and_type)); }
    explicit Trigger(const PartExpression& pe) { add(pe); }
    Trigger(const Trigger& rhs) = default;
    explicit Trigger(const bp::list& list);

    bool operator==(const Trigger& rhs) const { return vec_ == rhs.vec_; }
    bool operator!=(const Trigger& rhs) const { return !operator==(rhs); }
    std::string expression() const { return Expression::compose_expression(vec_); }

    const std::vector<PartExpression>& expr() const { return vec_; }

private:
    void add(const PartExpression& t) { vec_.push_back(t); }
    std::vector<PartExpression> vec_;
    Trigger& operator=(Trigger const& f) = delete; // prevent assignment
};

class Complete {
public:
    explicit Complete(const std::string& expression) { add(PartExpression(expression)); }
    Complete(const std::string& expression, bool and_type) { add(PartExpression(expression, and_type)); }
    explicit Complete(const PartExpression& pe) { add(pe); }
    Complete(const Complete& rhs) = default;
    explicit Complete(const bp::list& list);

    bool operator==(const Complete& rhs) const { return vec_ == rhs.vec_; }
    bool operator!=(const Complete& rhs) const { return !operator==(rhs); }
    std::string expression() const { return Expression::compose_expression(vec_); }

    const std::vector<PartExpression>& expr() const { return vec_; }

private:
    void add(const PartExpression& t) { vec_.push_back(t); }
    std::vector<PartExpression> vec_;
    Complete& operator=(Complete const& f) = delete; // prevent assignment
};

#endif /* ecflow_python_Trigger_HPP */
