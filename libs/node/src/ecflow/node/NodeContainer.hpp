// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

///
/// \brief The NodeContainer class holds families and tasks
///

#include <limits>

#include "ecflow/node/Node.hpp"

class NodeContainer : public Node {
protected:
    NodeContainer& operator=(const NodeContainer&);

public:
    explicit NodeContainer(const std::string& name, bool check);
    NodeContainer(const NodeContainer&);
    NodeContainer();
    ~NodeContainer() override;

    bool check_defaults() const override;

    void accept(ecf::NodeTreeVisitor&) override;
    void acceptVisitTraversor(ecf::NodeTreeVisitor& v) override;
    void reset() override;
    void begin() override;
    void requeue(Requeue_args&) override;
    void requeue_time_attrs() override;
    void handle_migration(const ecf::Calendar&) override;
    void reset_late_event_meters() override;
    bool run(JobsParam& jobsParam, bool force) override;
    void kill(const std::string& zombie_pid = "") override;
    void status() override;
    bool top_down_why(std::vector<std::string>& theReasonWhy, bool html_tags = false) const override;
    void collateChanges(DefsDelta&) const override;
    void set_memento(const OrderMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const ChildrenMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void order(Node* immediateChild, NOrder::Order) override;
    void move_peer(Node* src, Node* dest) override;

    bool hasAutoCancel() const override;
    bool calendarChanged(const ecf::Calendar&,
                         Node::Calendar_args&,
                         const ecf::LateAttr* inherited_late,
                         bool holding_parent_day_or_date) override;
    bool resolveDependencies(JobsParam&) override;
    bool has_time_based_attributes() const override;
    bool check(std::string& errorMsg, std::string& warningMsg) const override;
    void invalidate_trigger_references() const override;

    task_ptr add_task(const std::string& task_name);
    family_ptr add_family(const std::string& family_name);
    void addTask(const task_ptr&, size_t position = std::numeric_limits<std::size_t>::max());
    void addFamily(const family_ptr&, size_t position = std::numeric_limits<std::size_t>::max());
    void add_child(const node_ptr&, size_t position = std::numeric_limits<std::size_t>::max());

    const auto& children() const { return nodes_; }

    void immediateChildren(std::vector<node_ptr>&) const override;
    void allChildren(std::vector<node_ptr>&) const override;

    node_ptr findImmediateChild(const std::string& name, size_t& child_pos) const override;
    node_ptr find_immediate_child(const std::string_view&) const override;
    node_ptr find_node_up_the_tree(const std::string& name) const override;

    node_ptr find_relative_node(const std::vector<std::string>& pathToNode) override;
    void find_closest_matching_node(const std::vector<std::string>& pathToNode,
                                    int indexIntoPathNode,
                                    node_ptr& closest_matching_node);

    node_ptr find_by_name(const std::string& name) const;
    family_ptr findFamily(const std::string& familyName) const;
    task_ptr findTask(const std::string& taskName) const;

    std::string find_node_path(const std::string& type, const std::string& name) const override;

    [[deprecated]] const std::vector<node_ptr>& nodeVec() const { return nodes_; }
    std::vector<task_ptr> taskVec() const;
    std::vector<family_ptr> familyVec() const;

    bool operator==(const NodeContainer& rhs) const;

    bool hasTimeDependencies() const override;

    void check_job_creation(job_creation_ctrl_ptr jobCtrl) override;
    void generate_scripts(const std::map<std::string, std::string>& override) const override;

    bool checkInvariants(std::string& errorMsg) const override;
    void verification(std::string& errorMsg) const override;

    NState::State computedState(Node::TraverseType) const override;

    ///
    /// @brief Keeps, in a detached copy, only the children marked, recording no change.
    ///
    /// For copies of nodes sent to a client without the nodes it cannot read; never for the nodes of a server.
    ///
    /// @param[in] keep One flag per child, in the order of the children
    /// @throws std::logic_error when the number of flags differs from the number of children.
    ///
    void retain_children_of_copy(const std::vector<bool>& keep);

    node_ptr removeChild(Node* child) override;
    bool addChild(const node_ptr& child, size_t position = std::numeric_limits<std::size_t>::max()) override;
    bool isAddChildOk(Node* child, std::string& errorMsg) const override;

    void setRepeatToLastValueHierarchically() override;
    void setStateOnlyHierarchically(NState::State s, bool force = false) override;
    void set_state_hierarchically(NState::State s, bool force) override;
    void update_limits() override;
    void sort_attributes(ecf::Attr::Type attr,
                         bool recursive                          = true,
                         const std::vector<std::string>& no_sort = std::vector<std::string>()) override;

    bool has_archive() const override;
    void archive();

    ///
    /// @brief Identifies what triggers the restore of an archived container, for the Permissions attributes it brings.
    ///
    enum class RestoreTrigger {
        Requested, ///< An explicit restore (--restore, a plug among siblings): the restoration policy applies (D3.5,
                   ///< D3.6)
        Automatic, ///< Autorestore, or a restore during begin or requeue: the restoration policy applies (D3.5, D3.6)
        Transfer   ///< The source of a plug under another parent, already validated where it goes: restored as archived
    };

    ///
    /// @brief Restores the children of an archived container from its archive file.
    ///
    /// @param[in] remove_archive When true (the default), the archive file is removed and the restore is logged;
    ///            when false, the archive file and the log are left untouched, e.g. to restore a copy of the container
    ///            while the archive must remain for the original
    /// @param[in] trigger What triggers the restore; an explicit and an automatic restore differ only for the
    ///            Permissions attributes restored on a server without a server attribute (D3.6)
    /// @throws std::runtime_error when the container is not archived, has children, or the archive cannot be read;
    ///         and for an explicit restore of Permissions attributes on a server without a server attribute.
    ///
    /// The restoration policy of Permissions attributes (ecf::PermissionsChange::prepare_restore() and
    /// complete_restore()) applies to explicit and automatic restores of the definitions; a transfer, and a copy
    /// outside any definitions, are restored as archived.
    ///
    void restore(bool remove_archive = true, RestoreTrigger trigger = RestoreTrigger::Requested);
    void remove_archived_files();     // used in delete
    std::string archive_path() const; // can throw if ECF_HOME not defined

    boost::posix_time::time_duration sum_runtime() override;

private:
    void restore_on_begin_or_requeue();

    size_t child_position(const Node*) const override;
    void add_task_only(const task_ptr&, size_t position = std::numeric_limits<std::size_t>::max());
    void add_family_only(const family_ptr&, size_t position = std::numeric_limits<std::size_t>::max());

    void handle_defstatus_propagation();
    void match_closest_children(const std::vector<std::string>& pathToNode,
                                int indexIntoPathToNode,
                                node_ptr& closest_matching_node);

    void handleStateChange() override; // called when a state change happens

    friend class Defs;
    friend class Family;
    bool doDeleteChild(Node* child) override;

protected:
    void force_sync() override;
    void incremental_changes(DefsDelta& changes, compound_memento_ptr& comp) const;

private:
    void copy(const NodeContainer& rhs);
    void swap(NodeContainer& rhs);

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);

private:
    std::vector<node_ptr> nodes_;

protected:
    unsigned int order_state_change_no_{0};      // no need to persist
    unsigned int add_remove_state_change_no_{0}; // no need to persist
};
