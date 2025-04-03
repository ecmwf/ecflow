/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_Zombie_HPP
#define ecflow_attribute_Zombie_HPP

///
/// \brief Holds the zombie structure
///
/// WE have 3 structures:
///      Jobs(path,password,process_id)
///      Complications ONLY init command provides process_id.
///      Task(path,password,process_id)
///      Zombie(path,password,process_id)
/// Issues:
///   At the extreme we can have *2* Jobs running at one go. Typically, user commands
///   that have a force parameter will create 'user' zombies.
///   ** the command can be invoked at any time, i.e. we could invoke
///   ** the user command, whilst the task is in SUBMITTED state
///   ** This will go through and create user zombie. However, at this stage we have *NO* process_id
///   ** since the job has not started. Zombie(path,password)
///   When the Task init command is called, we get given a process_id.
///   ** HOWEVER ** need to determine if this is from the real child cmd or from the zombie
///   ** We search zombie list, and compare zombies, by path and password
///   ** if a match is found, we update the zombie id.
///
///   IMPORTANT:: Automated test:
///               If a child command provides just path and password, this is *NOT*
///               enough information to disambiguate a zombie from a real job.
///               To work around this the job file has been updated to add ECF_RID=$$
///
///   IMPORTANT:: For command line interface we just have the task path.
///               i,e we don't want to expose password, and user will not easily
///               know the process or remote id. Hence, we will make do with the
///               task path. We just find the first zombie, and act up on it
///

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "ecflow/attribute/ZombieAttr.hpp"
#include "ecflow/core/Child.hpp"

/// Use default copy constructor ,assignment operator and destructor
class Zombie {
public:
    Zombie(ecf::Child::ZombieType zombie_type,      // The kind of zombie
           ecf::Child::CmdType child_cmd,           // The Child command, that lead to this zombie
           const ZombieAttr& attr,                  // will hold Action if set on node tree
           const std::string& pathToTask,           // from child ipc
           const std::string& jobsPassword,         // from child ipc
           const std::string& process_or_remote_id, // from child ipc
           int try_no,
           const std::string& host,         // The host where the client was invoked
           const std::string& user_cmd = "" // user cmd that created this zombie
    );
    Zombie();

    bool operator==(const Zombie& rhs) const; // for python only
    std::string to_string() const;            // for python only

    /// accessors
    // distinguish between manual and automatic user action. manual take precedence
    bool manual_user_action() const { return user_action_set_; }
    bool fob() const;
    bool fail() const;
    bool adopt() const;
    bool block() const;
    bool remove() const;
    bool kill() const;

    ecf::Child::ZombieType type() const { return zombie_type_; }
    ecf::Child::CmdType last_child_cmd() const { return last_child_cmd_; }
    const ZombieAttr& attr() const { return attr_; }
    int calls() const { return calls_; }

    std::string type_str() const;
    const std::string& jobs_password() const { return jobs_password_; }
    const std::string& path_to_task() const { return path_to_task_; }
    const std::string& process_or_remote_id() const { return process_or_remote_id_; }
    const std::string& user_cmd() const { return user_cmd_; }
    const std::string& host() const { return host_; }
    int try_no() const { return try_no_; }
    int duration() const { return duration_; }
    ecf::ZombieCtrlAction user_action() const;
    std::string user_action_str() const;
    std::string explanation() const;

    const boost::posix_time::ptime& creation_time() const { return creation_time_; }

    bool empty() const { return path_to_task_.empty(); }

    /// returns in seconds the age the zombie is allowed to live
    /// Server typically checks every 60 seconds, hence this is lowest valid value that has effect
    int allowed_age() const;

    /// mutators
    void set_attr(const ZombieAttr& attr) { attr_ = attr; }
    void set_process_or_remote_id(const std::string& s) { process_or_remote_id_ = s; }
    void set_last_child_cmd(ecf::Child::CmdType c) { last_child_cmd_ = c; } // user cmd that created this zombie
    void set_host(const std::string& h) { host_ = h; }                      // The host where client/zombie was created
    void set_type(ecf::Child::ZombieType zt) { zombie_type_ = zt; }

    /// User action must take precedence over Zombie attribute settings on node tree
    ///
    /// Note: user_action to remove zombie is immediate hence no need to for set_remove()
    ///       Whereas for the other we want to store the action so that when next child
    ///       command communicate with the server, the action is applied.
    void set_duration(int d) { duration_ = d; }
    void set_fob();
    void set_fail();
    void set_adopt();
    void set_block();
    void set_kill();

    void increment_calls() { calls_++; }

    // write to standard out a title and list of zombies
    static std::string pretty_print(const std::vector<Zombie>& zombies, int indent = 0);
    static void pretty_print(const std::vector<Zombie>& zombies, std::vector<std::string>& list, int indent = 0);

    // MISC:
    // Added to support return by reference
    static const Zombie& EMPTY();
    static Zombie& EMPTY_();

private:
    ecf::ZombieCtrlAction user_action_{ecf::ZombieCtrlAction::BLOCK}; // [ fob, fail, remove, adopt, block, kill ]
    int try_no_{0};                                                   // task try number, set on construction
    int duration_{0};                                                 // How long zombie been alive
    int calls_{0};                                         // Number of times we have communicated with server.
    ecf::Child::ZombieType zombie_type_{ecf::Child::USER}; // [ ecf, ecf_pid, ecf_pid_passwd, ecf_passwd, path, user ]
    ecf::Child::CmdType last_child_cmd_{
        ecf::Child::INIT};                   // [ init | event | meter | label | wait | queue | abort | complete ]
    std::string path_to_task_;               // set on construction
    std::string jobs_password_;              // set on construction
    std::string process_or_remote_id_;       // set on construction
    std::string user_cmd_;                   // user cmd that created this zombie, empty otherwise
    std::string host_;                       // The client host name
    ZombieAttr attr_;                        // Default or attribute obtained from node tree.
    boost::posix_time::ptime creation_time_; // When zombie was created. Needed to control lifetime
    bool user_action_set_{false};            // Differentiate manual from automated, response, manual take precedence

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

std::ostream& operator<<(std::ostream& os, const Zombie&);

#endif /* ecflow_attribute_Zombie_HPP */
