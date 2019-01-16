/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #91 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "TaskApi.hpp"
#include "ExprAst.hpp"
#include "ExprAstVisitor.hpp"

#include "Defs.hpp"
#include "Task.hpp"
#include "SuiteChanged.hpp"
#include "Log.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//#define DEBUG_ZOMBIE 1

////////////////////////////////////////////////////////////////////////////////////////////////
bool TaskCmd::equals(ClientToServerCmd* rhs) const
{
	TaskCmd* the_rhs = dynamic_cast<TaskCmd*>(rhs);
	if (!the_rhs) return false;
	if (path_to_submittable_ !=  the_rhs->path_to_node()) return false;
	if (jobs_password_ !=  the_rhs->jobs_password()) return false;
	if (process_or_remote_id_ !=  the_rhs->process_or_remote_id()) return false;
	if (try_no_ !=  the_rhs->try_no()) return false;

	// avoid calling base equals, as task cmd's don't use User(user_) variables.
	return true;
}

// **********************************************************************************
// IMPORTANT: In the current SMS/ECF only the init child command, passes the
// process_or_remote_id_, for *ALL* other child commands this is empty.
// The Automated tests get round this via setting ECF_RID in the header/tail job includes
// However since this may not be in .sms/.ecf files, we can not rely on it
// Hence we need to be able to handle *EMPTY* process_or_remote_id_ for child commands
// ************************************************************************************
bool TaskCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& theReply) const
{
#ifdef DEBUG_ZOMBIE
	std::cout << "   TaskCmd::authenticate " << Child::to_string(child_type());
 	std::cout << " " << path_to_submittable_ << " " << process_or_remote_id_  << " " << jobs_password_ << " " << try_no_;
 	const Zombie& zombie = as->zombie_ctrl().find(path_to_submittable_,process_or_remote_id_,jobs_password_);
 	if (!zombie.empty()) std::cout << "  " << zombie;
 	else {
 	 	const Zombie& zombie = as->zombie_ctrl().find_by_path_only(path_to_submittable_);
 	 	if (!zombie.empty()) std::cout << "  find_by_path_only: " << zombie;
 	}
#endif
	/// ***************************************************************************
	/// Task based cmd have their own authentication mechanism, hence we
	/// Don't need to call the base class authenticate
	/// **************************************************************************

	if (!as->allowTaskCommunication()) {
  		// This is not an Error, hence we don't throw exception
		theReply = PreAllocatedReply::block_client_server_halted_cmd();
		return false;
	}

 	submittable_ = get_submittable(as);
	if (!submittable_) {
#ifdef DEBUG_ZOMBIE
	   std::cout << ": PATH Zombie\n";
#endif
		// Create path zombie, if not already created:
	   std::string action_taken;
		(void)as->zombie_ctrl().handle_path_zombie(as,this,action_taken,theReply);

		// distinguish output by using *path*
		std::stringstream ss;
		ss << " zombie(*path*) : chd:" << ecf::Child::to_string(child_type()) << " : " << path_to_submittable_ << " : "<< process_or_remote_id_ << " : " << jobs_password_ << " : action(" << action_taken << ")";
		log(Log::ERR,ss.str());
		return false;
	}

	// If the CMD *WAS* created with Submittable::DUMMY_JOBS_PASSWORD then we were testing
	// This will be copied from client to server, hence we want to avoid same check in the
	// server. when handleRequest is called()
	// DO NOT place #ifdef DEBUG otherwise test will start failing for the release build
	if ( jobs_password_ == Submittable::DUMMY_JOBS_PASSWORD()) {
		return true;
	}

   SuiteChanged1 changed(submittable_->suite());

	/// Check if User wants to explicitly bypass password checking
	/// This can be done via AlterCmd by adding a variable on the task,  ECF_PASS with value Submittable::FREE_JOBS_PASSWORD
	/// Note: this *does not* look for the variable up the node tree, only on the task.
	std::string ecf_pass_value;
	if (submittable_->findVariableValue(Str::ECF_PASS(), ecf_pass_value)) {

	   if ( ecf_pass_value == Submittable::FREE_JOBS_PASSWORD()) {
	      submittable_->flag().clear(ecf::Flag::ZOMBIE);
	      return true;
	   }
	}

  	/// Handle corner case ,where we have two jobs with different process id, but same password
  	/// Can happen if jobs is started externally,
	/// or via test(i.e submit 1,submit 2(force)) before job1 active its password is overridden
   bool submittable_allready_aborted = false;
   bool submittable_allready_active = false;
   bool submittable_allready_complete = false;
   bool password_missmatch = false;
   bool pid_missmatch = false;

   if ( submittable_->jobsPassword() != jobs_password_) {
#ifdef DEBUG_ZOMBIE
      std::cout << ": submittable pass(" << submittable_->jobsPassword() << ") != jobs_password_(" << jobs_password_ << ")";
#endif
      password_missmatch = true;
   }

   /// When state is in SUBMITTED, its process/remote_id is EMPTY. It will be set by the INIT child command.
   /// Hence we can *NOT* mark it as pid_missmatch.
   ///
   /// *** See Note above: Not all child commands pass a process_id. ***
   /// *** Hence this test for zombies is ONLY valid if process sets the process_or_remote_id_ ****
   /// *** User should really set ECF_RID in the scripts
   if (!submittable_->process_or_remote_id().empty() && !process_or_remote_id_.empty() && submittable_->process_or_remote_id() != process_or_remote_id_) {
#ifdef DEBUG_ZOMBIE
      std::cout << ":task pid(" << submittable_->process_or_remote_id() << ") != process pid(" << process_or_remote_id_ << ")";
#endif
      pid_missmatch = true;
   }

   NState::State submittable_state = submittable_->state();
   if ((child_type() == Child::INIT) && (submittable_state == NState::ACTIVE)) {
#ifdef DEBUG_ZOMBIE
      std::cout << ":(child_type() == Child::INIT) && submittable_state == NState::ACTIVE)";
#endif

      if (!password_missmatch && !pid_missmatch ) {
         std::stringstream ss;
         ss << " [ overloaded || --init*2 ] (pid & password match) : chd:" << ecf::Child::to_string(child_type()) << " : "  << path_to_submittable_ << " : already active : action(fob)";
         log(Log::WAR, ss.str() );
         theReply = PreAllocatedReply::ok_cmd();
         return false;
      }

      submittable_allready_active = true;
   }

   if ( submittable_state == NState::COMPLETE) {
#ifdef DEBUG_ZOMBIE
      std::cout << ": submittable_state == NState::COMPLETE)";
#endif
      if (child_type() == Child::COMPLETE) {
         // Note: when a node completes, we clear tasks password and pid, to save memory on checkpt & network bandwidth
         // (We could choose not to clear, This would allow us to disambiguate between 2/ and 3/ below). HOWEVER:
         //
         // How can this situation arise:
         //   1/ Two calls to --complete  (rare)
         //   2/ Overloaded server. Client send --complete to server, but it is overload and does not respond, the client then
         //      times out. Server handles the request. When client tries again we get here. (possible)
         //   3/ Zombie, two separate process. (possible, typically done by user action)
         //
         // For all three it should be safe to just fob:
         //   1/ Two calls to --complete # Be forgiving
         //   2/ Overloaded server       # The correct course of action
         //   3/ zombie                  # The zombie has completed anyway, don't bother blocking it

         submittable_->flag().clear(ecf::Flag::ZOMBIE);
         as->zombie_ctrl().remove_by_path( path_to_submittable_ ); // remove any associated zombies

         std::stringstream ss;
         ss << " [ overloaded || zombie || --complete*2 ] : chd:" << ecf::Child::to_string(child_type()) << " : " << path_to_submittable_ ;
         ss << " : already complete : action(fob)";
         log(Log::WAR, ss.str() );
         theReply = PreAllocatedReply::ok_cmd();
         return false;
      }

      // If Task state is complete, and we receive **any** child command then it is a zombie
      submittable_allready_complete = true;
   }

   if ( submittable_state == NState::ABORTED) {
#ifdef DEBUG_ZOMBIE
      std::cout << ": submittable_state == NState::ABORTED)";
#endif

      if (child_type() == Child::ABORT) {
         if (!password_missmatch && !pid_missmatch ) {
            /// If there is an associated zombie, remove from the list
            as->zombie_ctrl().remove( submittable_ );

            std::stringstream ss;
            ss << " [ overloaded || --abort*2 ] (pid & password match) : chd:" << ecf::Child::to_string(child_type()) << " : " << path_to_submittable_ << " : already aborted : action(fob)";
            log(Log::WAR, ss.str() );
            theReply = PreAllocatedReply::ok_cmd();
            return false;
         }
      }

      // If Task state is aborted, and we receive **any** child command then it is a zombie
      submittable_allready_aborted = true;
   }

#ifdef DEBUG_ZOMBIE
    std::cout << "\n";
#endif

  	if (password_missmatch || pid_missmatch || submittable_allready_active || submittable_allready_complete || submittable_allready_aborted){
		/// If the task has adopted we return true, and carry on as normal
      std::string action_taken;
  		if (!as->zombie_ctrl().handle_zombie(submittable_,this,action_taken,theReply)) {

  		   // LOG failure: Include type of zombie.
  		   // ** NOTE **: the zombie may have been removed by user actions. i.e if fob and child cmd is abort | complete, etc
  		   std::stringstream ss;
  		   ss << " zombie";
  		   const Zombie& theZombie = as->zombie_ctrl().find(path_to_submittable_, process_or_remote_id_, jobs_password_ );
  		   if (!theZombie.empty() ) ss << "(" << theZombie.type_str() << ")";

         ss << " : chd:" << ecf::Child::to_string(child_type());
         ss << " : " << path_to_submittable_ << "(" << NState::toString(submittable_state) << ")";
         ss << " : " << process_or_remote_id_  << " : " << jobs_password_;
         if (submittable_allready_active)   ss << " : already active";
         if (submittable_allready_complete) ss << " : already complete";
         if (submittable_allready_aborted)  ss << " : already aborted";
         if (password_missmatch) ss << " : passwd != [ task:"<< submittable_->jobsPassword()<<" child:" << jobs_password_ << " ]";
         if (pid_missmatch)      ss << " : pid != [ task:"<< submittable_->process_or_remote_id()<<" child:" << process_or_remote_id_ << " ]";
         ss << " : action(" << action_taken << ")";
         log(Log::ERR,ss.str());
  			return false;
  		}
  	}
	return true;
}

Submittable* TaskCmd::get_submittable(AbstractServer* as) const
{
	node_ptr node = as->defs()->findAbsNode(path_to_submittable_);
	if (!node.get()) {
		return NULL;
 	}

	return node->isSubmittable();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& InitCmd::print(std::ostream& os) const
{
   return os << Str::CHILD_CMD() << "init " << path_to_node();
}

bool InitCmd::equals(ClientToServerCmd* rhs) const
{
	InitCmd* the_rhs = dynamic_cast<InitCmd*>(rhs);
	if (!the_rhs) return false;
	return TaskCmd::equals(rhs);
}

STC_Cmd_ptr InitCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().task_init_++;

	{   // update suite change numbers before job submission. submittable_ setup during authentication
		SuiteChanged1 changed(submittable_->suite());
		submittable_->init(process_or_remote_id());    // will set task->set_state(NState::ACTIVE);
	}

	// Do job submission in case any triggers dependent on NState::ACTIVE
	as->increment_job_generation_count();
   return PreAllocatedReply::ok_cmd();
}

const char* InitCmd::arg() { return TaskApi::initArg();}
const char* InitCmd::desc() {
   return
            "Mark task as started(active). For use in the '.ecf' script file *only*\n"
            "Hence the context is supplied via environment variables.\n"
            "  arg = process_or_remote_id. The process id of the job or remote_id\n"
            "                              Using remote id allows the jobs to be killed\n\n"
            "If this child command is a zombie, then the default action will be to *block*.\n"
            "The default can be overridden by using zombie attributes.\n"
            "Otherwise the blocking period is defined by ECF_TIMEOUT.\n\n"
            "Usage:\n"
            "  ecflow_client --init=$$"
 	;
}

void InitCmd::addOption(boost::program_options::options_description& desc) const{
	desc.add_options()( InitCmd::arg(), po::value< string >(), InitCmd::desc() );
}

void InitCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* clientEnv ) const
{
	std::string process_or_remote_id = vm[ arg() ].as< std::string > ();

	if (clientEnv->debug())
		cout << "  InitCmd::create " << InitCmd::arg()
		<< "  clientEnv->task_path(" << clientEnv->task_path()
		<< ") clientEnv->jobs_password(" << clientEnv->jobs_password()
		<< ") clientEnv->process_or_remote_id(" << clientEnv->process_or_remote_id()
		<< ") clientEnv->task_try_no(" << clientEnv->task_try_no()
      << ") process_or_remote_id(" << process_or_remote_id
      << ") clientEnv->under_test(" << clientEnv->under_test()
		<< ")\n";

	std::string errorMsg;
	if ( !clientEnv->checkTaskPathAndPassword(errorMsg) ) {
	 	throw std::runtime_error( "InitCmd: " + errorMsg );
	}

	/// if ECF_RID is specified then it *MUST* be the same as input argument
	/// On cca we ECF_RID can be specified under test, and therefore fail this check, hence we use clientEnv->under_test()
 	if (!clientEnv->under_test() && !clientEnv->process_or_remote_id().empty() && clientEnv->process_or_remote_id() != process_or_remote_id) {
 		std::stringstream ss;
 		ss << "remote id(" << process_or_remote_id << ") passed as an argument, not the same the client environment ECF_RID(" << clientEnv->process_or_remote_id() << ")";
		throw std::runtime_error(ss.str());
	}

	cmd = Cmd_ptr( new InitCmd( clientEnv->task_path(),
	                            clientEnv->jobs_password(),
	                            process_or_remote_id,
	                            clientEnv->task_try_no()
	                          )
	             );
}
//////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& CompleteCmd::print(std::ostream& os) const
{
   return os << Str::CHILD_CMD() << "complete " << path_to_node();
}

bool CompleteCmd::equals(ClientToServerCmd* rhs) const
{
	CompleteCmd* the_rhs = dynamic_cast<CompleteCmd*>(rhs);
	if (!the_rhs) return false;
 	return TaskCmd::equals(rhs);
}

STC_Cmd_ptr CompleteCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().task_complete_++;

	{
      /// If there is an associated zombie, remove from the list. Must match,
	   /// Do this before task->complete(), since that clears password & process id
	   /// remove(..) uses password/ process id to match the right zombie
      as->zombie_ctrl().remove( submittable_ );

      // update suite change numbers before job submission, submittable_ setup during authentication
		SuiteChanged1 changed(submittable_->suite());
		submittable_->complete();          // will set task->set_state(NState::COMPLETE);
	}

	// Do job submission in case any triggers dependent on NState::COMPLETE
   as->increment_job_generation_count();
   return PreAllocatedReply::ok_cmd();
}

const char* CompleteCmd::arg() { return TaskApi::completeArg();}
const char* CompleteCmd::desc()
{
	return
	         "Mark task as complete. For use in the '.ecf' script file *only*\n"
	         "Hence the context is supplied via environment variables\n\n"
	         "If this child command is a zombie, then the default action will be to *block*.\n"
	         "The default can be overridden by using zombie attributes.\n"
	         "Otherwise the blocking period is defined by ECF_TIMEOUT.\n\n"
	         "Usage:\n"
	         "  ecflow_client --complete"
	         ;
}

void CompleteCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( CompleteCmd::arg(), CompleteCmd::desc() );
}
void CompleteCmd::create( 	Cmd_ptr& cmd,
							boost::program_options::variables_map& vm,
							AbstractClientEnv* clientEnv ) const
{
	if (clientEnv->debug())
		cout << "  CompleteCmd::create " << CompleteCmd::arg()
		<< " task_path(" << clientEnv->task_path()
		<< ") password(" << clientEnv->jobs_password()
		<< ") remote_id(" << clientEnv->process_or_remote_id()
		<< ") try_no(" << clientEnv->task_try_no()
		<< ")\n";


	std::string errorMsg;
	if ( !clientEnv->checkTaskPathAndPassword(errorMsg) ) {
	 	throw std::runtime_error( "CompleteCmd: " + errorMsg );
	}

	cmd = Cmd_ptr( new CompleteCmd( clientEnv->task_path(),
	                                clientEnv->jobs_password(),
	                                clientEnv->process_or_remote_id(),
	                                clientEnv->task_try_no()) );
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CtsWaitCmd::CtsWaitCmd(const std::string& pathToTask,
          const std::string& jobsPassword,
          const std::string& process_or_remote_id,
          int try_no,
          const std::string& expression)
 : TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no), expression_(expression)
{
   // Parse expression to make sure its valid
   (void)Expression::parse(expression,"CtsWaitCmd:"); // will throw for errors
}

std::ostream& CtsWaitCmd::print(std::ostream& os) const
{
	return os << Str::CHILD_CMD() << "wait " << expression_ << " " << path_to_node();
}

bool CtsWaitCmd::equals(ClientToServerCmd* rhs) const
{
	CtsWaitCmd* the_rhs = dynamic_cast< CtsWaitCmd* > ( rhs );
	if ( !the_rhs ) return false;
 	if (expression_ != the_rhs->expression()) return false;
	return TaskCmd::equals(rhs);
}

STC_Cmd_ptr CtsWaitCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().task_wait_++;

	SuiteChanged1 changed(submittable_->suite());

	// Parse the expression, should not fail since client should have already check expression parses
	// The complete expression have been parsed and we have created the abstract syntax tree
	// We now need CHECK the AST for path nodes, event and meter. repeats,etc.
	// *** This will also set the Node pointers ***
	// If the expression references paths that don't exist throw an error
	// This can be captured in the ecf script, which should then abort the task
	// Otherwise we will end up blocking indefinitely
	std::auto_ptr<AstTop> ast = submittable_->parse_and_check_expressions(expression_,true,"CtsWaitCmd:" ); // will throw for errors

	// Evaluate the expression
	if ( ast->evaluate() ) {

	   submittable_->flag().clear(ecf::Flag::WAIT);

	   // expression evaluates, return OK
	   return PreAllocatedReply::ok_cmd();
	}

	submittable_->flag().set(ecf::Flag::WAIT);

	// Block/wait while expression is false
	return PreAllocatedReply::block_client_on_home_server_cmd();
}

const char* CtsWaitCmd::arg()  { return TaskApi::waitArg();}
const char* CtsWaitCmd::desc() {
	return
	         "Evaluates an expression, and block while the expression is false.\n"
	         "For use in the '.ecf' file *only*, hence the context is supplied via environment variables\n"
	         "  arg1 = string(expression)\n"
	         "Usage:\n"
	         "  ecflow_client --wait=\"/suite/taskx == complete\""
	;
}

void CtsWaitCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( CtsWaitCmd::arg(), po::value< string >(),  CtsWaitCmd::desc() );
}
void CtsWaitCmd::create( 	Cmd_ptr& cmd,
							boost::program_options::variables_map& vm,
							AbstractClientEnv* clientEnv ) const
{
	std::string expression = vm[ arg() ].as< std::string > ();

	if (clientEnv->debug())
		cout << "  CtsWaitCmd::create " << CtsWaitCmd::arg()
		<< " task_path(" << clientEnv->task_path()
		<< ") password(" << clientEnv->jobs_password()
		<< ") remote_id(" << clientEnv->process_or_remote_id()
		<< ") try_no(" << clientEnv->task_try_no()
		<< ") expression(" << expression << ")\n";

	std::string errorMsg;
	if ( !clientEnv->checkTaskPathAndPassword(errorMsg) ) {
	 	throw std::runtime_error( "CtsWaitCmd: " + errorMsg );
	}

	cmd = Cmd_ptr( new CtsWaitCmd( clientEnv->task_path(),
	                               clientEnv->jobs_password(),
	                               clientEnv->process_or_remote_id(),
	                               clientEnv->task_try_no(),
	                               expression) );
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AbortCmd::AbortCmd(const std::string& pathToTask,
         const std::string& jobsPassword,
         const std::string& process_or_remote_id,
         int try_no,
         const std::string& reason)
:TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no), reason_(reason)
{
   if (!reason_.empty()) {
      // Do not use "\n" | ';' in abortedReason_, as this can mess up, --migrate output
      // Which would then affect --load.
      Str::replace(reason_,"\n","");
      Str::replace(reason_,";"," ");
   }
}

std::ostream& AbortCmd::print(std::ostream& os) const
{
   return os << Str::CHILD_CMD() << "abort " << path_to_node() << "  " << reason_;
}

bool AbortCmd::equals(ClientToServerCmd* rhs) const
{
	AbortCmd* the_rhs = dynamic_cast<AbortCmd*>(rhs);
	if (!the_rhs) return false;
 	if (reason_ != the_rhs->reason()) return false;
 	return TaskCmd::equals(rhs);
}

STC_Cmd_ptr AbortCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().task_abort_++;

	assert(isWrite()); // isWrite used in handleRequest() to control check pointing

	{
      /// If there is an associated zombie, remove from the list
      as->zombie_ctrl().remove( submittable_ );

	   // update suite change numbers before job submission, submittable_ setup during authentication
		SuiteChanged1 changed(submittable_->suite());

		string theReason = reason_;
		if ( theReason.empty() ) theReason = "Trap raised in job file";

		submittable_->aborted(theReason);  // will set task->set_state(NState::ABORTED);
	}

	// Do job submission in case any triggers dependent on NState::ABORTED
	// If task try number is less than ECF_TRIES we attempt to re-submit the job.(ie if still in limit)
   as->increment_job_generation_count();
   return PreAllocatedReply::ok_cmd();
}

const char* AbortCmd::arg()  { return TaskApi::abortArg();}
const char* AbortCmd::desc() {
   return
            "Mark task as aborted. For use in the '.ecf' script file *only*\n"
            "Hence the context is supplied via environment variables\n"
            "  arg1 = (optional) string(reason)\n"
            "         Optionally provide a reason why the abort was raised\n\n"
            "If this child command is a zombie, then the default action will be to *block*.\n"
            "The default can be overridden by using zombie attributes.\n"
            "Otherwise the blocking period is defined by ECF_TIMEOUT.\n\n"
            "Usage:\n"
            "  ecflow_client --abort=reasonX"
            ;
}

void AbortCmd::addOption(boost::program_options::options_description& desc) const{
	desc.add_options()( AbortCmd::arg(), po::value< string >()->implicit_value(string()), AbortCmd::desc() );
}
void AbortCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* clientEnv ) const
{
	std::string reason = vm[ arg() ].as< std::string > ();

	if (clientEnv->debug())
		cout << "  AbortCmd::create " << AbortCmd::arg()
		<< " task_path(" << clientEnv->task_path()
		<< ") password(" << clientEnv->jobs_password()
		<< ") remote_id(" << clientEnv->process_or_remote_id()
		<< ") try_no(" << clientEnv->task_try_no()
		<< ") reason(" << reason << ")\n";


	std::string errorMsg;
	if ( !clientEnv->checkTaskPathAndPassword(errorMsg) ) {
	 	throw std::runtime_error( "AbortCmd: " + errorMsg );
	}

	cmd = Cmd_ptr(new AbortCmd( clientEnv->task_path(),
 	                            clientEnv->jobs_password(),
 	                            clientEnv->process_or_remote_id(),
 	                            clientEnv->task_try_no(),
 	                            reason));
}
//////////////////////////////////////////////////////////////////////////////////////////////////

bool EventCmd::equals(ClientToServerCmd* rhs) const
{
	EventCmd* the_rhs = dynamic_cast<EventCmd*>(rhs);
	if (!the_rhs) return false;
	if (name_ != the_rhs->name()) return false;
  	return TaskCmd::equals(rhs);
}

std::ostream& EventCmd::print(std::ostream& os) const
{
 	return os << Str::CHILD_CMD() << "event " << name_ << " " << path_to_node();
}

STC_Cmd_ptr EventCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().task_event_++;

	{   // update suite change numbers before job submission,  task_ setup during authentication
		SuiteChanged1 changed(submittable_->suite());

		// The name could either be "string" or an integer either way it should be unique
		if (!submittable_->set_event(name_,true)) {
			std::string ss; ss = "Event request failed as event '"; ss += name_; ss += "' does not exist on task "; ss += path_to_node();
			ecf::log(Log::ERR,ss);
			return PreAllocatedReply::ok_cmd();
		}
	}

 	// Do job submission in case any triggers dependent on events
   as->increment_job_generation_count();
   return PreAllocatedReply::ok_cmd();
}

const char* EventCmd::arg()  { return TaskApi::eventArg();}
const char* EventCmd::desc() {
   return
            "Change event. For use in the '.ecf' script file *only*\n"
            "Hence the context is supplied via environment variables\n"
            "  arg1(string | int) = event-name\n\n"
            "If this child command is a zombie, then the default action will be to *fob*,\n"
            "i.e allow the ecflow client command to complete without an error\n"
            "The default can be overridden by using zombie attributes.\n\n"
            "Usage:\n"
            "  ecflow_client --event=ev"
            ;
}

void EventCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( EventCmd::arg(), po::value< string >(), EventCmd::desc() );
}
void EventCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* clientEnv ) const
{
	std::string event = vm[ arg() ].as< std::string > ();

	if (clientEnv->debug())
		cout << "  EventCmd::create " << EventCmd::arg()
		<< " task_path(" << clientEnv->task_path()
		<< ") password(" << clientEnv->jobs_password()
		<< ") remote_id(" << clientEnv->process_or_remote_id()
		<< ") try_no(" << clientEnv->task_try_no()
		<< ") event(" << event << ")\n";


	std::string errorMsg;
	if ( !clientEnv->checkTaskPathAndPassword(errorMsg) ) {
	 	throw std::runtime_error( "EventCmd: " + errorMsg );
	}

  	cmd = Cmd_ptr(new EventCmd( clientEnv->task_path(),
  	                            clientEnv->jobs_password(),
  	                            clientEnv->process_or_remote_id(),
  	                            clientEnv->task_try_no(),
  	                            event ));
}
//////////////////////////////////////////////////////////////////////////////////////////////////

bool MeterCmd::equals(ClientToServerCmd* rhs) const
{
	MeterCmd* the_rhs = dynamic_cast<MeterCmd*>(rhs);
	if (!the_rhs) return false;
	if (name_ != the_rhs->name()) return false;
	if (value_ != the_rhs->value()) return false;
 	return TaskCmd::equals(rhs);
}

std::ostream& MeterCmd::print(std::ostream& os) const
{
	return os << Str::CHILD_CMD() << "meter " << name_ << " " << value_ << " " << path_to_node();
}

STC_Cmd_ptr MeterCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().task_meter_++;

	{  // Added scope for SuiteChanged1 changed: i.e update suite change numbers before job submission
		// submittable_ setup during authentication
		SuiteChanged1 changed(submittable_->suite());

		/// Allow meter to set any valid value that is in range because:
		///   - When we have a network failure, and restoration. The meter tasks, will come in random, order.
		///   - When task is executed without a requee the meter value will less than maximum
		///
		/// This has *IMPLICATION*, if the meter is used in a trigger, using a equality
		/// operator, then the trigger will always hold.  hence suite designers need to
		/// aware of this.
		try {

	      Meter& the_meter = submittable_->find_meter(name_);
	      if (the_meter.empty()) {
	         LOG(Log::ERR,"MeterCmd::doHandleRequest: failed as meter '"  << name_ << "' does not exist on task " << path_to_node());
	         return PreAllocatedReply::ok_cmd();
	      }

		   /// Invalid meter values(out or range) will raise exceptions.
	      /// Just ignore the request rather than failing client cmd
		   the_meter.set_value(value_);
		}
		catch (std::exception& e) {
         LOG(Log::ERR,"MeterCmd::doHandleRequest: failed for task " << path_to_node() << ". " << e.what());
         return PreAllocatedReply::ok_cmd();
		}
	}

 	// Do job submission in case any triggers dependent on meters
   as->increment_job_generation_count();
   return PreAllocatedReply::ok_cmd();
}

const char* MeterCmd::arg()  { return TaskApi::meterArg();}
const char* MeterCmd::desc() {
   return
            "Change meter. For use in the '.ecf' script file *only*\n"
            "Hence the context is supplied via environment variables\n"
            "  arg1(string) = meter-name\n"
            "  arg2(int)    = the new meter value\n\n"
            "If this child command is a zombie, then the default action will be to *fob*,\n"
            "i.e allow the ecflow client command to complete without an error\n"
            "The default can be overridden by using zombie attributes.\n\n"
            "Usage:\n"
            "  ecflow_client --meter=my_meter 20"
            ;
}

void MeterCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( MeterCmd::arg(), po::value< vector<string> >()->multitoken(), MeterCmd::desc() );
}
void MeterCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* clientEnv ) const
{
	vector<string> args = vm[ arg() ].as< vector<string> >();

	if (clientEnv->debug()) {
		dumpVecArgs(MeterCmd::arg(),args);
		cout << "  MeterCmd::create " << MeterCmd::arg()
		<< " task_path(" << clientEnv->task_path()
		<< ") password(" << clientEnv->jobs_password()
		<< ") remote_id(" << clientEnv->process_or_remote_id()
		<< ") try_no(" << clientEnv->task_try_no()
		<< ")\n";
	}

	if (args.size() != 2 ) {
		std::stringstream ss;
		ss << "MeterCmd: Two arguments expected, found " << args.size()
		   << " Please specify <meter-name> <meter-value>, ie --meter=name 100\n";
		throw std::runtime_error( ss.str() );
 	}

	int value = 0;
	try {
		std::string strVal = args[1];
		value = boost::lexical_cast<int>(strVal);
	}
	catch (boost::bad_lexical_cast& e) {
 		throw std::runtime_error( "MeterCmd: Second argument must be a integer, i.e. --meter=name 100\n" );
 	}

	std::string errorMsg;
	if ( !clientEnv->checkTaskPathAndPassword(errorMsg) ) {
	 	throw std::runtime_error( "MeterCmd: " + errorMsg );
	}

	cmd = Cmd_ptr(new MeterCmd( clientEnv->task_path(),
	                            clientEnv->jobs_password(),
	                            clientEnv->process_or_remote_id(),
	                            clientEnv->task_try_no(),
	                            args[0],
	                            value ));
}
//////////////////////////////////////////////////////////////////////////////////////////////////

bool LabelCmd::equals(ClientToServerCmd* rhs) const
{
	LabelCmd* the_rhs = dynamic_cast<LabelCmd*>(rhs);
	if (!the_rhs) return false;
	if (name_ != the_rhs->name()) return false;
	if (label_ != the_rhs->label()) return false;
  	return TaskCmd::equals(rhs);
}

std::ostream& LabelCmd::print(std::ostream& os) const
{
	return os << Str::CHILD_CMD() << "label " << name_ << " '" << label_  << "' " << path_to_node();
}

STC_Cmd_ptr LabelCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().task_label_++;

	assert(isWrite()); // isWrite used in handleRequest() to control check pointing

	// submittable_ setup during authentication
	if (submittable_->findLabel(name_)) {

	   SuiteChanged1 changed(submittable_->suite());
	   submittable_->changeLabel(name_,label_);
	}
	// else {
	//   // ECFLOW-175, avoid filling up log file. Can get thousands of these messages, especially form MARS
	//   std::string ss;
	//   ss = "Label request failed as label '"; ss += name_; ss += "' does not exist on task "; ss += path_to_node();
	//	  ecf::log(Log::ERR,ss);
	//}

	// Note: reclaiming memory for label_ earlier make *no* difference to performance of server

	return PreAllocatedReply::ok_cmd();
}

const char* LabelCmd::arg()  { return TaskApi::labelArg();}
const char* LabelCmd::desc() {
   return
            "Change Label. For use in the '.ecf' script file *only*\n"
            "Hence the context is supplied via environment variables\n"
            "  arg1 = label-name\n"
            "  arg2 = The new label value\n"
            "         The labels values can be single or multi-line(space separated quoted strings)\n\n"
            "If this child command is a zombie, then the default action will be to *fob*,\n"
            "i.e allow the ecflow client command to complete without an error\n"
            "The default can be overridden by using zombie attributes.\n\n"
            "Usage:\n"
            "  ecflow_client --label=progressed merlin"
            ;
}

void LabelCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( LabelCmd::arg(), po::value< vector<string> >()->multitoken(), LabelCmd::desc() );
}
void LabelCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* clientEnv ) const
{
	vector<string> args = vm[  arg() ].as< vector<string> >();

	if (clientEnv->debug()) {
		dumpVecArgs(LabelCmd::arg(),args);
		cout << "  LabelCmd::create " << LabelCmd::arg()
		<< " task_path(" << clientEnv->task_path()
		<< ") password(" << clientEnv->jobs_password()
		<< ") remote_id(" << clientEnv->process_or_remote_id()
		<< ") try_no(" << clientEnv->task_try_no()
		<< ")\n";
	}

	if (args.size() < 2 ) {
		std::stringstream ss;
		ss << "LabelCmd: At least 2 arguments expected. Please specify <label-name> <string1> <string2>\n";
		throw std::runtime_error( ss.str() );
 	}

	std::string labelName = args[0];
	args.erase(args.begin()); // remove name from vector of strings
	std::string labelValue;
	for(size_t i =0; i < args.size(); i++) {
		if (i != 0) labelValue += " ";
		labelValue += args[i];
	}

	std::string errorMsg;
	if ( !clientEnv->checkTaskPathAndPassword(errorMsg) ) {
	 	throw std::runtime_error( "LabelCmd: " + errorMsg );
	}

	cmd = Cmd_ptr(new LabelCmd( clientEnv->task_path(),
	                            clientEnv->jobs_password(),
	                            clientEnv->process_or_remote_id(),
	                            clientEnv->task_try_no(),
	                            labelName,
	                            labelValue));
}

std::ostream& operator<<(std::ostream& os, const InitCmd& c)        { return c.print(os); }
std::ostream& operator<<(std::ostream& os, const EventCmd& c)       { return c.print(os); }
std::ostream& operator<<(std::ostream& os, const MeterCmd& c)       { return c.print(os); }
std::ostream& operator<<(std::ostream& os, const LabelCmd& c)       { return c.print(os); }
std::ostream& operator<<(std::ostream& os, const AbortCmd& c)       { return c.print(os); }
std::ostream& operator<<(std::ostream& os, const CompleteCmd& c)    { return c.print(os); }
std::ostream& operator<<(std::ostream& os, const CtsWaitCmd& c)     { return c.print(os); }
