/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef DEBUG
    #include <iostream>
#endif

#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"

namespace ecf {

SuiteChanged::SuiteChanged(suite_ptr s)
    : suite_(s),
      state_change_no_(Ecf::state_change_no()),
      modify_change_no_(Ecf::modify_change_no()) {
}

SuiteChanged::~SuiteChanged() {
    suite_ptr suite = suite_.lock();
    if (suite.get()) {
        if (modify_change_no_ != Ecf::modify_change_no()) {
            suite->set_modify_change_no(Ecf::modify_change_no());
        }
        if (state_change_no_ != Ecf::state_change_no()) {
            suite->set_state_change_no(Ecf::state_change_no());
        }
    }
}

// ============================================================================
SuiteChanged0::SuiteChanged0(node_ptr s)
    : node_(s),
      suite_(s->suite()),
      state_change_no_(Ecf::state_change_no()),
      modify_change_no_(Ecf::modify_change_no()) {
}

SuiteChanged0::~SuiteChanged0() {
    node_ptr node = node_.lock();
    if (node.get() && suite_) {
        if (modify_change_no_ != Ecf::modify_change_no()) {
            suite_->set_modify_change_no(Ecf::modify_change_no());
            // std::cout << "SuiteChanged0::~SuiteChanged0() modify_ changed \n";
        }
        if (state_change_no_ != Ecf::state_change_no()) {
            suite_->set_state_change_no(Ecf::state_change_no());
            // std::cout << "SuiteChanged0::~SuiteChanged0() state changed \n";
        }
    }
}

// ============================================================================
SuiteChangedPtr::SuiteChangedPtr(Node* s)
    : suite_(s->suite()),
      state_change_no_(Ecf::state_change_no()),
      modify_change_no_(Ecf::modify_change_no()) {
}

SuiteChangedPtr::~SuiteChangedPtr() {
    if (suite_) {
        if (modify_change_no_ != Ecf::modify_change_no()) {
            suite_->set_modify_change_no(Ecf::modify_change_no());
            // std::cout << "SuiteChangedPtr::~SuiteChangedPtr() modify_ changed \n";
        }
        if (state_change_no_ != Ecf::state_change_no()) {
            suite_->set_state_change_no(Ecf::state_change_no());
            // std::cout << "SuiteChangedPtr::~SuiteChangedPtr() state changed \n";
        }
    }
}
//================================================================

SuiteChanged1::SuiteChanged1(Suite* s)
    : suite_(s),
      state_change_no_(Ecf::state_change_no()),
      modify_change_no_(Ecf::modify_change_no()) {
}

SuiteChanged1::~SuiteChanged1() {
    if (modify_change_no_ != Ecf::modify_change_no()) {
        suite_->set_modify_change_no(Ecf::modify_change_no());
        // std::cout << "SuiteChanged1::~SuiteChanged0() modify_ changed \n";
    }
    if (state_change_no_ != Ecf::state_change_no()) {
        suite_->set_state_change_no(Ecf::state_change_no());
        // std::cout << "SuiteChanged1::~SuiteChanged0() modify_ changed \n";
    }
}

} // namespace ecf
