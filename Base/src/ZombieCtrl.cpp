/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #41 $ 
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
#include "ZombieCtrl.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Submittable.hpp"
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"

using namespace ecf;
using namespace std;
using namespace boost::posix_time;

//#define DEBUG_ZOMBIE  1

/// Zombie creation:
/// *** ECF  *** Zombies is created with: process(node path, password, pid/rid , try_no) ***
/// *** PATH *** Zombies is created with: process(node path, password, pid/rid , try_no) ***
/// *** USER *** Zombies is created with:    TASK(node path, password, pid/rid , try_no) ***
///
/// There are several places where we hold path,password,pid/rid,etc
///                        Task Cmd/
///            Node Tree   Process(n)   USER Zombie         PATH/ECF Zombie
///  Path                               same as node tree   same as process
///  password                           same as node tree   same as process
///  pid/rid                            same as node tree   same as process
///  try no                             same as node tree   same as process
///    ^                    |
///    | ----adopt-----------
///
/// Zombie Finding:
/// For a given Task, we could have multiple zombie process, i.e. with different password/process id
/// We can't assume that there is only one zombie process. Hence search/zombie matching
/// should involve matching with password/process id first and then resort to path matching
///
/// Note: Only the init child command is required pass the process id. The other child command may
//        or may *not* provide this. In the test scenario we do.
static bool match(const Zombie& z, const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password);


bool ZombieCtrl::handle_path_zombie(
         AbstractServer* as,
         const TaskCmd* task_cmd,  // The child command
         std::string& action_taken,
         STC_Cmd_ptr& theReply     // Reply varies according to User Action
)
{
#ifdef DEBUG_ZOMBIE
	std::cout << "   ZombieCtrl::handle_path_zombie:";
#endif
	const std::string& path_to_task = task_cmd->path_to_node();
	const std::string& jobs_password = task_cmd->jobs_password();
	const std::string& process_or_remote_id = task_cmd->process_or_remote_id();


	/// *** The ZombieAttr may have added/ deleted via AlterCmd. This allows for dynamic changes
	/// *no* task, find the closest Zombie attribute up the Node tree, ie attribute could be on family/suite even though task has been deleted
	/// If none found we resort to default behaviour
	node_ptr closest_matching_node = as->defs()->find_closest_matching_node(path_to_task);
#ifdef DEBUG_ZOMBIE
	if (closest_matching_node.get()) std::cout << " closest node found: ";
#endif


#ifdef DEBUG_ZOMBIE
   std::cout << " Searching for match over " << zombies_.size() << " zombies :";
#endif
	Zombie& theExistingZombie = find_zombie(path_to_task, process_or_remote_id, jobs_password );
 	if(!theExistingZombie.empty() ) {
 	   // When NULL is passed for task, we change existing zombie to be of type PATH
 		return handle_existing_zombie(theExistingZombie,NULL/*task*/,closest_matching_node,task_cmd,action_taken,theReply);
 	}

	/// Create a zombie,
   /// *** PATH *** Zombies is created with: process path, process password, and process id/rid , process try_no ***
#ifdef DEBUG_ZOMBIE
	std::cout << " No matching zombie fnd: Creating Path Zombie: ";
#endif

	ZombieAttr attr = ZombieAttr::get_default_attr(Child::PATH);
	if (closest_matching_node.get()) {
		closest_matching_node->findParentZombie(Child::PATH, attr ); // Override default from node tree
	}

	Zombie new_zombie(Child::PATH,task_cmd->child_type(),attr,path_to_task,jobs_password,process_or_remote_id,task_cmd->try_no());
 	zombies_.push_back( new_zombie );

 	/// The user action may end deleting the zombie just added. Depends on ZombieAttribute settings
	return handle_user_actions(new_zombie,NULL /*task*/,task_cmd,action_taken,theReply);
}


bool ZombieCtrl::handle_zombie(
                               Submittable* task,              // This NULL for path zombies
                               const TaskCmd* task_cmd, // The child command
                               std::string& action_taken,
                               STC_Cmd_ptr& theReply    // Reply varies according to User Action
)
{
#ifdef DEBUG_ZOMBIE
	std::cout << "   ZombieCtrl::handle_zombie:";
#endif
	const std::string& path_to_task = task_cmd->path_to_node();
	const std::string& jobs_password = task_cmd->jobs_password();
	const std::string& process_or_remote_id = task_cmd->process_or_remote_id();

	Zombie& theExistingZombie = find_zombie(path_to_task, process_or_remote_id, jobs_password );
 	if(!theExistingZombie.empty() ) {
 		return handle_existing_zombie(theExistingZombie,task,node_ptr(),task_cmd,action_taken,theReply);
 	}

 	/// Create Zombie:
 	/// *** ECF *** Zombies is created with: process path, process password, and process id/rid , process try_no ***
	Child::ZombieType zombie_type = Child::ECF;
	ZombieAttr attr = ZombieAttr::get_default_attr( zombie_type );

	/// Look for any Zombie attribute up node tree, use this to construct & configure zombie
	task->findParentZombie(zombie_type, attr );

	/// Handle corner case ,where we have two jobs with different process id, but same password
	/// Can happen if jobs is started externally, or via test, occasionally
 	ecf::Child::CmdType child_type = task_cmd->child_type();
	if (child_type == Child::INIT && task->state() == NState::ACTIVE) {

		/// Find zombie by path only, and remove it. Re-added again below. With updated, data<<<<
#ifdef DEBUG_ZOMBIE
		cout << " >TASK already active:< ";
#endif
		size_t zombieVecSize = zombies_.size();
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if (zombies_[i].path_to_task() == path_to_task) {
				zombie_type = zombies_[i].type();     // recover the original zombie type
				zombies_.erase( zombies_.begin() + i);
#ifdef DEBUG_ZOMBIE
				cout << " Removing: ";
#endif
				break;
			}
		}
	}


#ifdef DEBUG_ZOMBIE
	std::cout << " Creating ECF Zombie:";
#endif
	Zombie new_zombie(zombie_type,child_type,attr,path_to_task,jobs_password,process_or_remote_id,task_cmd->try_no());
	zombies_.push_back( new_zombie );

	/// Mark task as zombie for xcdp
	task->flag().set(ecf::Flag::ZOMBIE);

	return handle_user_actions(new_zombie,task,task_cmd,action_taken,theReply);
}

bool ZombieCtrl::handle_existing_zombie(
                   Zombie& theExistingZombie,      // The server already knows about the zombie
                   Submittable* task,              // This NULL for path zombies
                   node_ptr closest_matching_node, // only defined for path zombies
                   const TaskCmd* task_cmd,        // The child command
                   std::string& action_taken,      // User action taken
                   STC_Cmd_ptr& theReply           // Reply varies according to User Action
)
{
#ifdef DEBUG_ZOMBIE
	std::cout << " handle_existing_zombie: Updating child_type: ";
#endif
	// If we have no task, then change the zombie type to PATH
	if (!task) {
#ifdef DEBUG_ZOMBIE
	   std::cout << " : updating zombie type to PATH: ";
#endif
	   theExistingZombie.set_type( ecf::Child::PATH );
	}

	/// *** The ZombieAttr may have added/ deleted via AlterCmd. This allows for dynamic changes
	ZombieAttr attr = ZombieAttr::get_default_attr(theExistingZombie.type());
	if (closest_matching_node.get()) {
#ifdef DEBUG_ZOMBIE
	   cout << " Attr found for path zombie(" << attr.toString() << "): ";
#endif
		closest_matching_node->findParentZombie(theExistingZombie.type(), attr ); // Override default from node tree
	}

	if (task && task->findParentZombie(theExistingZombie.type(), attr )) {        // Override default from node tree
#ifdef DEBUG_ZOMBIE
		cout << " Attr found(" << attr.toString() << "): ";
#endif
	}
	theExistingZombie.set_attr( attr );                            // Update attribute stored on the zombie
	theExistingZombie.set_last_child_cmd( task_cmd->child_type() );// The zombie stores the last child command.
	theExistingZombie.increment_calls();                           // record how times server handled with zombie


	/// Update the process id, if it is empty on the existing zombie
	/// *** NOTE**** can not update process id from task, as that could be an ID from a different JOB,
	/// ************ Hence zombie matching resorts to path and password matching
	const std::string& process_or_remote_id = task_cmd->process_or_remote_id();
	if (theExistingZombie.process_or_remote_id().empty() && !process_or_remote_id.empty()) {
#ifdef DEBUG_ZOMBIE
		std::cout << "Updating process id(" << process_or_remote_id << "): ";
#endif
		theExistingZombie.set_process_or_remote_id( process_or_remote_id );
	}

	return handle_user_actions(theExistingZombie,task,task_cmd,action_taken,theReply);
}

bool ZombieCtrl::handle_user_actions(
                   Zombie& theZombie,        // Existing or one we just created
                   Submittable* task,        // This is NULL for path zombies
                   const TaskCmd* task_cmd,  // The child command
                   std::string& action_taken,// User action taken
                   STC_Cmd_ptr& theReply     // Reply varies according to User Action
)
{
	const std::string& path_to_task = task_cmd->path_to_node();
	const std::string& process_password = task_cmd->jobs_password();
	const std::string& process_or_remote_id = task_cmd->process_or_remote_id();

	if (theZombie.manual_user_action()) action_taken = "manual-";
	else action_taken = "automatic-";

	// *ADOPT* If zombie is set to adopt, copy over password and carry on as >NORMAL< , i.e. we return true
	if ( task && theZombie.adopt()) {

	   action_taken += "adopt";
		/// Zombie was marked to adopt. password copied over, and zombie removed
		/// *MUST* use the password of the process, and *NOT* the zombie
		/// Since next time process communicates, it will be *WITH* the process password
		task->set_jobs_password( process_password );
		task->set_process_or_remote_id( process_or_remote_id );

		/// Remove the zombie, as its been adopted
		/// matching by password/process id may fail, hence remove by path
		bool remove_ok = remove(path_to_task, process_or_remote_id, process_password );
		if (!remove_ok) {
		   (void) remove_by_path(path_to_task);
		}

		/// Clear the zombie flag
		task->flag().clear(ecf::Flag::ZOMBIE);

#ifdef DEBUG_ZOMBIE
		std::cout << " >>>ADOPT<<< then remove(" << remove_ok <<") ";
		if (!remove_ok)  std::cout << " >>>ERROR<<<< Remove failed ";
		std::cout << " zombies_.size(" << zombies_.size() << ")\n";
#endif
		return true;
	}

	// *FOB*
	if (theZombie.fob()) {
		/// Means return as if everything is OK. hence ClientInvoker will *NOT* block, and job can continue.
      action_taken += "fob";

		/// On the child COMPLETE/ABORT cmd, remove the zombie:
      /// *****************************************************************************************
	   /// Since we are returning false, The Task Cmd complete/abort **wont** be able to remove the zombie
	   /// i.e since we want job to continue, *WITHOUT* invoking the dohandeRequest
      /// *****************************************************************************************
		if (task_cmd->child_type() == Child::COMPLETE || task_cmd->child_type() == Child::ABORT ) {

			bool remove_ok = remove(path_to_task, process_or_remote_id, process_password );
	      if (!remove_ok) {
	         (void)remove_by_path(path_to_task);
	      }

			/// Clear the zombie flag
			if (task) task->flag().clear(ecf::Flag::ZOMBIE);

#ifdef DEBUG_ZOMBIE
			std::cout << " child == COMPLETE remove zombie ";
			if (!remove_ok)  std::cout << " >>>ERROR<<<< Remove failed ";
#endif
		}

#ifdef DEBUG_ZOMBIE
		std::cout << " >>>FOB<<< zombies_.size(" << zombies_.size() << ")\n";
#endif
		theReply = PreAllocatedReply::ok_cmd();
		return false;
	}

	// *FAIL* Ask ClientInvoker *NOT* to block,  *fail* with an error.
	if (theZombie.fail()) {
#ifdef DEBUG_ZOMBIE
		std::cout << " >>>FAIL<<< zombies_.size(" << zombies_.size() << ")\n";
#endif
      action_taken += "fail";
		theReply = PreAllocatedReply::error_cmd("[ authentication failed ] Request set to FAIL via zombie setting");
		return false;
	}

   // *KILL* Typically kill is immediate(i.e. via ZombieCmd), However this could have been set via ZombieAttribute
   if (theZombie.kill()) {
      // when a task a script is killed(i,e with kill -15), it will typically be trapped
      // This will then typically call abort. We have a choice:
      //    a/ If we remove the zombie, the action taken will be lost, when the abort arises, hence default action is block
      //    b/ Change the action type to be fob, so that the abort does not block
      //    c/ Continue killing until process terminate. Up to use to remove zombies
      // Opted for option b/ however we do *NOT* change action type, we just fob
      if (task) {
         if (!task->flag().is_set(ecf::Flag::KILLED)) {

            action_taken += "kill & fob";

            // Kill the task, separate process, will typically send kill -15 to script.
            task->kill(theZombie.process_or_remote_id());
         }
         else {
            action_taken += "kill(already killed, fobing instead)";
         }
      }
      else  {
         action_taken += "kill(no task, fobing instead)";
      }
#ifdef DEBUG_ZOMBIE
      std::cout << " >>>KILL<<< zombies_.size(" << zombies_.size() << ")\n";
#endif
      theReply = PreAllocatedReply::ok_cmd(); // do not block the script, fob
      return false;
   }

	// *REMOVE* Typically Removal is immediate(i.e. via ZombieCmd), However this could have been set via ZombieAttribute
	if (theZombie.remove()) {
		/// Ask ClientInvoker to continue blocking, Zombie may re-appear
      action_taken += "remove";
		bool remove_ok = remove(path_to_task, process_or_remote_id, process_password);
      if (!remove_ok)  (void)remove_by_path(path_to_task);

#ifdef DEBUG_ZOMBIE
		std::cout << " >>>REMOVE<<< zombies_.size(" << zombies_.size() << ") : BLOCKING ";
		if (!remove_ok)  std::cout << " >>>ERROR<<<< Remove failed ";
#endif
	   theReply = PreAllocatedReply::block_client_zombie_cmd();
	   return false;
	}

	// *DEFAULT*:
	//       Label,event,meter       : fob
	//       init,complete,abort,wait: block
	if (task_cmd->child_type() == Child::LABEL ||
	    task_cmd->child_type() == Child::EVENT ||
	    task_cmd->child_type() == Child::METER) {

      /// Means return as if everything is OK. hence ClientInvoker will *NOT* block, and job can continue.
#ifdef DEBUG_ZOMBIE
   std::cout << ": FOB\n";
#endif
	   action_taken += "fob";
      theReply = PreAllocatedReply::ok_cmd();
      return false;
	}

#ifdef DEBUG_ZOMBIE
	std::cout << ": BLOCKING\n";
#endif
	// i.e it will make several attempts , and then start contacting servers in the hosts file.
   action_taken += "block";
	theReply = PreAllocatedReply::block_client_zombie_cmd();
	return false;
}


void ZombieCtrl::do_add_user_zombies(const std::vector<Submittable*>& tasks)
{
 	size_t taskVecSize = tasks.size();
	for(size_t i = 0; i < taskVecSize; i++) {
		Submittable* t = tasks[i];
 		if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED) {

 			Zombie& theExistingZombie = find( t );
  			if (theExistingZombie.empty() ) {

#ifdef DEBUG_ZOMBIE
  				std::cout << "   ZombieCtrl::do_add_user_zombies " << t->absNodePath() << " " << t->process_or_remote_id() << " " << t->jobsPassword() << "\n";
#endif

  				ZombieAttr attr = ZombieAttr::get_default_attr(Child::USER); // get the default USER zombie attribute
  				t->findParentZombie(Child::USER, attr );                     // Override default from the node tree

 				zombies_.push_back(Zombie(Child::USER,Child::INIT,attr,t->absNodePath(),t->jobsPassword(),t->process_or_remote_id(),t->try_no()));

 		 	  	/// Mark task as zombie for xcdp
 		 	  	t->flag().set(ecf::Flag::ZOMBIE);
 			}
		}
	}
}

void ZombieCtrl::add_user_zombies( node_ptr node)
{
	if (!node.get()) return;
	std::vector<Submittable*> tasks;
	node->get_all_active_submittables(tasks);
	do_add_user_zombies(tasks);
}

void ZombieCtrl::add_user_zombies( suite_ptr suite)
{
	if (!suite.get()) return;
	std::vector<Submittable*> tasks;
	suite->get_all_active_submittables(tasks);
	do_add_user_zombies(tasks);
}

void ZombieCtrl::add_user_zombies( defs_ptr defs)
{
	if (!defs.get()) return;
	std::vector<Submittable*> tasks;
	defs->get_all_active_submittables(tasks);
	do_add_user_zombies(tasks);
}

/// Returns the list of zombies, **updated** with seconds since creation
void ZombieCtrl::get( std::vector< Zombie >& ret)   {

	boost::posix_time::ptime time_now = Calendar::second_clock_time();

	size_t zombieVecSize = zombies_.size(); ret.reserve(zombieVecSize);
	for(size_t i = 0 ; i < zombieVecSize; i++) {

		time_duration duration = time_now - zombies_[i].creation_time();
		zombies_[i].set_duration( duration.total_seconds() );

		ret.push_back(zombies_[i]);
	}
}

void ZombieCtrl::remove_stale_zombies( const boost::posix_time::ptime& time_now )
{
	for(std::vector<Zombie>::iterator i = zombies_.begin(); i != zombies_.end(); ++i) {
		time_duration duration = time_now - (*i).creation_time();
		if ( duration.total_seconds() > (*i).allowed_age()) {
#ifdef DEBUG_ZOMBIE
			std::cout << "   ZombieCtrl::remove_stale_zombies " << (*i) << "\n";
#endif
			zombies_.erase(i--);
		}
 	}
}

void ZombieCtrl::fob( const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password) {

	Zombie& theExistingZombie = find_zombie(path_to_task, process_or_remote_id, password );
	if( theExistingZombie.empty() ) return;
	theExistingZombie.set_fob();
}

void ZombieCtrl::fobCli( const std::string& path_to_task,  Submittable* task) {

	if (task) {
		/// Try to determine the real zombie. (not 100% precise) by comparing its password with zombie
		/// If zombie password does *NOT* match then this is the real zombie.
		size_t zombieVecSize = zombies_.size();
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if (zombies_[i].path_to_task() == path_to_task && zombies_[i].jobs_password() != task->jobsPassword()) {
	  			zombies_[i].set_fob();
	  			return;
			}
		}
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if (zombies_[i].path_to_task() == path_to_task && zombies_[i].process_or_remote_id() != task->process_or_remote_id()) {
	  			zombies_[i].set_fob();
	  			return;
			}
		}
	}

   /// The best we can do
   Zombie& theExistingZombie = find_by_path( path_to_task );
   if ( theExistingZombie.empty() )  return;
   theExistingZombie.set_fob();
}

void ZombieCtrl::fail(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password) {

	Zombie& theExistingZombie = find_zombie(path_to_task, process_or_remote_id, password );
	if( theExistingZombie.empty() ) return;
	theExistingZombie.set_fail();
}

void ZombieCtrl::failCli( const std::string& path_to_task,  Submittable* task) {

	if (task) {
		/// Try to determine the real zombie. (not 100% precise) by comparing its password with zombie
		/// If zombie password does *NOT* match then this is the real zombie.
		size_t zombieVecSize = zombies_.size();
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if (zombies_[i].path_to_task() == path_to_task && zombies_[i].jobs_password() != task->jobsPassword()) {
	  			zombies_[i].set_fail();
	  			return;
			}
		}
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if (zombies_[i].path_to_task() == path_to_task && zombies_[i].process_or_remote_id() != task->process_or_remote_id()) {
	  			zombies_[i].set_fail();
	  			return;
			}
		}
	}

   /// The best we can do
   Zombie& theExistingZombie = find_by_path( path_to_task );
   if ( theExistingZombie.empty() )  return;
   theExistingZombie.set_fail();
}

void ZombieCtrl::adopt( const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password ) {

	Zombie& theExistingZombie = find_zombie(path_to_task, process_or_remote_id, password );
	if ( theExistingZombie.empty() ) return;
	theExistingZombie.set_adopt();
}

void ZombieCtrl::adoptCli( const std::string& path_to_task,  Submittable* task) {

	if (!task) {
		throw std::runtime_error("ZombieCtrl::adoptCli: Can't adopt zombie, there is no corresponding task!");
	}

	/// Try to determine the real zombie. (not 100% precise) by comparing its password with zombie
	/// If zombie password does *NOT* match then this is the real zombie.
	size_t zombieVecSize = zombies_.size();
	for(size_t i = 0 ; i < zombieVecSize; i++) {
	   if (zombies_[i].path_to_task() == path_to_task && zombies_[i].jobs_password() != task->jobsPassword()) {
	      zombies_[i].set_adopt();
	      return;
	   }
	}

   /// ***************************************************************************************
   /// IMPORTANT: We should *NEVER* adopt a zombie, when the process id are different
   /// This can end up, with two process running, Will mess up job output, as well as corruption caused
	/// but running the same job twice. Better to kill both and re-queue.
   /// Note: PBS can create two process, i.e same password, different PID's
   /// ***************************************************************************************
   for(size_t i = 0 ; i < zombieVecSize; i++) {
      if (zombies_[i].path_to_task() == path_to_task && zombies_[i].process_or_remote_id() != task->process_or_remote_id()) {
         std::stringstream ss;
         ss << "ZombieCtrl::adoptCli: Can *not* adopt zombies, where process id are different. Task("
            << task->process_or_remote_id() << ") zombie(" << zombies_[i].process_or_remote_id()
            << "). Please kill both process, and re-queue";
         throw std::runtime_error(ss.str());
      }
   }
}

void ZombieCtrl::block( const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password ) {

	Zombie& theExistingZombie = find_zombie(path_to_task, process_or_remote_id, password );
	if ( theExistingZombie.empty() ) return;
	theExistingZombie.set_block();
}

void ZombieCtrl::blockCli( const std::string& path_to_task,  Submittable* task) {

	if (!task) {
		throw std::runtime_error("ZombieCtrl::blockCli: Can't block zombie, there is no corresponding task for path " + path_to_task );
	}
	else {
		/// Try to determine the real zombie. (not 100% precise) by comparing its password with zombie
		/// If zombie password does *NOT* match then this is the real zombie.
		size_t zombieVecSize = zombies_.size();
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if (zombies_[i].path_to_task() == path_to_task && zombies_[i].jobs_password() != task->jobsPassword()) {
	  			zombies_[i].set_block();
	  			return;
			}
		}
	}
}

void ZombieCtrl::kill( const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password ) {

   Zombie& theExistingZombie = find_zombie(path_to_task, process_or_remote_id, password );
   if ( theExistingZombie.empty() ) return;
   theExistingZombie.set_kill();
}

void ZombieCtrl::killCli( const std::string& path_to_task,  Submittable* task) {

   if (!task) {
      throw std::runtime_error("ZombieCtrl::killCli: Can't kill zombie, there is no corresponding task for path " + path_to_task );
   }

   /// Try to determine the real zombie. (not 100% precise) by comparing its password with zombie
   /// If zombie password does *NOT* match then this is the real zombie.
   size_t zombieVecSize = zombies_.size();
   for(size_t i = 0 ; i < zombieVecSize; i++) {
      if (zombies_[i].path_to_task() == path_to_task && zombies_[i].jobs_password() != task->jobsPassword()) {
         task->kill(zombies_[i].process_or_remote_id());
         zombies_[i].set_kill();
         return;
      }
   }
   for(size_t i = 0 ; i < zombieVecSize; i++) {
      if ( zombies_[i].path_to_task() == path_to_task && zombies_[i].process_or_remote_id() != task->process_or_remote_id()) {
         task->kill(zombies_[i].process_or_remote_id());
         zombies_[i].set_kill();
         return ;
      }
   }
   /// The best we can do
   Zombie& theExistingZombie = find_by_path( path_to_task );
   if ( theExistingZombie.empty() ) {
      throw std::runtime_error("ZombieCtrl::killCli: Can't kill, could not locate zombie(and hence pid) for path: " + path_to_task );
   }
   task->kill(theExistingZombie.process_or_remote_id());
   theExistingZombie.set_kill();
   remove_by_path(path_to_task);
}

/// Called by the child commands, ie complete and abort
/// Hence remove zombie with matching password/process id ?
bool ZombieCtrl::remove( Submittable* t)
{
	if (t) {
		return remove(t->absNodePath(), t->process_or_remote_id(), t->jobsPassword() );
	}
	return false;
}

bool ZombieCtrl::remove( const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password ) {

	/// Note: Its possible for two separate jobs to have the same password. (submit 1, submit 2) before job1 active, password overridden by submit 2
	/// Hence remove needs to at least match process_id
	size_t zombieVecSize = zombies_.size();
	for(size_t i = 0 ; i < zombieVecSize; i++) {
		if (match(zombies_[i],path_to_task,process_or_remote_id,password)) {
//#ifdef DEBUG_ZOMBIE
//			std::cout << "   ZombieCtrl::remove " << zombies_[i] << " \n";
//#endif
			zombies_.erase( zombies_.begin() + i);
			return true;
		}
	}
   return false;
}

void ZombieCtrl::removeCli( const std::string& path_to_task,  Submittable* task) {

	if (task) {
		/// Try to determine the real zombie. (not 100% precise) by comparing its password with zombie
		/// If zombie password does *NOT* match then this is the real zombie.
	   size_t zombieVecSize = zombies_.size();
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if ( zombies_[i].path_to_task() == path_to_task && zombies_[i].jobs_password() != task->jobsPassword()) {
#ifdef DEBUG_ZOMBIE
				std::cout << "   ZombieCtrl::removeCli " << zombies_[i] << " \n";
#endif
				zombies_.erase( zombies_.begin() + i);
				return ;
			}
		}
		for(size_t i = 0 ; i < zombieVecSize; i++) {
			if ( zombies_[i].path_to_task() == path_to_task && zombies_[i].process_or_remote_id() != task->process_or_remote_id()) {
#ifdef DEBUG_ZOMBIE
			std::cout << "   ZombieCtrl::removeCli " << zombies_[i] << " \n";
#endif
				zombies_.erase( zombies_.begin() + i);
				return ;
			}
		}
	}

   /// The best we can do
   (void)remove_by_path(path_to_task);
}

bool ZombieCtrl::remove_by_path(const std::string& path_to_task)
{
   size_t zombieVecSize = zombies_.size();
   for(size_t i = 0 ; i < zombieVecSize; i++) {
      if ( zombies_[i].path_to_task() == path_to_task) {
#ifdef DEBUG_ZOMBIE
         std::cout << "   ZombieCtrl::remove_by_path : " << zombies_[i] << " \n";
#endif
         zombies_.erase( zombies_.begin() + i);
         return true;
      }
   }
   return false;
}

/// Query
const Zombie& ZombieCtrl::find(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password) const
{
	size_t zombieVecSize = zombies_.size();
	for(size_t i = 0 ; i < zombieVecSize; i++) {
		if (match(zombies_[i],path_to_task,process_or_remote_id,password)) {
 			return zombies_[i];
		}
	}
	return Zombie::EMPTY();
}

//================= private ===================================================================

Zombie& ZombieCtrl::find(Submittable* task)
{
	if (task) return find_zombie(task->absNodePath(),task->process_or_remote_id(),task->jobsPassword());
	return Zombie::EMPTY_();
}

const Zombie& ZombieCtrl::find(Submittable* task) const
{
	if (task) return find(task->absNodePath(),task->process_or_remote_id(),task->jobsPassword());
	return Zombie::EMPTY();
}

Zombie& ZombieCtrl::find_zombie(const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password)
{
	size_t zombieVecSize = zombies_.size();
	for(size_t i = 0 ; i < zombieVecSize; i++) {
		if (match(zombies_[i],path_to_task,process_or_remote_id,password)) {
 			return zombies_[i];
 		}
	}
	return find_by_path(path_to_task);
}


bool match(const Zombie& z, const std::string& path_to_task, const std::string& process_or_remote_id, const std::string& password)
{
	///  Process/remote id only created when task becomes active.
	if (z.path_to_task() == path_to_task) {
		if (process_or_remote_id.empty() || z.process_or_remote_id().empty()) {
			if (z.jobs_password() == password) {
				return true;
			}
		}
		else  if ( z.process_or_remote_id() == process_or_remote_id ) {
			return true;
		}
	}
	return false;
}

Zombie& ZombieCtrl::find_by_path(const std::string& path_to_task)
{
	size_t zombieVecSize = zombies_.size();
	for(size_t i = 0 ; i < zombieVecSize; i++) {
		if (zombies_[i].path_to_task() == path_to_task) {
  			return zombies_[i];
		}
	}
	return Zombie::EMPTY_();
}

const Zombie& ZombieCtrl::find_by_path_only(const std::string& path_to_task) const
{
	size_t zombieVecSize = zombies_.size();
	for(size_t i = 0 ; i < zombieVecSize; i++) {
		if (zombies_[i].path_to_task() == path_to_task) {
  			return zombies_[i];
		}
	}
	return Zombie::EMPTY();
}
