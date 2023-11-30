/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_SuiteChanged_HPP
#define ecflow_node_SuiteChanged_HPP

#include "NodeFwd.hpp"

namespace ecf {

// Determine if suite was changed or modified if so, update suite change no
// This mechanism was used because, when changing some attributes, we cannot
// immediately access the parent suites, to update the change numbers.
//
// When given a choice between where to add SuiteChanged, i.e in Node Tree or Commands
// Generally favour commands, as it will require less maintenance over time.
//
// This mechanism was added specifically to support changes over client handles
// i.e suites are added to handles, hence we need a way to determine which
// suites (and hence handle) changed, and hence minimise the need for updates.

class SuiteChanged {
private:
    SuiteChanged(const SuiteChanged&)                  = delete;
    const SuiteChanged& operator=(const SuiteChanged&) = delete;

public:
    explicit SuiteChanged(suite_ptr s);
    ~SuiteChanged();

private:
    weak_suite_ptr suite_;
    unsigned int state_change_no_;
    unsigned int modify_change_no_;
};

class SuiteChanged0 {
private:
    SuiteChanged0(const SuiteChanged0&)                  = delete;
    const SuiteChanged0& operator=(const SuiteChanged0&) = delete;

public:
    explicit SuiteChanged0(node_ptr s);
    ~SuiteChanged0();

private:
    weak_node_ptr node_;
    Suite* suite_; // if node is removed suite pointer is not accessible, hence store first
    unsigned int state_change_no_;
    unsigned int modify_change_no_;
};

// Faster than using node_ptr.
class SuiteChangedPtr {
private:
    SuiteChangedPtr(const SuiteChangedPtr&)                  = delete;
    const SuiteChangedPtr& operator=(const SuiteChangedPtr&) = delete;

public:
    explicit SuiteChangedPtr(Node* s);
    ~SuiteChangedPtr();

private:
    Suite* suite_; // if node is removed suite pointer is not accessible, hence store first
    unsigned int state_change_no_;
    unsigned int modify_change_no_;
};

class SuiteChanged1 {
private:
    SuiteChanged1(const SuiteChanged1&)                  = delete;
    const SuiteChanged1& operator=(const SuiteChanged1&) = delete;

public:
    explicit SuiteChanged1(Suite* s);
    ~SuiteChanged1();

private:
    Suite* suite_;
    unsigned int state_change_no_;
    unsigned int modify_change_no_;
};

} // namespace ecf

#endif /* ecflow_node_SuiteChanged_HPP */
