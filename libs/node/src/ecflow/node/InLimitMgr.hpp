/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_InLimitMgr_HPP
#define ecflow_node_InLimitMgr_HPP

#include <memory>
#include <set>
#include <string>

#include "ecflow/node/InLimit.hpp"
#include "ecflow/node/LimitFwd.hpp"
#include "ecflow/node/NodeFwd.hpp"

namespace cereal {
class access;
}

// class InLimitMgr:
// Design notes:
// Please note: when ever we want access the inlimits, limit ptrs we
// must resolve(/compute them first). This save on client code which
// modifies the node tree from having handle it.
// If this proves to be a bottle next. We could add a caching mechanism
// base on the Ecf class,so that we need only update the pointers
// when a structural modification is made.
//
class InLimitMgr {
public:
    explicit InLimitMgr(Node* n)
        : node_(n) {}
    InLimitMgr(const InLimitMgr& rhs)
        : vec_(rhs.vec_) {}
    InLimitMgr() = default;

    // needed by node copy constructor
    void set_node(Node* n) { node_ = n; }

    // Used by begin/re-queue
    void reset(); // clear incremented flag, for family limited nodes.

    ///
    /// @brief Clear the 'incremented' flag of the inlimits that reference the given Limit.
    ///
    /// This is the counterpart of reset(), restricted to a single Limit. It is used when a Limit
    /// is reset with the nodes currently consuming it.
    ///
    /// @param[in] limit The Limit whose referencing inlimits are cleared.
    ///
    void reset_for(const Limit* limit);

    // standard functions: ==============================================
    InLimitMgr& operator=(const InLimitMgr&);
    bool operator==(const InLimitMgr& rhs) const;
    void clear() { vec_.clear(); }

    // Access functions: ======================================================
    const std::vector<InLimit>& inlimits() const { return vec_; }

    // Add functions: ===============================================================
    void addInLimit(const InLimit&, bool check = true); // will throw std::runtime_error if duplicate

    // Delete functions: can throw std::runtime_error ===================================
    // if name argument is empty, delete all attributes of that type
    // if delete was successful return true, else return false.
    // Can throw std::runtime_error if the attribute cannot be found
    bool deleteInlimit(const std::string& name);

    // mementos functions:
    void get_memento(compound_memento_ptr& comp) const;

    // Find functions: ============================================================
    /// *** This will resolve the in limits first ***
    /// Used in *test* only
    Limit* findLimitViaInLimit(const InLimit&) const;

    bool findInLimitByNameAndPath(const InLimit&) const; // use name,path,token,

    // Why:
    bool why(std::vector<std::string>& vec, bool html = false) const; // return true if why found

    // Limit functions:

    /// Are the in limits pointers to the Limits in limit.
    /// This is a very heavily used function. *******
    /// *** This will resolve the in limits first ***
    bool inLimit() const;

    ///
    /// @brief Consumes the tokens of the inlimits held by this Node, on the Limits they reference.
    ///
    /// This is called after job submission, to indicate that a resource is consumed. A Limit already
    /// present in @p limitSet is skipped, which is how an inlimit held by a task takes precedence over
    /// an inlimit of the same Limit held by an ancestor: Node::incrementInLimit() visits the task before
    /// walking up the parent hierarchy. An inlimit that limits a single node (i.e. -n) consumes its tokens
    /// at most once, and records the path of this Node rather than @p task_path.
    ///
    /// @note The inlimit references to their Limits are resolved before the update.
    ///
    /// @param[in,out] limitSet The Limits already updated during the traversal; each Limit found is
    ///                         inserted so that it is updated at most once.
    /// @param[in] task_path The path of the submitted task, recorded as consuming the Limit.
    /// @param[in] only When not nullptr, restricts the update to the given Limit; the inlimits referencing
    ///                 any other Limit are left untouched, and their Limits are not inserted into
    ///                 @p limitSet.
    ///
    void incrementInLimit(std::set<Limit*>& limitSet, const std::string& task_path, const Limit* only = nullptr);

    ///
    /// @brief Releases the tokens of the inlimits held by this Node, on the Limits they reference.
    ///
    /// This is called after a job aborts, completes, or is re-queued, to indicate that a resource is
    /// available again. The precedence rules are the ones of incrementInLimit(). An inlimit that limits
    /// a single node (i.e. -n) releases its tokens only once none of the descendant tasks is submitted
    /// or active.
    ///
    /// @note The inlimit references to their Limits are resolved before the update.
    /// @note Calling this method when no token is held is safe, and has no effect.
    ///
    /// @param[in,out] limitSet The Limits already updated during the traversal; each Limit found is
    ///                         inserted so that it is updated at most once.
    /// @param[in] task_path The path of the task giving up the token; a Limit that does not record this
    ///                      path is left unchanged.
    ///
    void decrementInLimit(std::set<Limit*>& limitSet, const std::string& task_path);

    ///
    /// @brief Releases the tokens of the inlimits held by this Node that limit submission only.
    ///
    /// This is called once a job becomes active, since an inlimit that limits submission (i.e. -s) holds
    /// its tokens only while the job is submitted. The inlimits that do not limit submission are left
    /// untouched. The precedence rules are the ones of incrementInLimit().
    ///
    /// @note The inlimit references to their Limits are resolved before the update.
    ///
    /// @param[in,out] limitSet The Limits already updated during the traversal; each Limit found is
    ///                         inserted so that it is updated at most once.
    /// @param[in] task_path The path of the task giving up the token; a Limit that does not record this
    ///                      path is left unchanged.
    /// @param[in] only When not nullptr, restricts the update to the given Limit; the inlimits referencing
    ///                 any other Limit are left untouched, and their Limits are not inserted into
    ///                 @p limitSet.
    ///
    void decrementInLimitForSubmission(std::set<Limit*>& limitSet,
                                       const std::string& task_path,
                                       const Limit* only = nullptr);

    /// Check to see if inlimit's can reference their Limits
    void check(std::string& errorMsg, std::string& warningMsg, bool reportErrors, bool reportWarnings) const;

    /// Add externs where the inlimit reference to limits cannot be resolved
    void auto_add_inlimit_externs(Defs*) const;

    /// Needed by python interface
    std::vector<InLimit>::const_iterator inlimit_begin() const { return vec_.begin(); }
    std::vector<InLimit>::const_iterator inlimit_end() const { return vec_.end(); }

private:
    /// Setup in-limits, to point to their limits,
    void resolveInLimitReferences() const;
    void resolveInLimit(InLimit&,
                        std::string& errorMsg,
                        std::string& warningMsg,
                        bool reportErrors,
                        bool reportWarnings) const;
    void resolveInLimit(InLimit&) const;

    limit_ptr find_limit(const InLimit&,
                         std::string& errorMsg,
                         std::string& warningMsg,
                         bool reportErrors,
                         bool reportWarnings) const;

private:
    Node* node_{nullptr}; // Not persisted, constructor will always set this up.

    mutable std::vector<InLimit> vec_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

#endif /* ecflow_node_InLimitMgr_HPP */
