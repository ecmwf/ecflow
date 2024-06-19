/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Flag_HPP
#define ecflow_node_Flag_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace cereal {
class access;
}

namespace ecf {

/// Flag are used store what has happened to a node. These are shown as icon in ecFlowview
/// Uses compiler generated copy constructor, assignment operator and destructor

/// During interactive use. A Node can be *forced to complete*, or forced to *run*
/// Typically the user may want to force a node to complete, if they are trying
/// to update the repeat variable.
///
/// In either case  we need to miss a time slot, this is done by setting the
/// NO_REQUE_IF_SINGLE_TIME_DEP, then at REQUE time we query the flag, if it was set
/// we avoid resetting the time slots. effectively missing the next time slot.
///
/// This functionality is only required during interactive force or run
/// However if the job aborted, we need to clear NO_REQUE_IF_SINGLE_TIME_DEP, i.e
///     time 10:00
///     time 11:00
/// If at 9.00am use the run command, we want to miss the 10:00 time slot.
/// However if the run at 9.00 fails, and we run again, we also miss 11:00 time slot
/// to avoid this if the job aborts, we clear NO_REQUE_IF_SINGLE_TIME_DEP flag.

class Flag {
public:
    using underlying_type_t = int;
    static_assert(sizeof(underlying_type_t) >= 4, "Flag's underlying type must have at least 4 bytes");

    Flag() = default;

    /// The BYRULE is used to distinguish between tasks that have RUN and completed
    /// and those that have completed by complete expression.
    enum Type {

        // Node* do not run when try_no > ECF_TRIES, and task aborted by user
        FORCE_ABORT = 0,

        // task
        USER_EDIT = 1,

        // task*
        TASK_ABORTED = 2,

        // task*
        EDIT_FAILED = 3,

        // task*
        JOBCMD_FAILED = 4,

        // task*
        NO_SCRIPT = 5,

        // task* do not run when try_no > ECF_TRIES, and task killed by user
        KILLED = 6,

        // Node attribute,
        LATE = 7,

        // Node
        MESSAGE = 8,

        // Node*, set if node is set to complete by complete trigger expression
        BYRULE = 9,

        // Node                                   ( NOT USED currently)
        QUEUELIMIT = 10,

        // task*  set when waiting for trigger expression in client command
        WAIT = 11,

        // Server                                 ( NOT USED currently)
        LOCKED = 12,

        // task*  Set/cleared but never queried by GUI
        ZOMBIE = 13,

        //
        NO_REQUE_IF_SINGLE_TIME_DEP = 14,

        // Container
        ARCHIVED = 15,

        // Container, Avoid re-archiving node that is restored, until re-queued again
        RESTORED = 16,

        // Job threshold exceeded, slow disk, large includes/huge scripts,overloaded machine,server)
        THRESHOLD = 17,

        // Record on defs that server received SIGTERM signal, main used in test
        ECF_SIGTERM = 18,

        //
        NOT_SET = 19,

        // Error in opening or writing to the log file
        LOG_ERROR = 20,

        // Error in saving checkpoint file
        CHECKPT_ERROR = 21,

        // task*
        KILLCMD_FAILED = 22,

        // task*
        STATUSCMD_FAILED = 23,

        // task*
        STATUS = 24,

        // Error connecting to remote source
        REMOTE_ERROR = 25
    };

    bool operator==(const Flag& rhs) const { return flag_ == rhs.flag_; }
    bool operator!=(const Flag& rhs) const { return !operator==(rhs); }

    // Flag functions:
    void set(Type flag);
    void clear(Type flag);
    bool is_set(Type flag) const { return (flag_ & (1 << flag)); }

    void reset();
    int flag() const { return flag_; }
    void set_flag(underlying_type_t f) { flag_ = f; }
    void set_flag(const std::string& flags); // these are comma separated

    /// returns a comma separated list of all flags set
    std::string to_string() const;
    void write(std::string&) const;

    /// returns the string equivalent
    static std::string enum_to_string(Flag::Type flag);
    static const char* enum_to_char_star(Flag::Type flag);

    /// Used to determine change in state relative to client
    void set_state_change_no(unsigned int n) { state_change_no_ = n; }
    unsigned int state_change_no() const { return state_change_no_; }

    /// returns the list of all flag types
    static std::vector<Flag::Type> list();
    static constexpr std::array<Flag::Type, 25> array();

    /// Converts from string to flag types.
    static Flag::Type string_to_flag_type(const std::string& s);

    /// valid flag types, than can be used in AlterCmd
    static void valid_flag_type(std::vector<std::string>& vec);

private:
    underlying_type_t flag_{0};
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};
} // namespace ecf

#endif /* ecflow_node_Flag_HPP */
