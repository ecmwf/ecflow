/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_InLimit_HPP
#define ecflow_node_InLimit_HPP

#include <string>

#include "ecflow/node/LimitFwd.hpp"

namespace cereal {
class access;
}

// Inlimit. Multiple inlimits on same Node are logically ANDED
//    inlimit    limitName     // This will consume one token in the limit <limitName>
//    inlimit    limitName 10  // This will consume 10 tokens in the limit <limitName>
//    inlimit -s limitName     // Limit submission, consume one token in the limit <limitName>
//    inlimit -s limitName 10  // Limit submission, consume consume 10 tokens in the limit <limitName>
//    inlimit -n limitName     // Only applicable to a Suite/family, does not matter how many tasks
//                             // the family has, will only consume one token in the family
//                             // Can control number of active families.
//
// Inlimit of the same name specified on a task take priority over the family
class InLimit {
public:
    explicit InLimit(const std::string& limit_name, // referenced limit
                     const std::string& path_to_node_with_referenced_limit =
                         std::string(),                 // if empty, search for limit up parent hierarchy
                     int tokens                = 1,     // tokens to consume in the Limit
                     bool limit_this_node_only = false, // if true limit this node only
                     bool limit_submission     = false, // limit submission only
                     bool check                = true   // disable name checking
    );
    InLimit() = default;

    bool operator==(const InLimit& rhs) const;
    bool operator<(const InLimit& rhs) const { return n_ < rhs.name(); }

    const std::string& name() const { return n_; } // must be defined
    const std::string& pathToNode() const {
        return path_;
    } // can be empty,the node referenced by the In-Limit, this should hold the Limit.
    int tokens() const { return tokens_; }

    bool limit_submission() const { return limit_submission_; }
    bool limit_this_node_only() const { return limit_this_node_only_; }
    bool incremented() const { return incremented_; } // only used with limit_this_node_only
    void set_incremented(bool f) { incremented_ = f; }

    std::string toString() const;

public:
    void write(std::string&) const;

private:
    void limit(limit_ptr l) { limit_ = std::weak_ptr<Limit>(l); }

public:
    Limit* limit() const { return limit_.lock().get(); } // can return NULL
private:
    friend class InLimitMgr;

private:
    std::weak_ptr<Limit> limit_; // NOT persisted since computed on the fly
    std::string n_;
    std::string path_;
    int tokens_{1};
    bool limit_this_node_only_{
        false}; // default is false,if True, will consume one token(s) only, regardless of number of children
    bool limit_submission_{false}; // limit submission only
    bool incremented_{false};      // state

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

#endif /* ecflow_node_InLimit_HPP */
