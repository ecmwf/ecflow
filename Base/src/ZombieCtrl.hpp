#ifndef ZOMBIE_CTRL_HPP_
#define ZOMBIE_CTRL_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : manages the zombies
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "Zombie.hpp"
#include "NodeFwd.hpp"
#include "Cmd.hpp"
class TaskCmd;
class AbstractServer;

/// All zombies are auto deleted after a period of time. See Zombie::allowed_age()
class ZombieCtrl : private boost::noncopyable {
public:
	ZombieCtrl() {}

	/// Handle the zombie, and return back to the client
	bool handle_zombie(
	                   Submittable*,            // Must be NON NULL
	                   const TaskCmd* task_cmd, // The child command
	                   std::string& action_taken,// action taken
	                   STC_Cmd_ptr& theReply    // Reply varies according to User Action
	                   );

	bool handle_path_zombie(
	                   AbstractServer* as,       // Find closest matching node
	                   const TaskCmd* task_cmd,  // The child command
                      std::string& action_taken,// action taken
	                   STC_Cmd_ptr& theReply     // Reply varies according to User Action
	                   );


	/// Find all *Tasks* beneath the input Node, that are in active or submitted states
	/// We deliberately ignore aliases (This was requested by Axel)
	/// Then add as 'USER' zombies. This should be called when commands like, delete, requeue,
	/// run, are using the force option, and will create zombies.
	void add_user_zombies(node_ptr);
	void add_user_zombies(suite_ptr);
	void add_user_zombies(defs_ptr);

	/// Returns the list of zombies, **updated** with seconds since creation
	/// To avoid sending attr to client, we copy over its setting, if in effect
 	void get(std::vector<Zombie>&)  ;


	/// remove all zombies, older than their allowed age
 	/// Server typically checks every 60 seconds
	void remove_stale_zombies(const boost::posix_time::ptime& time_now);


	/// Will ask the child command to terminate, *without* the child command raising any errors
	/// Since we can have many child commands(init,event,meter, label,complete) in a job
	/// This fob must stay around, since we don't know how long the job will last for.
	/// Will override, fail,recover,remove
	/// For Command Line Interface only the task_path is provided. Just have to make do.
	void fob(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);
	void fobCli(const std::string& path_to_task, Submittable*);


	/// Similar to a fob, But will ask the child command to terminate, with an error.
	/// Typically this will raise a trap in the job file, and hence raise an abort
	/// However the abort will also be a zombie, and will also be auto-terminated
	/// Hence the job file must be careful to use 'set -e' to avoid infinite recursion
	/// Since we can have many child commands(init,event,meter, label,complete) in a job
	/// This fob must stay around, since we don't know how long the job will last for.
	/// Will override, fob,recover,remove
	/// For Command Line Interface only the task_path is provided. Just have to make do.
	void fail( const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);
	void failCli(const std::string& path_to_task, Submittable*);


	/// Find the task node, corresponding to the zombie, and copy over
	/// the password. Then remove the zombie. Next time the child commands
	/// communicate with the server, all will be ok
	/// Will override, fail,fob,remove
	/// For Command Line Interface only the task_path is provided. Just have to make do.
	void adopt(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);
	void adoptCli(const std::string& path_to_task, Submittable*);
	void block(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);
	void blockCli(const std::string& path_to_task, Submittable*);
   void kill(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);
   void killCli(const std::string& path_to_task, Submittable*);


	/// Remove the zombie corresponding to the Task path. Should only be one
	/// Since we can have many child commands(init,event,meter, label,complete) in a job
	/// the zombies may get re-added, should we rember this.
	/// If path not found does nothing.
	/// For Command Line Interface only the task_path is provided. Just have to make do.
	bool remove(Submittable*);
	bool remove(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);
	void removeCli(const std::string& path_to_task, Submittable*);
    bool remove_by_path(const std::string& path_to_task);

	/// Query
 	const Zombie& find(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password) const;
	const Zombie& find_by_path_only(const std::string& path_to_task) const;

private:

	bool handle_existing_zombie(
	                   Zombie &,                // The server already knows about the zombie
	                   Submittable*,            // This NULL for path zombies
	                   node_ptr closest_matching_node, // only set for path zombies
	                   const TaskCmd* task_cmd, // The child command
                      std::string& action_taken,// User action taken
	                   STC_Cmd_ptr& theReply    // Reply varies according to User Action
	                   );

	bool handle_user_actions(
	                   Zombie &,                 // The server already knows about the zombie
	                   Submittable*,             // This NULL for path zombies
	                   const TaskCmd* task_cmd,  // The child command
                      std::string& action_taken,// User action taken
	                   STC_Cmd_ptr& theReply     // Reply varies according to User Action
	                   );

	Zombie& find_zombie(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);
	Zombie& find(Submittable*);
	const Zombie& find(Submittable*) const;
	void do_add_user_zombies(const std::vector<Submittable*>& tasks);
	Zombie& find_by_path(const std::string& path_to_task);

private:
	std::vector<Zombie> zombies_;
};
#endif
