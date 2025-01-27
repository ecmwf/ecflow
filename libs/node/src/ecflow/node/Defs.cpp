/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Defs.hpp"

#include <cassert>
#include <limits>
#include <stdexcept>

#include "ecflow/core/CalendarUpdateParams.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Indentor.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/NodePath.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/StringSplitter.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/AbstractObserver.hpp"
#include "ecflow/node/DefsDelta.hpp"
#include "ecflow/node/ExprDuplicate.hpp"
#include "ecflow/node/JobCreationCtrl.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/NodeState.hpp"
#include "ecflow/node/NodeStats.hpp"
#include "ecflow/node/NodeTreeVisitor.hpp"
#include "ecflow/node/ResolveExternsVisitor.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/move_peer.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp" /// The reason why Parser code moved into Defs, avoid cyclic dependency

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace ecf;
using namespace std;

// #define DEBUG_JOB_SUBMISSION 1
// #define DEBUG_MEMENTO 1

Defs::Defs() = default;

Defs::Defs(const std::string& port) : server_(port) {
}

Defs::Defs(const Defs& rhs) : state_(rhs.state_), server_(rhs.server_), flag_(rhs.flag_), client_suite_mgr_(this) {
    size_t theSize = rhs.suiteVec_.size();
    for (size_t s = 0; s < theSize; s++) {
        suite_ptr suite_copy = std::make_shared<Suite>(*rhs.suiteVec_[s]);
        suite_copy->set_defs(this);
        suiteVec_.push_back(suite_copy);
    }

    // edit history is not copied
    // externs not copied
    // observers not copied
}

void Defs::copy_defs_state_only(const defs_ptr& server_defs) {
    if (!server_defs)
        return;

    // Initialise the defs state. We need to reflect the real state.
    set_state(server_defs->state());

    // initialise flag
    flag_ = server_defs->get_flag();

    // Initialise the server state
    set_server().set_state(server_defs->server().get_state());
    set_server().set_user_variables(server_defs->server().user_variables());
    set_server().set_server_variables(server_defs->server().server_variables());
}

Defs& Defs::operator=(const Defs& rhs) {
    if (this != &rhs) {
        Defs tmp(rhs); // does *NOT* use Suite::operator=(const Suite& rhs), we use copy/swap

        std::swap(state_, tmp.state_);
        std::swap(server_, tmp.server_);
        std::swap(suiteVec_, tmp.suiteVec_);
        std::swap(flag_, tmp.flag_);

        // edit history is not copied
        // externs not copied
        // observers not copied

        size_t vec_size = suiteVec_.size();
        for (size_t i = 0; i < vec_size; i++) {
            suiteVec_[i]->set_defs(this);
        }

        modify_change_no_ = Ecf::incr_modify_change_no();
    }
    return *this;
}

defs_ptr Defs::create() {
    return std::make_shared<Defs>();
}
defs_ptr Defs::create(const std::string& port) {
    return std::make_shared<Defs>(port);
} // Defs::create(port)

Defs::~Defs() {
    //    cout << "   Deleting defs "\n";
    if (!Ecf::server()) {
        notify_delete();
    }

    // Duplicate AST are held in a static map. Delete them, to avoid valgrind complaining
    ExprDuplicate reclaim_cloned_ast_memory;
}

void Defs::handle_migration() {
    // Fix up any migration issues. Remove when in Bolgna, and ecflow4 no longer used
    for (const auto& s : suiteVec_) {
        s->handle_migration(s->calendar());
    }

    // remove any edit history that is no longer referenced. Ignore root, which will not be found
    auto it = edit_history_.begin();
    while (it != edit_history_.end()) {

        if ((*it).first == Str::ROOT_PATH()) {
            it++;
            continue; // root path is defs, which is not a node, hence ignore
        }

        node_ptr node = findAbsNode((*it).first);
        if (!node.get()) {
            it = edit_history_.erase(it);
        }
        else
            it++;
    }
}

///// State relation functions: ==================================================
NState::State Defs::state() const {
    return state_.state();
}

void Defs::set_state_only(NState::State the_new_state) {
    state_.setState(the_new_state); // this will update state_change_no
}

void Defs::set_state(NState::State the_new_state) {
    set_state_only(the_new_state); // this will update state_change_no

    // Log the state change
    //           " " +  submitted(max) + ": /"
    // reserve : 1   +  9              + 3      = 13
    std::string log_state_change;
    log_state_change.reserve(13);
    log_state_change += " ";
    log_state_change += NState::toString(the_new_state);
    log_state_change += ": /";
    ecf::log(Log::LOG, log_state_change);
}

void Defs::set_most_significant_state() {
    NState::State computedStateOfImmediateChildren =
        ecf::theComputedNodeState(suiteVec_, true /* immediate children only */);
    if (computedStateOfImmediateChildren != state_.state())
        set_state(computedStateOfImmediateChildren);
}

/// Others ======================================================================
void Defs::check_job_creation(job_creation_ctrl_ptr jobCtrl) {
    /// Job generation checking. is done via the python API
    /// As such it done directly on the Defs.
    /// However Job generation checking will end up changing the states of the DEFS
    /// If this defs is loaded into the server the state of each node may be surprising. (i.e submitted)
    /// Hence we need to reset the state.
    if (!jobCtrl.get())
        throw std::runtime_error("Defs::check_job_creation: NULL JobCreationCtrl passed");

    if (jobCtrl->verbose())
        cout << "Defs::check_job_creation(verbose):\n";

    // This function should NOT really change the data model
    // The changed state is reset, hence we need to preserve change and modify numbers
    EcfPreserveChangeNo preserveChangeNo;

    if (jobCtrl->node_path().empty()) {

        size_t theSize = suiteVec_.size();
        for (size_t s = 0; s < theSize; s++) {
            /// begin will cause creation of generated variables. The generated variables
            /// are use in client scripts and used to locate the ecf files
            suiteVec_[s]->begin();
            suiteVec_[s]->check_job_creation(jobCtrl);

            /// reset the state
            suiteVec_[s]->reset(); // will reset begin
            suiteVec_[s]->setStateOnlyHierarchically(NState::UNKNOWN);
            set_most_significant_state();
        }
    }
    else {

        node_ptr node = findAbsNode(jobCtrl->node_path());
        if (node.get()) {
            /// begin will cause creation of generated variables. The generated variables
            /// are use in client scripts and used to locate the ecf files
            node->suite()->begin();
            node->check_job_creation(jobCtrl);

            /// reset the state
            node->reset();
            node->suite()->reset_begin();
            node->setStateOnlyHierarchically(NState::UNKNOWN);
        }
        else {
            std::stringstream ss;
            ss << "Defs::check_job_creation: failed as node path '" << jobCtrl->node_path() << "' does not exist.\n";
            jobCtrl->error_msg() = ss.str();
        }
    }
}

void Defs::do_generate_scripts(const std::map<std::string, std::string>& override) const {
    size_t theSize = suiteVec_.size();
    for (size_t s = 0; s < theSize; s++) {
        suiteVec_[s]->generate_scripts(override);
    }
}
void Defs::generate_scripts() const {
    std::map<std::string, std::string> override;
    do_generate_scripts(override);
}

static void remove_autocancelled(const std::vector<node_ptr>& auto_cancelled_nodes) {
    // Permanently remove any auto-cancelled nodes.
    if (!auto_cancelled_nodes.empty()) {
        auto theNodeEnd = auto_cancelled_nodes.end();
        string msg;
        for (auto n = auto_cancelled_nodes.begin(); n != theNodeEnd; ++n) {
            // If we have two autocancel in the hierarchy, with same attributes. Then
            // (*n)->remove() on the second will fail( with a crash, SuiteChanged0 destructor,  no suite pointer)
            // since it would already be detached. See ECFLOW-556
            // By checking we can still reach the Defs we know we are not detached
            if ((*n)->defs()) {
                msg.clear();
                msg = "autocancel ";
                msg += (*n)->debugNodePath();
                ecf::log(Log::MSG, msg);
                (*n)->remove();
            }
        }
    }
}

static void auto_archive(const std::vector<node_ptr>& auto_archive_nodes) {
    if (!auto_archive_nodes.empty()) {
        auto theNodeEnd = auto_archive_nodes.end();
        for (auto n = auto_archive_nodes.begin(); n != theNodeEnd; ++n) {
            // If we have two auto archive in the hierarchy, with same attributes. Then
            // By checking we can still reach the Defs we know we are not detached
            NodeContainer* nc = (*n)->isNodeContainer();
            if (nc && nc->defs()) {
                nc->archive();
            }
        }
    }
}

void Defs::updateCalendar(const ecf::CalendarUpdateParams& calUpdateParams) {
    // Collate any auto cancelled nodes as a result of calendar update
    // Collate any auto archive nodes as a result of calendar update
    Node::Calendar_args cal_args;

    // updateCalendarCount_ is only used in *test*
    updateCalendarCount_++;

    size_t theSize = suiteVec_.size();
    for (size_t s = 0; s < theSize; s++) {
        suiteVec_[s]->updateCalendar(calUpdateParams, cal_args);
    }

    // Permanently remove any auto-cancelled nodes.
    remove_autocancelled(cal_args.auto_cancelled_nodes_);

    // Archive any nodes with auto archive attribute, Must be suite/family
    auto_archive(cal_args.auto_archive_nodes_);
}

void Defs::update_calendar(Suite* suite, const ecf::CalendarUpdateParams& cal_update_params) {
    // Collate any auto cancelled nodes as a result of calendar update
    // Collate any auto archive nodes as a result of calendar update
    Node::Calendar_args cal_args;

    suite->updateCalendar(cal_update_params, cal_args);

    // Permanently remove any auto-cancelled nodes.
    remove_autocancelled(cal_args.auto_cancelled_nodes_);

    // Archive any nodes with auto archive attribute, Must be suite/family
    auto_archive(cal_args.auto_archive_nodes_);
}

bool Defs::catch_up_to_real_time() {
    auto time_now = Calendar::second_clock_time();

    //   cout << "Time Now: " << to_simple_string(time_now) << "\n";
    //   for(const auto& suite : suiteVec_)  cout << suite->calendar().toString() << "\n";
    //   cout << "===================================================================\n";
    bool updated = false;
    time_duration schedule_increment(0, 0, server_.jobSubmissionInterval(), 0);
    for (const auto& suite : suiteVec_) {
        // Check if suite has time,today,date,day,cron,late,autocancel,autoarchive attributes
        if (suite->has_time_based_attributes()) {
            auto suite_time = suite->calendar().suiteTime();
            if (time_now - suite_time <= hours(1)) {
                suite_time += schedule_increment;
                while (suite_time <= time_now) {

                    update_calendar(suite.get(), ecf::CalendarUpdateParams(suite_time, schedule_increment, true));

                    // cout << suite->name() << " : " << suite->calendar().toString() << "\n";
                    suite_time += schedule_increment;
                    updated = true;
                }
            }
        }
    }
    //   cout << "===================================================================\n";
    //   for(const auto& suite : suiteVec_)  cout << suite->calendar().toString() << "\n";
    return updated;
}

void Defs::absorb(Defs* input_defs, bool force) {
    // Dont absorb myself.
    if (input_defs == this) {
        return;
    }

    // updateCalendarCount_ is *only* used in test, reset whenever a new defs is loaded
    updateCalendarCount_ = 0;

    // We must make a copy, otherwise we are iterating over a vector that is being deleted
    std::vector<suite_ptr> suiteVecCopy = input_defs->suiteVec();
    size_t theSize                      = suiteVecCopy.size();
    for (size_t s = 0; s < theSize; s++) {

        /// regardless remove the suite from the input defs
        suite_ptr the_input_suite = input_defs->removeSuite(suiteVecCopy[s]);

        if (force) {
            /// The suite of the same name exists. remove it from *existing* defs
            suite_ptr existing_suite = findSuite(the_input_suite->name());
            if (existing_suite.get()) {
                removeSuite(existing_suite);
            }
        }

        /// Add the suite. Will throw if suite of same name already exists.
        /// This stops accidental overwrite
        addSuite(the_input_suite);
    }
    LOG_ASSERT(input_defs->suiteVec().empty(), "Defs::absorb");

    // Copy over server user variables
    set_server().add_or_update_user_variables(input_defs->server().user_variables());

    // This only works on the client side. since server does not store externs
    const set<string>& ex = input_defs->externs();
    for (const auto& i : ex) {
        add_extern(i);
    }
}

void Defs::accept(ecf::NodeTreeVisitor& v) {
    v.visitDefs(this);
    size_t theSuiteVecSize = suiteVec_.size();
    for (size_t i = 0; i < theSuiteVecSize; i++) {
        suiteVec_[i]->accept(v);
    }
}

void Defs::acceptVisitTraversor(ecf::NodeTreeVisitor& v) {
    LOG_ASSERT(v.traverseObjectStructureViaVisitors(), "");
    v.visitDefs(this);
}

bool Defs::verification(std::string& errorMsg) const {
    size_t theSuiteVecSize = suiteVec_.size();
    for (size_t i = 0; i < theSuiteVecSize; i++) {
        suiteVec_[i]->verification(errorMsg);
    }
    return errorMsg.empty();
}

suite_ptr Defs::add_suite(const std::string& name) {
    if (findSuite(name).get()) {
        std::stringstream ss;
        ss << "Add Suite failed: A Suite of name '" << name << "' already exists";
        throw std::runtime_error(ss.str());
    }
    suite_ptr s = Suite::create(name);

    if (s->defs()) {
        std::stringstream ss;
        ss << "Place Suite failed: The suite of name '" << s->name() << "' already owned by another Defs ";
        throw std::runtime_error(ss.str());
    }

    insert_suite(s, std::numeric_limits<std::size_t>::max());

    Ecf::incr_modify_change_no();
    client_suite_mgr_.suite_added_in_defs(s);

    return s;
}

void Defs::addSuite(const suite_ptr& s, size_t position) {
    if (findSuite(s->name()).get()) {
        std::stringstream ss;
        ss << "Add Suite failed: A Suite of name '" << s->name() << "' already exists";
        throw std::runtime_error(ss.str());
    }

    if (s->defs()) {
        std::stringstream ss;
        ss << "Place Suite failed: The suite of name '" << s->name() << "' already owned by another Defs ";
        throw std::runtime_error(ss.str());
    }

    insert_suite(s, position);

    Ecf::incr_modify_change_no();
    client_suite_mgr_.suite_added_in_defs(s);
}

void Defs::placeSuite(const suite_ptr& s, size_t position) {
    if (findSuite(s->name()).get()) {
        std::stringstream ss;
        ss << "Place Suite failed: A Suite of name '" << s->name() << "' already exists";
        throw std::runtime_error(ss.str());
    }

    if (s->defs()) {
        std::stringstream ss;
        ss << "Place Suite failed: The suite of name '" << s->name() << "' already owned by another Defs ";
        throw std::runtime_error(ss.str());
    }

    insert_suite(s, position);

    Ecf::incr_modify_change_no();
    client_suite_mgr_.suite_replaced_in_defs(s);
}

void Defs::insert_suite(const suite_ptr& s, size_t position) {
    assert(!s->defs()); // the suite to be inserted should still not have an associated defs

    s->set_defs(this);
    if (position >= suiteVec_.size()) {
        suiteVec_.push_back(s);
    }
    else {
        suiteVec_.insert(suiteVec_.begin() + position, s);
    }
}

suite_ptr Defs::removeSuite(suite_ptr s) {
    auto i = std::find(suiteVec_.begin(), suiteVec_.end(), s);
    if (i != suiteVec_.end()) {
        s->set_defs(nullptr); // allows suite to added to different defs
        suiteVec_.erase(i);   // iterator invalidated
        Ecf::incr_modify_change_no();
        client_suite_mgr_.suite_deleted_in_defs(s); // must be after Ecf::incr_modify_change_no();
        return s;                                   // transfer ownership of suite
    }

    // Something serious has gone wrong. Cannot find the suite
    cout << "Defs::removeSuite: assert failure:  suite '" << s->name() << "' suiteVec_.size() = " << suiteVec_.size()
         << "\n";
    for (unsigned si = 0; si < suiteVec_.size(); ++si) {
        cout << si << " " << suiteVec_[si]->name() << "\n";
    }
    LOG_ASSERT(false, "Defs::removeSuite the suite not found");
    return suite_ptr();
}

size_t Defs::child_position(const Node* child) const {
    size_t vecSize = suiteVec_.size();
    for (size_t t = 0; t < vecSize; t++) {
        if (suiteVec_[t].get() == child) {
            return t;
        }
    }
    return std::numeric_limits<std::size_t>::max();
}

node_ptr Defs::removeChild(Node* child) {
    size_t vecSize = suiteVec_.size();
    for (size_t t = 0; t < vecSize; t++) {
        if (suiteVec_[t].get() == child) {
            Ecf::incr_modify_change_no();
            suiteVec_[t]->set_defs(nullptr); // Must be set to NULL, allows suite to be added to different defs
            client_suite_mgr_.suite_deleted_in_defs(suiteVec_[t]); // must be after Ecf::incr_modify_change_no();
            node_ptr node = std::dynamic_pointer_cast<Node>(suiteVec_[t]);
            suiteVec_.erase(suiteVec_.begin() + t);
            return node;
        }
    }

    // Something has gone wrong.
    cout << "Defs::removeChild: assert failed:  suite '" << child->name() << "' suiteVec_.size() = " << suiteVec_.size()
         << "\n";
    for (unsigned i = 0; i < suiteVec_.size(); ++i) {
        cout << i << " " << suiteVec_[i]->name() << "\n";
    }
    LOG_ASSERT(false, "Defs::removeChild,the suite not found");
    return node_ptr();
}

bool Defs::addChild(const node_ptr& child, size_t position) {
    LOG_ASSERT(child.get(), "");
    LOG_ASSERT(child->isSuite(), "");

    // *** CANT construct shared_ptr from a raw pointer, must use dynamic_pointer_cast,
    // *** otherwise the reference counts will get messed up.
    // If the suite of the same exists, it is deleted first
    addSuite(std::dynamic_pointer_cast<Suite>(child), position);
    return true;
}

bool Defs::placeChild(const node_ptr& child, size_t position) {
    LOG_ASSERT(child.get(), "");
    LOG_ASSERT(child->isSuite(), "");

    // *** CANT construct shared_ptr from a raw pointer, must use dynamic_pointer_cast,
    // *** otherwise the reference counts will get messed up.
    // If the suite of the same exists, it is deleted first
    placeSuite(std::dynamic_pointer_cast<Suite>(child), position);
    return true;
}

void Defs::add_extern(const std::string& ex) {
    if (ex.empty()) {
        throw std::runtime_error("Defs::add_extern: Cannot add empty extern");
    }
    externs_.insert(ex);
    // auto result = externs_.insert(ex);
    // cout << "Defs::add_extern " << ex << " result " << result.second << "\n";
}

void Defs::auto_add_externs(bool remove_existing_externs_first) {
    // cout << "\nDefs::auto_add_externs START EXTERNS size: " << externs_.size() << "
    // ==================================================== \n";
    if (remove_existing_externs_first) {
        externs_.clear();
    }
    /// Automatically add externs
    ResolveExternsVisitor visitor(this);
    acceptVisitTraversor(visitor);
    // cout << "Defs::auto_add_externs END EXTERNS size : " << externs_.size() << "
    // ==================================================== \n";
}

void Defs::beginSuite(const suite_ptr& suite) {
    if (!suite.get())
        throw std::runtime_error("Defs::beginSuite: Begin failed as suite is not loaded");

    if (!suite->begun()) {
        // Hierarchical set the state. Handle case where we have children that are all defstatus complete
        // and hence needs parent set to complete. See Simulator/good_defs/operations/naw.def
        //	  family naw
        //	    family general
        //	      time 06:00
        //	      task metgrams
        //	        defstatus complete
        //	      task equipot
        //	        defstatus complete
        //	    endfamily
        suite->begin();

        set_most_significant_state();
    }
    else {
        LOG(Log::WAR, "Suite " << suite->name() << " has already begun");
    }
}

void Defs::beginAll() {
    bool at_least_one_suite_begun = false;
    size_t theSuiteVecSize        = suiteVec_.size();
    for (size_t s = 0; s < theSuiteVecSize; s++) {
        if (!suiteVec_[s]->begun()) {
            suiteVec_[s]->begin();
            at_least_one_suite_begun = true;
        }
    }

    if (at_least_one_suite_begun) {
        set_most_significant_state();
    }
}

void Defs::reset_begin() {
    std::for_each(suiteVec_.begin(), suiteVec_.end(), [](suite_ptr s) { s->reset_begin(); });
}

void Defs::requeue() {
    bool edit_history_set = flag().is_set(ecf::Flag::MESSAGE);
    flag_.reset();
    if (edit_history_set)
        flag().set(ecf::Flag::MESSAGE);

    Node::Requeue_args args;
    size_t theSuiteVecSize = suiteVec_.size();
    for (size_t s = 0; s < theSuiteVecSize; s++) {
        suiteVec_[s]->requeue(args);
    }

    set_most_significant_state();
}

void Defs::sort_attributes(ecf::Attr::Type attr, bool recursive, const std::vector<std::string>& no_sort) {
    if (attr == ecf::Attr::VARIABLE || attr == ecf::Attr::ALL)
        server_.sort_variables();

    if (recursive) {
        size_t theSuiteVecSize = suiteVec_.size();
        for (size_t s = 0; s < theSuiteVecSize; s++) {
            SuiteChanged changed(suiteVec_[s]);
            suiteVec_[s]->sort_attributes(attr, recursive, no_sort);
        }
    }
}

void Defs::check_suite_can_begin(const suite_ptr& suite) const {
    NState::State suiteState = suite->state();
    if (!suite->begun() && suiteState != NState::UNKNOWN && suiteState != NState::COMPLETE) {
        int count = 0;
        std::vector<Task*> tasks;
        getAllTasks(tasks);
        std::stringstream ts;
        for (auto& task : tasks) {
            if (task->state() == NState::ACTIVE || task->state() == NState::SUBMITTED) {
                ts << "   " << task->absNodePath() << "\n";
                count++;
            }
        }
        /// allow suite to begin even its aborted provide no tasks in active or submitted states
        if (count > 0) {
            std::stringstream ss;
            ss << "Begin failed as suite " << suite->name() << "(computed state=" << NState::toString(suiteState)
               << ") can only begin if its in UNKNOWN or COMPLETE state\n";
            ss << "Found " << count << " tasks with state 'active' or 'submitted'\n";
            ss << ts.str();
            ss << "Use the force argument to bypass this check, at the risk of creating zombies\n";
            throw std::runtime_error(ss.str());
        }
    }
}

bool Defs::hasTimeDependencies() const {
    size_t theSuiteVecSize = suiteVec_.size();
    for (size_t s = 0; s < theSuiteVecSize; s++) {
        if (suiteVec_[s]->hasTimeDependencies())
            return true;
    }
    return false;
}

std::string Defs::print(PrintStyle::Type_t t) const {
    PrintStyle style(t);
    std::string s;
    print(s);
    return s;
}

void Defs::print(std::string& os) const {
    // cout << "Defs::print(start) print_cache_ " << print_cache_ << "\n";
    os.clear();
    if (print_cache_ != 0)
        os.reserve(print_cache_);
    else
        os.reserve(4096);
    os += "#";
    os += ecf::Version::raw();
    os += "\n";

    if (!PrintStyle::defsStyle())
        write_state(os);

    if (PrintStyle::getStyle() == PrintStyle::STATE) {
        os += "# server state: ";
        os += SState::to_string(server().get_state());
        os += "\n";
    }

    // In PrintStyle::MIGRATE we do NOT persist the externs. (+matches boost serialisation)
    if (!PrintStyle::persist_style()) {
        auto extern_end = externs_.end();
        for (auto i = externs_.begin(); i != extern_end; ++i) {
            os += "extern ";
            os += *i;
            os += "\n";
        }
    }

    for (const auto& s : suiteVec_) {
        s->print(os);
    }

    os += "# enddef\n"; // ECFLOW-1227 so user knows there was no truncation
    print_cache_ = os.size();
    // cout << "Defs::print print_cache_ " << print_cache_ << "\n";
}

void Defs::write_state(std::string& os) const {
    // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
    //             multiple statement on a single line i.e.
    //                 task a; task b;
    // *IMPORTANT* make sure name are unique, i.e can't have state: and server_state:
    // Otherwise read_state() will mess up
    os += "defs_state ";
    os += PrintStyle::to_string();

    if (state_ != NState::UNKNOWN) {
        os += " state>:";
        os += NState::toString(state_);
    } // make <state> is unique
    if (flag_.flag() != 0) {
        os += " flag:";
        flag_.write(os);
    }
    if (state_change_no_ != 0) {
        os += " state_change:";
        os += ecf::convert_to<std::string>(state_change_no_);
    }
    if (modify_change_no_ != 0) {
        os += " modify_change:";
        os += ecf::convert_to<std::string>(modify_change_no_);
    }
    if (server().get_state() != ServerState::default_state()) {
        os += " server_state:";
        os += SState::to_string(server().get_state());
    }

    // This only works when the full defs is requested, otherwise zero as defs is fabricated for handles
    os += " cal_count:";
    os += ecf::convert_to<std::string>(updateCalendarCount_);
    os += "\n";

    // This read by the DefsParser
    const std::vector<Variable>& server_user_variables = server().user_variables();
    size_t the_size                                    = server_user_variables.size();
    for (size_t i = 0; i < the_size; ++i)
        server_user_variables[i].print(os);

    const std::vector<Variable>& server_variables = server().server_variables();
    the_size                                      = server_variables.size();
    for (size_t i = 0; i < the_size; ++i)
        server_variables[i].print_server_variable(os); // edit var value # server

    // READ by Defs::read_history()
    // We need to define a separator for the message, will to allow it to be re-read
    // This separator cannot be :
    // ' ' space, used in the messages
    // %  Used in job submission
    // :  Used in time, and name (:ma0)
    // [] Used in time
    // integers used in the time.
    // -  Used in commands
    if (save_edit_history_) {
        Indentor in;
        for (const auto& i : edit_history_) {
            Indentor::indent(os);
            os += "history ";
            os += i.first;
            os += " ";                                      // node path
            const std::vector<std::string>& vec = i.second; // list of requests
            for (const auto& c : vec) {

                // We expect to output a single newline, hence if there are additional new lines
                // It can mess  up, re-parse. i.e during alter change label/value, user could have added newlines
                if (c.find("\n") == std::string::npos) {
                    os += "\b";
                    os += c;
                }
                else {
                    std::string h = c;
                    Str::replaceall(h, "\n", "\\n");
                    os += "\b";
                    os += h;
                }
            }
            os += "\n";
        }
        save_edit_history_ = false;
    }
}

std::string Defs::dump_edit_history() const {
    std::stringstream os;
    for (const auto& i : edit_history_) {
        os << "history ";
        os << i.first;
        os << "  ";                                     // node path
        const std::vector<std::string>& vec = i.second; // list of requests
        for (const auto& c : vec) {

            // We expect to output a single newline, hence if there are additional new lines
            // It can mess  up, re-parse. i.e during alter change label/value, user could have added newlines
            if (c.find("\n") == std::string::npos) {
                os << " ";
                os << c;
            }
            else {
                std::string h = c;
                Str::replaceall(h, "\n", "\\n");
                os << " ";
                os << h;
            }
        }
        os << "\n";
    }
    return os.str();
}

void Defs::read_state(const std::string& line, const std::vector<std::string>& lineTokens) {
    //   cout << "line = " << line << "\n";
    std::string token;
    size_t line_tokens_size = lineTokens.size();
    for (size_t i = 2; i < line_tokens_size; i++) {
        token.clear();
        const std::string& line_token_i = lineTokens[i];
        if (line_token_i.find("state>:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Defs::read_state: state extraction failed : " + line_token_i);
            std::pair<NState::State, bool> state_pair = NState::to_state(token);
            if (!state_pair.second)
                throw std::runtime_error("Defs::read_state: Invalid state specified : " + token);
            set_state_only(state_pair.first);
        }
        else if (line_token_i.find("flag:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Defs::read_state: Invalid flag specified : " + line);
            flag().set_flag(token); // this can throw
        }
        else if (line_token_i.find("state_change:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Defs::read_state: Invalid state_change specified : " + line);
            int sc = Extract::theInt(token, "Defs::read_state: invalid state_change specified : " + line);
            set_state_change_no(sc);
        }
        else if (line_token_i.find("modify_change:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Defs::read_state: Invalid modify_change specified : " + line);
            int mc = Extract::theInt(token, "Defs::read_state: invalid state_change specified : " + line);
            set_modify_change_no(mc);
        }
        else if (line_token_i.find("server_state:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Defs::read_state: Invalid server_state specified : " + line);
            if (!SState::isValid(token))
                throw std::runtime_error("Defs::read_state: Invalid server_state specified : " + line);
            set_server().set_state(SState::toState(token));
        }
        else if (line_token_i.find("cal_count:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Defs::read_state: Invalid cal_count specified : " + line);
            updateCalendarCount_ = Extract::theInt(token, "Defs::read_state: invalid cal_count specified : " + line);
        }
    }
}

void Defs::read_history(const std::string& line, const std::vector<std::string>& lineTokens) {
    // expect:
    // history <node_path> \bmsg1\bmsg2
    // The message can contain spaces,
    // Multiple spaces will be lost !!
    // Edit history older than ecf_prune_node_log_ days, will be pruned.
    if (lineTokens.size() < 2)
        throw std::runtime_error("Defs::read_history: Invalid history " + line);

    DefsHistoryParser parser;
    parser.parse(line);

    const std::vector<std::string>& parsed_messages = parser.parsed_messages();
    if (ecf_prune_node_log_ == 0) {
        for (const auto& parsed_message : parsed_messages) {
            add_edit_history(lineTokens[1], parsed_message);
        }
    }
    else {
        std::vector<std::string> vec;
        date todays_date_in_utc = day_clock::universal_day();

        for (const auto& parsed_message : parsed_messages) {
            // extract the date, expecting MSG:[HH:MM:SS D.M.YYYY]
            if (parsed_message.find("MSG:[") == 0) {

                size_t space_pos = parsed_message.find(" ");
                size_t close_p   = parsed_message.find("]");
                std::string date = parsed_message.substr(space_pos + 1, close_p - space_pos - 1);
                vec.clear();
                Str::split(date, vec, ".");
                if (vec.size() == 3) {
                    try {
                        int day   = ecf::convert_to<int>(vec[0]);
                        int month = ecf::convert_to<int>(vec[1]);
                        int year  = ecf::convert_to<int>(vec[2]);

                        boost::gregorian::date node_log_date(year, month, day);
                        boost::gregorian::date_duration duration = todays_date_in_utc - node_log_date;
                        if (duration.days() > ecf_prune_node_log_) {
                            continue;
                        }
                    }
                    catch (...) {
                    }
                }
            }
            add_edit_history(lineTokens[1], parsed_message);
        }
    }
}

bool Defs::compare_edit_history(const Defs& rhs) const {
    if (edit_history_ != rhs.edit_history_)
        return false;
    return true;
}

bool Defs::compare_change_no(const Defs& rhs) const {
    if (state_change_no_ != rhs.state_change_no_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Defs::compare_change_no: state_change_no_(" << state_change_no_
                      << ") != rhs.state_change_no_(" << rhs.state_change_no_ << ")\n";
        }
#endif
        return false;
    }
    if (modify_change_no_ != rhs.modify_change_no_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Defs::compare_change_no: modify_change_no_(" << modify_change_no_
                      << ") != rhs.modify_change_no_(" << rhs.modify_change_no_ << ")\n";
        }
#endif
        return false;
    }
    return true;
}

bool Defs::operator==(const Defs& rhs) const {
    if (state() != rhs.state()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Defs::operator==  state(" << NState::toString(state()) << ") != rhs.state("
                      << NState::toString(rhs.state()) << ")) \n";
        }
#endif
        return false;
    }

    if (server_ != rhs.server()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Defs::operator== server_ != rhs.server())\n";
        }
#endif
        return false;
    }

    if (flag_ != rhs.flag_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Defs::operator== ( flag_ != rhs.flag_) : '" << flag_.to_string() << "' != '"
                      << rhs.flag_.to_string() << "'\n";
        }
#endif
        return false;
    }

    /// Note:: WE specifically exclude testing of externs.
    /// Externs are not persisted, hence cannot take part in comparison
    /// Externs only live on the client side.

    if (suiteVec_.size() != rhs.suiteVec_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Defs::operator==    suiteVec_.size(" << suiteVec_.size() << ") != rhs.suiteVec_.size( "
                      << rhs.suiteVec_.size() << ") \n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < suiteVec_.size(); ++i) {
        if (!(*(suiteVec_[i]) == *(rhs.suiteVec_[i]))) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Defs::operator==    !( *(suiteVec_[i]) == *(rhs.suiteVec_[i]) )\n";
            }
#endif
            return false;
        }
    }
    return true;
}

node_ptr Defs::findAbsNode(const std::string& pathToNode) const {
    //	std::cout << "Defs::findAbsNode " << pathToNode << "\n";
    // The pathToNode is of the form:
    //     /suite
    //     /suite/family
    //     /suite/family/task
    //     /suite/family/family/family/task
    // This is 14% quicker than the previous algorithm, that split 'pathToNode' into a vector of strings first.
    node_ptr ret;
    bool first = false;
    StringSplitter string_splitter(pathToNode, Str::PATH_SEPERATOR());
    while (!string_splitter.finished()) {
        std::string_view path_token = string_splitter.next();
        // std::cout << "path_token:'" << path_token << "'  last = " << string_splitter.last() << "\n";
        if (!first) {
            for (const auto& suite : suiteVec_) {
                if (path_token == suite->name()) {
                    ret = suite;
                    if (string_splitter.last()) {
                        // cout << "finished returning " << ret->absNodePath() << " *last* suite found\n";
                        return ret;
                    }
                    break;
                }
            }
            if (!ret) {
                // cout << "finished returning suite not found\n";
                return node_ptr();
            }
            first = true;
        }
        else {
            // cout << "seraching from " << ret->absNodePath() << " for " << ref << "\n";
            ret = ret->find_immediate_child(path_token);
            if (ret) {
                if (string_splitter.last()) {
                    // cout << "finished returning " << ret->absNodePath() << " *last* \n";
                    return ret;
                }
                continue;
            }
            // cout << "finished returning node not found\n";
            return node_ptr();
        }
    }
    // cout << "finished returning node not found, end of loop\n";
    return node_ptr();

    // OLD code, slower. Since we are creating a vector of strings
    //	std::vector<std::string> theNodeNames; theNodeNames.reserve(4);
    //	NodePath::split(pathToNode,theNodeNames);
    //	if ( theNodeNames.empty() ) {
    //		return node_ptr();
    //	}
    //	size_t child_pos = 0 ; // unused
    //	size_t pathSize = theNodeNames.size();
    //	size_t theSuiteVecSize = suiteVec_.size();
    //	for(size_t s = 0; s < theSuiteVecSize; s++)  {
    //		size_t index = 0;
    //		if (theNodeNames[index] == suiteVec_[s]->name()) {
    //			node_ptr the_node = suiteVec_[s];
    //			if (pathSize == 1) return the_node;
    //			index++; // skip over suite,
    //			while (index < pathSize) {
    //				the_node = the_node->findImmediateChild(theNodeNames[index],child_pos);
    //				if (the_node) {
    //					if (index == pathSize - 1) return the_node;
    //					index++;
    //				}
    //				else {
    //					return node_ptr();
    //				}
    //			}
    //			return node_ptr();
    //		}
    //	}
    //	return node_ptr();
}

node_ptr Defs::find_closest_matching_node(const std::string& pathToNode) const {
    std::vector<std::string> theNodeNames;
    NodePath::split(pathToNode, theNodeNames);
    if (theNodeNames.empty())
        return node_ptr();

    node_ptr closest_matching_node;
    int index              = 0;
    size_t theSuiteVecSize = suiteVec_.size();
    for (size_t s = 0; s < theSuiteVecSize; s++) {
        suiteVec_[s]->find_closest_matching_node(theNodeNames, index, closest_matching_node);
        if (closest_matching_node.get())
            return closest_matching_node;
    }
    return node_ptr();
}

bool Defs::find_extern(const std::string& pathToNode, const std::string& node_attr_name) const {
    if (externs_.empty()) {
        return false;
    }

    if (node_attr_name.empty()) {
        if (externs_.find(pathToNode) != externs_.end()) {
            return true;
        }
        return false;
    }

    std::string extern_path = pathToNode;
    extern_path += Str::COLON();
    extern_path += node_attr_name;

    if (externs_.find(extern_path) != externs_.end()) {
        return true;
    }
    return false;
}

suite_ptr Defs::findSuite(const std::string& name) const {
    for (const auto& s : suiteVec_) {
        if (s->name() == name) {
            return s;
        }
    }
    return suite_ptr();
}

std::string Defs::find_node_path(const std::string& type, const std::string& name) const {
    for (const auto& s : suiteVec_) {
        std::string res = s->find_node_path(type, name);
        if (!res.empty())
            return res;
    }
    return string();
}

node_ptr Defs::find_node(const std::string& type, const std::string& pathToNode) const {
    // std::cout << "Defs::find_node  type:" << type << " path: " << pathToNode << "\n";
    node_ptr node_p = findAbsNode(pathToNode);
    if (!node_p) {
        // std::cout << " node not found\n";
        return node_p;
    }

    if (Str::caseInsCompare(type, "task")) {
        if (node_p->isTask())
            return node_p;
        return node_ptr();
    }
    if (Str::caseInsCompare(type, "family")) {
        if (node_p->isFamily())
            return node_p;
        return node_ptr();
    }
    if (Str::caseInsCompare(type, "suite")) {
        if (node_p->suite())
            return node_p;
        return node_ptr();
    }

    throw std::runtime_error("Defs::find_node: Node of type can't be found " + type);
    return node_ptr();
}

bool Defs::check(std::string& errorMsg, std::string& warningMsg) const {
    for (const auto& s : suiteVec_) {
        s->check(errorMsg, warningMsg);
    }
    return errorMsg.empty();
}

void Defs::getAllTasks(std::vector<Task*>& tasks) const {
    for (const auto& s : suiteVec_) {
        s->getAllTasks(tasks);
    }
}

void Defs::getAllSubmittables(std::vector<Submittable*>& tasks) const {
    for (const auto& s : suiteVec_) {
        s->getAllSubmittables(tasks);
    }
}

void Defs::get_all_active_submittables(std::vector<Submittable*>& tasks) const {
    for (const auto& s : suiteVec_) {
        s->get_all_active_submittables(tasks);
    }
}

void Defs::get_all_tasks(std::vector<task_ptr>& tasks) const {
    for (const auto& s : suiteVec_) {
        s->get_all_tasks(tasks);
    }
}

void Defs::get_all_nodes(std::vector<node_ptr>& nodes) const {
    for (const auto& s : suiteVec_) {
        s->get_all_nodes(nodes);
    }
}

void Defs::get_all_aliases(std::vector<alias_ptr>& aliases) const {
    for (const auto& s : suiteVec_) {
        s->get_all_aliases(aliases);
    }
}

void Defs::getAllFamilies(std::vector<Family*>& vec) const {
    for (const auto& s : suiteVec_) {
        s->getAllFamilies(vec);
    }
}

void Defs::getAllNodes(std::vector<Node*>& vec) const {
    vec.reserve(vec.size() + suiteVec_.size());
    for (const auto& s : suiteVec_) {
        vec.push_back(s.get());
        s->getAllNodes(vec);
    }
}

void Defs::getAllAstNodes(std::set<Node*>& theSet) const {
    for (const auto& s : suiteVec_) {
        s->getAllAstNodes(theSet);
    }
}

bool Defs::deleteChild(Node* nodeToBeDeleted) {
    // Find node and children of the node to be deleted and remove them from node history
    remove_edit_history(nodeToBeDeleted);

    Node* parent = nodeToBeDeleted->parent();
    if (parent)
        return parent->doDeleteChild(nodeToBeDeleted);
    return doDeleteChild(nodeToBeDeleted);
}

bool Defs::doDeleteChild(Node* nodeToBeDeleted) {
    //	std::cout << "Defs::doDeleteChild nodeToBeDeleted   = " << nodeToBeDeleted->debugNodePath() << "\n";

    auto theSuiteEnd = suiteVec_.end();
    for (auto s = suiteVec_.begin(); s != theSuiteEnd; ++s) {
        if ((*s).get() == nodeToBeDeleted) {
            Ecf::incr_modify_change_no();

            // Remove any child archived file
            (*s)->remove_archived_files();

            client_suite_mgr_.suite_deleted_in_defs(*s); // must be after Ecf::incr_modify_change_no();
            (*s)->set_defs(nullptr);                     // Must be set to NULL, allows re-added to a different defs
            suiteVec_.erase(s);
            set_most_significant_state(); // must be after suiteVec_.erase(s);
            return true;
        }
    }

    // recurse down only if we did not remove the suite
    for (auto s = suiteVec_.begin(); s != theSuiteEnd; ++s) {
        // SuiteChanged is called within doDeleteChild
        if ((*s)->doDeleteChild(nodeToBeDeleted)) {
            return true;
        }
    }
    return false;
}

void Defs::invalidate_trigger_references() const {
    for (const auto& s : suiteVec_) {
        s->invalidate_trigger_references();
    }
}

node_ptr Defs::replaceChild(const std::string& path,
                            const defs_ptr& clientDefs,
                            bool createNodesAsNeeded,
                            bool force,
                            std::string& errorMsg) {
    node_ptr clientNode = clientDefs->findAbsNode(path);
    if (!clientNode.get()) {
        errorMsg = "Cannot replace node since path ";
        errorMsg += path;
        errorMsg += " does not exist on the client definition";
        return node_ptr();
    }

    node_ptr serverNode = findAbsNode(path);
    if (!force && serverNode.get()) {
        // Check if serverNode has child tasks in submitted or active states
        vector<Task*> taskVec;
        serverNode->getAllTasks(taskVec); // taskVec will be empty if serverNode is a task
        int count = 0;
        for (Task* t : taskVec) {
            if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED)
                count++;
        }
        if (count != 0) {
            std::stringstream ss;
            ss << "Cannot replace node " << serverNode->debugNodePath() << " because it has " << count
               << " tasks which are active or submitted\n";
            ss << "Please use the 'force' option to bypass this check, at the expense of creating zombies\n";
            errorMsg = ss.str();
            return node_ptr();
        }
    }

    /// REPLACE ===========================================================
    if (!createNodesAsNeeded || serverNode.get()) {
        // Then the child must exist in the server defs (i.e. this)
        if (!serverNode.get()) {
            errorMsg = "Cannot replace child since path ";
            errorMsg += path;
            errorMsg += " does not exist on the server definition. Please use <parent> option";
            return node_ptr();
        }

        invalidate_trigger_references();

        // HAVE a FULL match in the server

        // Take a note of begun status, in case the suite is being replaced.
        bool begin_node = serverNode->suite()->begun();

        // Preserver suspended states, otherwise preserve state of client node
        if (serverNode->isSuspended())
            clientNode->suspend();

        std::vector<node_ptr> all_server_node_children;
        serverNode->allChildren(all_server_node_children);
        for (auto& i : all_server_node_children) {
            if (i->isSuspended()) {
                node_ptr client_node = clientDefs->findAbsNode(i->absNodePath());
                if (client_node)
                    client_node->suspend();
            }
        }

        // Find the position of the server node relative to its peers
        // We use this to re-add client node at the same position
        size_t child_pos = serverNode->position();

        // Delete node on the server, Must recurse down
        Node* parentNodeOnServer = serverNode->parent();
        deleteChild(serverNode.get());

        // Remove reference in the client defs to clientNode and detach from its parent
        // transfer ownership to the server
        bool addOk                  = false;
        node_ptr client_node_to_add = clientNode->remove();
        if (parentNodeOnServer) {
            addOk = parentNodeOnServer->addChild(client_node_to_add, child_pos);
        }
        else {
            addOk = placeChild(client_node_to_add, child_pos); // this is the second part of replacing a suite!
        }
        LOG_ASSERT(addOk, "");

        // preserve begun status. Note: we should't call begin() on the client side. As the suites will not have
        // been begun. This can cause assert, because begin time attributes assumes that calendar has been
        // initialised. This is not the case for client ASSERT failure: !c.suiteTime().is_special() at
        // ../core/src/TimeSeries.cpp:526 init has not been called on calendar. TimeSeries::duration ECFLOW-1612
        if (begin_node)
            client_node_to_add->begin();

        client_node_to_add->set_most_significant_state_up_node_tree();
        return client_node_to_add;
    }

    invalidate_trigger_references();

    // ADD ======================================================================
    // Create/Add nodes as needed for a *PARTIAL* match
    // If the path is /a/b/c/d/e/f it may be that path /a/b already exists
    // hence we need only create the missing nodes   c, d, e, f
    LOG_ASSERT(serverNode == nullptr, "");
    node_ptr server_parent;
    Node* last_client_child = clientNode.get(); // remember the last child
    Node* client_parent     = clientNode->parent();
    while (client_parent) {
        server_parent = findAbsNode(client_parent->absNodePath());
        if (server_parent) {
            break;
        }
        last_client_child = client_parent;
        client_parent     = client_parent->parent();
    }
    if (server_parent.get() == nullptr) {
        // NOT EVEN A PARTIAL path match, hence move over WHOLE suite, detach from client and add to server
        node_ptr client_suite_to_add = clientNode->suite()->remove();
        bool addOk                   = addChild(client_suite_to_add);
        LOG_ASSERT(addOk, "");

        client_suite_to_add->set_most_significant_state_up_node_tree();
        return client_suite_to_add;
    }
    if (server_parent->isTask()) {
        errorMsg = "Cannot replace child '";
        errorMsg += path;
        errorMsg += "' since path (";
        errorMsg += server_parent->absNodePath();
        errorMsg += ") in the server is a task.";
        return node_ptr();
    }

    // PARTIAL PATH MATCH,
    LOG_ASSERT(last_client_child, "");
    LOG_ASSERT(client_parent, "");
    LOG_ASSERT(last_client_child->parent() == client_parent, "");
    LOG_ASSERT(client_parent->absNodePath() == server_parent->absNodePath(), "");

    /// If the child of same name exist we *replace* at the same position otherwise we *add* it to the end
    size_t client_child_pos = last_client_child->position();

    size_t server_child_pos; // will be set to  std::numeric_limits<std::size_t>::max(), if child not found
    node_ptr server_child = server_parent->findImmediateChild(last_client_child->name(), server_child_pos);
    if (server_child.get()) {

        // Copy over suspended state
        if (server_child->isSuspended()) {
            last_client_child->suspend();
        }

        // Child of same name exist on the server, hence remove it
        deleteChild(server_child.get());
    }

    // Take a note of begun status, in case the suite is being replaced.
    bool begin_node = server_parent->suite()->begun();

    node_ptr client_node_to_add = last_client_child->remove();
    bool addOk                  = server_parent->addChild(client_node_to_add, client_child_pos);
    LOG_ASSERT(addOk, "");

    // preserve begun status. Note: we should't call begin() on the client side. As the suites will not have been
    // begun. This can cause assert, because begin time attributes assumes that calendar has been initialised. This
    // is not the case for client ASSERT failure: !c.suiteTime().is_special() at ../core/src/TimeSeries.cpp:526 init
    // has not been called on calendar. TimeSeries::duration ECFLOW-1612
    if (begin_node)
        client_node_to_add->begin();

    client_node_to_add->set_most_significant_state_up_node_tree();

    return client_node_to_add;
}

void Defs::cereal_save_as_checkpt(const std::string& the_fileName) const {
    // only_save_edit_history_when_check_pointing or if explicitly requested
    save_edit_history_ = true; // this is reset after edit_history is saved

    /// Can throw archive exception
    ecf::save(the_fileName, *this);
}

void Defs::cereal_restore_from_checkpt(const std::string& the_fileName) {
    //	cout << "Defs::cereal_restore_from_checkpt " << the_fileName << "\n";

    if (the_fileName.empty())
        return;

    // deleting existing content first. *** Note: Server environment left as is ****
    clear();

    ecf::restore(the_fileName, (*this));

    //	cout << "Restored: " << suiteVec_.size() << " suites\n";
}

void Defs::save_as_checkpt(const std::string& the_fileName) const {
    // Save as defs will always save children, hence no need for CheckPtContext

    // only_save_edit_history_when_check_pointing or if explicitly requested
    save_edit_history_ = true; // this is reset after edit_history is saved

    // Speed up check-pointing by avoiding indentation. i.e run_time and disk space
    // to view indented code use 'ecflow_client --load=checkpt_file check_only print'
    ecf::DisableIndentor disable_indentation;
    save_as_filename(the_fileName, PrintStyle::MIGRATE);
}

void Defs::save_as_filename(const std::string& the_fileName, PrintStyle::Type_t p_style) const {
    PrintStyle printStyle(p_style);

    std::ofstream ofs(the_fileName.c_str());
    std::string s;
    print(s);
    ofs << s;

    if (!ofs.good()) {
        std::string err = "Defs::save_as_filename: path(";
        err += the_fileName;
        err += ") failed: ";
        err += File::stream_error_condition(ofs);
        throw std::runtime_error(err);
    }
}

void Defs::save_as_string(std::string& the_string, PrintStyle::Type_t p_style) const {
    PrintStyle printStyle(p_style);

    // Speed up check-pointing by avoiding indentation. i.e run_time and disk space
    // to view indented code use 'ecflow_client --load=checkpt_file check_only print'
    ecf::DisableIndentor disable_indentation;
    print(the_string);
}

void Defs::restore(const std::string& the_fileName) {
    if (the_fileName.empty())
        return;

    /// *************************************************************************
    /// The reason why Parser code moved to Node directory. Avoid cyclic loop
    /// *************************************************************************
    std::string errorMsg, warningMsg;
    if (!restore(the_fileName, errorMsg, warningMsg)) {
        std::stringstream e;
        e << "Defs::defs_restore_from_checkpt: " << errorMsg;
        throw std::runtime_error(e.str());
    }
}

bool Defs::restore(const std::string& the_fileName, std::string& errorMsg, std::string& warningMsg) {
    if (the_fileName.empty()) {
        errorMsg = "Defs::restore: the filename string is empty";
        return false;
    }

    // deleting existing content first. *** Note: Server environment left as is ****
    clear();

    DefsStructureParser parser(this, the_fileName);
    bool ret = parser.doParse(errorMsg, warningMsg);
    return ret;
}

void Defs::restore_from_string(const std::string& str) {
    /// *************************************************************************
    /// The reason why Parser code moved to Node directory. Avoid cyclic loop
    /// *************************************************************************
    std::string errorMsg, warningMsg;
    if (!restore_from_string(str, errorMsg, warningMsg)) {
        std::stringstream e;
        e << "Defs::restore_from_string: " << errorMsg;
        throw std::runtime_error(e.str());
    }
}

bool Defs::restore_from_string(const std::string& str, std::string& errorMsg, std::string& warningMsg) {
    if (str.empty()) {
        errorMsg = "Defs::restore_from_string: the string is empty";
        return false;
    }

    // deleting existing content first. *** Note: Server environment left as is ****
    clear();

    // Do *NOT* Reset the state and modify numbers
    // As we we need this numbers for Syncing between client<->Server
    DefsStructureParser parser(this, str, false /* not used*/);
    bool ret = parser.doParse(errorMsg, warningMsg);
    return ret;
}

void Defs::clear() {
    // Duplicate AST are held in a static map.
    ExprDuplicate reclaim_cloned_ast_memory;

    // *** Note: Server environment left as is ****
    suiteVec_.clear();
    externs_.clear();
    client_suite_mgr_.clear();
    state_.setState(NState::UNKNOWN);
    edit_history_.clear();
    save_edit_history_ = false;
    Ecf::incr_modify_change_no();
}

bool Defs::checkInvariants(std::string& errorMsg) const {
    for (const auto& s : suiteVec_) {
        if (s->defs() != this) {
            std::stringstream ss;
            ss << "Defs::checkInvariants suite->defs() function not correct. Child suite parent ptr not correct\n";
            ss << "For suite " << s->name();
            errorMsg += ss.str();
            return false;
        }
        if (!s->isSuite()) {
            std::stringstream ss;
            ss << "Defs::checkInvariants suite isSuite() return NULL ? for suite " << s->name();
            errorMsg += ss.str();
            return false;
        }
        if (s->isSuite() != s->suite()) {
            std::stringstream ss;
            ss << "Defs::checkInvariants  s->isSuite(" << s->isSuite() << ") != s->suite(" << s->suite() << ") ";
            ss << "for suite " << s->name();
            errorMsg += ss.str();
            return false;
        }
        if (!s->checkInvariants(errorMsg)) {
            return false;
        }
    }

    if (Ecf::server()) {
        /// The change no should NOT be greater than Ecf::state_change_no()

        if (state_change_no_ > Ecf::state_change_no()) {
            std::stringstream ss;
            ss << "Defs::checkInvariants: state_change_no(" << state_.state_change_no() << ") > Ecf::state_change_no("
               << Ecf::state_change_no() << ")\n";
            errorMsg += ss.str();
            return false;
        }
        if (modify_change_no_ > Ecf::modify_change_no()) {
            std::stringstream ss;
            ss << "Defs::checkInvariants: modify_change_no_(" << modify_change_no_ << ") > Ecf::modify_change_no("
               << Ecf::modify_change_no() << ")\n";
            errorMsg += ss.str();
            return false;
        }

        if (flag_.state_change_no() > Ecf::state_change_no()) {
            std::stringstream ss;
            ss << "Defs::checkInvariants: flag.state_change_no()(" << flag_.state_change_no()
               << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
            errorMsg += ss.str();
            return false;
        }

        if (state_.state_change_no() > Ecf::state_change_no()) {
            std::stringstream ss;
            ss << "Defs::checkInvariants: state_.state_change_no()(" << state_.state_change_no()
               << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
            errorMsg += ss.str();
            return false;
        }

        if (server_.state_change_no() > Ecf::state_change_no()) {
            std::stringstream ss;
            ss << "Defs::checkInvariants: server_.state_change_no()(" << server_.state_change_no()
               << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
            errorMsg += ss.str();
            return false;
        }
    }
    return true;
}

void Defs::order(Node* immediateChild, NOrder::Order ord) {
    switch (ord) {
        case NOrder::TOP: {
            for (auto i = suiteVec_.begin(); i != suiteVec_.end(); ++i) {
                suite_ptr s = (*i);
                if (s.get() == immediateChild) {
                    suiteVec_.erase(i);
                    suiteVec_.insert(suiteVec_.begin(), s);
                    client_suite_mgr_.update_suite_order();
                    order_state_change_no_ = Ecf::incr_state_change_no();
                    return;
                }
            }
            throw std::runtime_error("Defs::order: TOP, immediate child suite not found");
        }
        case NOrder::BOTTOM: {
            for (auto i = suiteVec_.begin(); i != suiteVec_.end(); ++i) {
                suite_ptr s = (*i);
                if (s.get() == immediateChild) {
                    suiteVec_.erase(i);
                    suiteVec_.push_back(s);
                    order_state_change_no_ = Ecf::incr_state_change_no();
                    client_suite_mgr_.update_suite_order();
                    return;
                }
            }
            throw std::runtime_error("Defs::order: BOTTOM, immediate child suite not found");
        }
        case NOrder::ALPHA: {
            std::sort(suiteVec_.begin(), suiteVec_.end(), [](const suite_ptr& a, const suite_ptr& b) {
                try {
                    int a_as_int = ecf::convert_to<int>(a->name());
                    int b_as_int = ecf::convert_to<int>(b->name());
                    return a_as_int < b_as_int;
                }
                catch (const ecf::bad_conversion&) {
                }

                return Str::caseInsLess(a->name(), b->name());
            });
            order_state_change_no_ = Ecf::incr_state_change_no();
            client_suite_mgr_.update_suite_order();
            break;
        }
        case NOrder::ORDER: {
            std::sort(suiteVec_.begin(), suiteVec_.end(), [](const suite_ptr& a, const suite_ptr& b) {
                return Str::caseInsGreater(a->name(), b->name());
            });
            order_state_change_no_ = Ecf::incr_state_change_no();
            client_suite_mgr_.update_suite_order();
            break;
        }
        case NOrder::UP: {
            for (size_t t = 0; t < suiteVec_.size(); t++) {
                if (suiteVec_[t].get() == immediateChild) {
                    if (t != 0) {
                        suite_ptr s = suiteVec_[t];
                        suiteVec_.erase(suiteVec_.begin() + t);
                        t--;
                        suiteVec_.insert(suiteVec_.begin() + t, s);
                        order_state_change_no_ = Ecf::incr_state_change_no();
                    }
                    client_suite_mgr_.update_suite_order();
                    return;
                }
            }
            throw std::runtime_error("Defs::order: UP, immediate child suite not found");
        }
        case NOrder::DOWN: {
            for (size_t t = 0; t < suiteVec_.size(); t++) {
                if (suiteVec_[t].get() == immediateChild) {
                    if (t != suiteVec_.size() - 1) {
                        suite_ptr s = suiteVec_[t];
                        suiteVec_.erase(suiteVec_.begin() + t);
                        t++;
                        suiteVec_.insert(suiteVec_.begin() + t, s);
                        order_state_change_no_ = Ecf::incr_state_change_no();
                    }
                    client_suite_mgr_.update_suite_order();
                    return;
                }
            }
            throw std::runtime_error("Defs::order: DOWN, immediate child suite not found");
        }
        case NOrder::RUNTIME: {
            for (suite_ptr suite : suiteVec_) {
                if (suite->state() != NState::COMPLETE) {
                    throw std::runtime_error("Defs::order: To order by RUNTIME All suites must be complete");
                }
            }
            for (suite_ptr suite : suiteVec_)
                (void)suite->sum_runtime();
            std::sort(suiteVec_.begin(), suiteVec_.end(), [](const suite_ptr& a, const suite_ptr& b) {
                return a->state_change_runtime() > b->state_change_runtime();
            });
            order_state_change_no_ = Ecf::incr_state_change_no();
            client_suite_mgr_.update_suite_order();
            break;
        }
    }
}

void Defs::move_peer(Node* source, Node* dest) {
    move_peer_node(suiteVec_, source, dest, "Defs");
    order_state_change_no_ = Ecf::incr_state_change_no();
    client_suite_mgr_.update_suite_order();
}

void Defs::top_down_why(std::vector<std::string>& theReasonWhy, bool html_tags) const {
    bool why_found = why(theReasonWhy, html_tags);
    if (!why_found) {
        for (const auto& s : suiteVec_) {
            (void)s->top_down_why(theReasonWhy, html_tags);
        }
    }
}

bool Defs::why(std::vector<std::string>& theReasonWhy, bool html) const {
    if (isSuspended()) {
        std::string the_reason = "The server is *not* RUNNING.";
        theReasonWhy.push_back(the_reason);
        return true;
    }
    else if (state() != NState::QUEUED && state() != NState::ABORTED) {
        std::stringstream ss;
        if (html)
            ss << "The definition state(" << NState::to_html(state()) << ") is not queued or aborted.";
        else
            ss << "The definition state(" << NState::toString(state()) << ") is not queued or aborted.";
        theReasonWhy.push_back(ss.str());
    }
    return server_.why(theReasonWhy);
}

std::string Defs::toString() const {
    // Let the Client control the print style
    std::stringstream ss;
    ss << this;
    return ss.str();
}

// Memento functions
void Defs::collateChanges(unsigned int client_handle, DefsDelta& incremental_changes) const {
    // Collate any small scale changes to the defs
    collate_defs_changes_only(incremental_changes);

    if (0 == client_handle) {
        // small scale changes. Collate changes over all suites.
        // Suite stores the maximum state change, over *all* its children, this is used by client handle mechanism
        // and here to avoid traversing down the hierarchy.
        // ******** We must trap all child changes under the suite. See class SuiteChanged
        // ******** otherwise some attribute sync's will be missed
        for (const auto& s : suiteVec_) {
            //   *IF* node/attribute change no > client_state_change_no
            //   *THEN*
            //       Create a memento, and store in incremental_changes_
            s->collateChanges(incremental_changes);
        }
    }
    else {

        // small scale changes over the suites in our handle, determine what's changed,
        // relative to each node and attributes client_state_change_no.
        //   *IF* node/attribute change no > client_state_change_no
        //   *THEN*
        //       Create a memento, and store in incremental_changes_
        client_suite_mgr_.collateChanges(client_handle, incremental_changes);
    }
}

void Defs::collate_defs_changes_only(DefsDelta& incremental_changes) const {
    // ************************************************************************************************
    // determine if defs state changed. make sure this is in sync with defs_only_max_state_change_no()
    // ************************************************************************************************
    compound_memento_ptr comp;
    if (state_.state_change_no() > incremental_changes.client_state_change_no()) {
        if (!comp.get())
            comp = std::make_shared<CompoundMemento>(Str::ROOT_PATH());
        comp->add(std::make_shared<StateMemento>(state_.state()));
    }
    if (order_state_change_no_ > incremental_changes.client_state_change_no()) {
        if (!comp.get())
            comp = std::make_shared<CompoundMemento>(Str::ROOT_PATH());
        std::vector<std::string> order;
        order.reserve(suiteVec_.size());
        for (const auto& i : suiteVec_)
            order.push_back(i->name());
        comp->add(std::make_shared<OrderMemento>(order));
    }

    // Determine if the flag changed
    if (flag_.state_change_no() > incremental_changes.client_state_change_no()) {
        if (!comp.get())
            comp = std::make_shared<CompoundMemento>(Str::ROOT_PATH());
        comp->add(std::make_shared<FlagMemento>(flag_));
    }

    // determine if defs server state, currently only watch server state. i.e HALTED, SHUTDOWN, RUNNING
    if (server_.state_change_no() > incremental_changes.client_state_change_no()) {
        if (!comp.get())
            comp = std::make_shared<CompoundMemento>(Str::ROOT_PATH());
        comp->add(std::make_shared<ServerStateMemento>(server_.get_state()));
    }
    if (server_.variable_state_change_no() > incremental_changes.client_state_change_no()) {
        if (!comp.get())
            comp = std::make_shared<CompoundMemento>(Str::ROOT_PATH());
        comp->add(std::make_shared<ServerVariableMemento>(server_.user_variables()));
    }

    if (comp.get()) {
        incremental_changes.add(comp);
    }
}

unsigned int Defs::defs_only_max_state_change_no() const {
    // ************************************************************************************************
    // make sure this is in sync with collate_defs_changes_only()
    // ************************************************************************************************
    unsigned int max_change_no = 0;
    max_change_no              = std::max(max_change_no, state_.state_change_no());
    max_change_no              = std::max(max_change_no, order_state_change_no_);
    max_change_no              = std::max(max_change_no, flag_.state_change_no());
    max_change_no              = std::max(max_change_no, server_.state_change_no());
    max_change_no              = std::max(max_change_no, server_.variable_state_change_no());
    return max_change_no;
}

void Defs::set_memento(const StateMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "Defs::set_memento(const StateMemento* memento)\n";
#endif

    if (aspect_only)
        aspects.push_back(ecf::Aspect::STATE);
    else
        set_state(memento->state_);
}

void Defs::set_memento(const ServerStateMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "Defs::set_memento(const ServerStateMemento* memento)\n";
#endif

    if (aspect_only)
        aspects.push_back(ecf::Aspect::SERVER_STATE);
    else
        server_.set_state(memento->state_);
}

void Defs::set_memento(const ServerVariableMemento* memento,
                       std::vector<ecf::Aspect::Type>& aspects,
                       bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "Defs::set_memento(const ServerVariableMemento* memento)\n";
#endif

    if (aspect_only) {
        if (server_.user_variables().size() != memento->serverEnv_.size()) {
            aspects.push_back(ecf::Aspect::ADD_REMOVE_ATTR);
        }
        aspects.push_back(ecf::Aspect::SERVER_VARIABLE);
        return;
    }

    server_.set_user_variables(memento->serverEnv_);
}

void Defs::set_memento(const OrderMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "Defs::set_memento(const OrderMemento* memento)\n";
#endif
    if (aspect_only) {
        aspects.push_back(ecf::Aspect::ORDER);
        return;
    }

    // Order the suites

    // Order nodes according to memento ordering
    const std::vector<std::string>& order = memento->order_;

    // NOTE: When we have handles only a small subset of the suites, are returned
    //       Whereas order will always contain all the suites.
    //       Hence we need to handle the case where: order.size() != suiteVec_.size()

    std::vector<suite_ptr> vec;
    vec.reserve(suiteVec_.size());
    size_t node_vec_size = suiteVec_.size();
    for (const auto& i : order) {
        for (size_t t = 0; t < node_vec_size; t++) {
            if (i == suiteVec_[t]->name()) {
                vec.push_back(suiteVec_[t]);
                break;
            }
        }
    }
    if (vec.size() != suiteVec_.size()) {
        std::cout << "Defs::set_memento could not find all the names\n";
        return;
    }
    suiteVec_ = vec;
}

void Defs::set_memento(const FlagMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Defs::set_memento(const FlagMemento* memento)\n";
#endif

    if (aspect_only)
        aspects.push_back(ecf::Aspect::FLAG);
    else
        flag_.set_flag(memento->flag_.flag());
}

// =====================================================================

void Defs::add_edit_history(const std::string& path, const std::string& request) {
    auto i = edit_history_.find(path);
    if (i == edit_history_.end()) {
        edit_history_.insert(std::make_pair(path, std::vector<std::string>(1, request)));
    }
    else {
        (*i).second.push_back(request);
        if ((*i).second.size() > Defs::max_edit_history_size_per_node()) {
            (*i).second.erase((*i).second.begin());
        }
    }
}

void Defs::remove_edit_history(Node* node) {
    if (!node)
        return;

    // When removing a node *it* and all its children also need to be removed.
    std::vector<node_ptr> node_children;
    node->get_all_nodes(node_children);
    for (const auto& c : node_children) {

        auto it = edit_history_.find(c->absNodePath());
        if (it != edit_history_.end()) {
            edit_history_.erase(it);
        }
    }
}

void Defs::clear_edit_history() {
    edit_history_.clear();
}

const std::vector<std::string>& Defs::get_edit_history(const std::string& path) const {
    auto i = edit_history_.find(path);
    if (i != edit_history_.end()) {
        return (*i).second;
    }
    return empty_edit_history();
}

const std::vector<std::string>& Defs::empty_edit_history() {
    static std::vector<std::string> static_edit_history;
    return static_edit_history;
}

// =====================================================================================

void Defs::notify_delete() {
    // make a copy, to avoid iterating over observer list that is being changed
    std::vector<AbstractObserver*> copy_of_observers = observers_;
    for (auto& copy_of_observer : copy_of_observers) {
        copy_of_observer->update_delete(this);
    }

    /// Check to make sure that the Observer called detach
    /// We cannot call detach ourselves, since the the client needs to
    /// call detach in the case where the graphical tree is destroyed by user
    /// In this case the Subject/Node is being deleted.
    assert(observers_.empty());
}

void Defs::notify_start(const std::vector<ecf::Aspect::Type>& aspects) {
    for (auto& observer : observers_) {
        observer->update_start(this, aspects);
    }
}

void Defs::notify(const std::vector<ecf::Aspect::Type>& aspects) {
    for (auto& observer : observers_) {
        observer->update(this, aspects);
    }
}

void Defs::attach(AbstractObserver* obs) {
    observers_.push_back(obs);
}

void Defs::detach(AbstractObserver* obs) {
    for (size_t i = 0; i < observers_.size(); i++) {
        if (observers_[i] == obs) {
            observers_.erase(observers_.begin() + i);
            return;
        }
    }
}

bool Defs::is_observed(AbstractObserver* obs) const {
    for (auto observer : observers_) {
        if (observer == obs) {
            return true;
        }
    }
    return false;
}
// =====================================================================================

std::ostream& operator<<(std::ostream& os, const Defs* d) {
    if (d) {
        std::string s;
        d->print(s);
        os << s;
        return os;
    }
    return os << "DEFS == NULL\n";
}
std::ostream& operator<<(std::ostream& os, const Defs& d) {
    std::string s;
    d.print(s);
    os << s;
    return os;
}

// =========================================================================

DefsHistoryParser::DefsHistoryParser() = default;

void DefsHistoryParser::parse(const std::string& line) {
    size_t pos = line.find("\b");
    if (pos != std::string::npos) {
        // keep compatibility with current way of writing history
        std::string requests = line.substr(pos);
        Str::split(requests, parsed_messages_, "\b");
        return;
    }

    // fallback, split line based on looking for logType like 'MSG:[' | 'LOG:['
    string::size_type first = find_log(line, 0);
    if (first == std::string::npos)
        return;

    string::size_type next = find_log(line, first + 4);
    if (next == std::string::npos) {
        parsed_messages_.push_back(line.substr(first));
        return;
    }

    while (next != std::string::npos) {
        parsed_messages_.push_back(line.substr(first, next - first));
        first = next;
        next  = find_log(line, first + 4);

        if (next == std::string::npos) {
            parsed_messages_.push_back(line.substr(first));
            return;
        }
    }
}

string::size_type DefsHistoryParser::find_log(const std::string& line, string::size_type pos) const {
    std::vector<std::string> log_types;
    Log::get_log_types(log_types);

    for (auto log_type : log_types) {
        log_type += ":[";
        string::size_type log_type_pos = line.find(log_type, pos);
        if (log_type_pos != std::string::npos) {
            return log_type_pos;
        }
    }
    return std::string::npos;
}

std::string Defs::stats() const {
    std::vector<node_ptr> node_vec;
    get_all_nodes(node_vec);

    std::vector<Family*> family_vec;
    getAllFamilies(family_vec);

    std::vector<task_ptr> task_vec;
    get_all_tasks(task_vec);

    size_t alias = 0;
    for (auto task : task_vec)
        alias += task->aliases().size();

    NodeStats stats;
    stats.suites_ = suiteVec_.size();
    stats.family_ = family_vec.size();
    stats.task_   = task_vec.size();
    stats.alias_  = alias;
    stats.nodes_  = node_vec.size();

    stats.edit_history_nodes_ = edit_history_.size();
    for (const auto& i : edit_history_) {
        const std::vector<std::string>& vec = i.second; // list of requests
        stats.edit_history_paths_ += vec.size();
    }

    for (auto node : node_vec)
        node->stats(stats);
    return stats.print();
}

template <class Archive>
void Defs::serialize(Archive& ar, std::uint32_t const version) {
    ar(CEREAL_NVP(state_change_no_),
       CEREAL_NVP(modify_change_no_),
       CEREAL_NVP(updateCalendarCount_),
       CEREAL_NVP(state_),
       CEREAL_NVP(server_),
       CEREAL_NVP(suiteVec_));

    CEREAL_OPTIONAL_NVP(ar, flag_, [this]() { return flag_.flag() != 0; }); // conditionally save

    // only save the edit history when check pointing.
    CEREAL_OPTIONAL_NVP(
        ar, edit_history_, [this]() { return save_edit_history_ && !edit_history_.empty(); }); // conditionally save

    if (Archive::is_loading::value) {
        size_t vec_size = suiteVec_.size();
        for (size_t i = 0; i < vec_size; i++) {
            suiteVec_[i]->set_defs(this);
        }
    }
}

CEREAL_TEMPLATE_SPECIALIZE_V(Defs);
