//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #37 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <assert.h>
#include <sstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/exception.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include "Submittable.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "File.hpp"
#include "Passwd.hpp"
#include "Stl.hpp"
#include "Str.hpp"
#include "Indentor.hpp"
#include "Log.hpp"
#include "System.hpp"
#include "JobsParam.hpp"
#include "EcfFile.hpp"
#include "Ecf.hpp"
#include "DefsDelta.hpp"
#include "Extract.hpp"

namespace fs = boost::filesystem;
using namespace ecf;
using namespace std;
using namespace boost;

//#define DEBUG_TASK_LOCATION 1

/////////////////////////////////////////////////////////////////////////////////////////////////////
// init static
const std::string& Submittable::DUMMY_JOBS_PASSWORD()        { static const std::string DUMMY_JOBS_PASSWORD = "_DJP_"; return DUMMY_JOBS_PASSWORD;}
const std::string& Submittable::FREE_JOBS_PASSWORD()         { static const std::string FREE_JOBS_PASSWORD = "FREE"; return FREE_JOBS_PASSWORD;}
const std::string& Submittable::DUMMY_PROCESS_OR_REMOTE_ID() { static const std::string DUMMY_PROCESS_OR_REMOTE_ID = "_RID_"; return DUMMY_PROCESS_OR_REMOTE_ID;}

Submittable& Submittable::operator=(const Submittable& rhs)
{
   if (this != &rhs) {
      Node::operator=(rhs);
      jobsPassword_ = rhs.jobsPassword_;
      process_or_remote_id_ = rhs.process_or_remote_id_;
      abortedReason_ = rhs.abortedReason_;
      tryNo_ = rhs.tryNo_;

      delete sub_gen_variables_;
      sub_gen_variables_ = NULL;

      state_change_no_ = Ecf::incr_state_change_no();
   }
   return *this;
}

Submittable::~Submittable()
{
   delete sub_gen_variables_;
}

void Submittable::init( const std::string& the_process_or_remote_id)
{
   set_state( NState::ACTIVE );
   set_process_or_remote_id(the_process_or_remote_id);

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Submittable::init\n";
#endif
}

void Submittable::complete()
{
// cout << "Completed " << debugNodePath() << " at " << suite()->calendar().toString() << "\n";
   set_state( NState::COMPLETE );
   flag().clear(ecf::Flag::ZOMBIE);

   /// Should we clear jobsPassword_ & process_id? It can be argued that
   /// we should keep this. In case it is needed. i.e The job may not really be
   /// complete. However keeping, this means the memory usage will continue to rise
   /// Dependent on number of tasks. This can affect network bandwidth as well.
   /// Hence to reduce network bandwidth we chose to clear the strings
   clear();       // jobs password, process_id, aborted_reason

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Submittable::complete()\n";
#endif
}

void Submittable::aborted(const std::string& reason)
{
   // Called during *abnormal* child termination
   // This will bubble the state, and decrement any limits
   set_aborted_only(reason);
}

void Submittable::set_process_or_remote_id(const std::string& id)
{
   process_or_remote_id_ = id;
   set_genvar_ecfrid(process_or_remote_id_);
   state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG
   if (tryNo_ == 0 && process_or_remote_id_ != Submittable::DUMMY_PROCESS_OR_REMOTE_ID()) {
      LogToCout logToCout;
      LOG(Log::ERR,"Submittable::set_process_or_remote_id: " << absNodePath() << " process_id(" << id << ") tryNo == 0, how can this be ???");
   }
#endif

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Submittable::set_process_or_remote_id\n";
#endif
}

void Submittable::reset()
{
   tryNo_ = 0;    // reset try number
   clear();       // jobs password, process_id, aborted_reason
   Node::reset();
}

void Submittable::begin()
{
   /// It is *very* important that we reset the passwords. This allows us to detect zombies.
   tryNo_ = 0;    // reset try number
   clear();       // jobs password, process_id, aborted_reason

   Node::begin(); // see  Notes in: Node::begin() about update_generated_variables()

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Submittable::begin()\n";
#endif
}

void Submittable::requeue(Requeue_args& args)
{
   /// It is *very* important that we reset the passwords. This allows us to detect zombies.
   tryNo_ = 0;    // reset try number
   clear();       // jobs password, process_id, aborted_reason

   Node::requeue(args);
   update_generated_variables();

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Submittable::requeue\n";
#endif
}


void Submittable::calendarChanged(const ecf::Calendar& c, std::vector<node_ptr>& auto_cancelled_nodes,const ecf::LateAttr* inherited_late)
{
   Node::calendarChanged(c,auto_cancelled_nodes,NULL);

   // Late flag should ONLY be set on Submittable
   check_for_lateness(c,inherited_late);
}

std::string Submittable::write_state() const
{
   // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
   //             multiple statement on a single line i.e.
   //                 task a; task b;
   std::string ret;
   if ( !jobsPassword_.empty() && jobsPassword_!= Submittable::DUMMY_JOBS_PASSWORD()) { ret += " passwd:"; ret += jobsPassword_;}
   if ( !process_or_remote_id_.empty() )  { ret += " rid:"; ret += process_or_remote_id_; }

   // The abortedReason_, can contain user generated messages, including \n and ;, hence remove these
   // as they can mess up the parsing on reload.
   if ( !abortedReason_.empty() ) {
      std::string the_abort_reason = abortedReason_;
      Str::replaceall(the_abort_reason,"\n","\\n");
      Str::replaceall(the_abort_reason,";"," ");
      ret += " abort<:"; ret += the_abort_reason; ret += ">abort";
   }
   if ( tryNo_ != 0) { ret += " try:"; ret += boost::lexical_cast<std::string>(tryNo_); }
   ret += Node::write_state();
   return ret;
}

void Submittable::read_state(const std::string& line,const std::vector<std::string>& lineTokens)
{
   //  0    1   2
   // task name #
   for(size_t i = 3; i < lineTokens.size(); i++) {
      if (lineTokens[i].find("passwd:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],jobsPassword_))
            throw std::runtime_error( "Submittable::read_state failed for jobs password : " + name());
      }
      else if (lineTokens[i].find("rid:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],process_or_remote_id_))
            throw std::runtime_error( "Submittable::read_state failed for rid : " + name());
      }
      else if (lineTokens[i].find("try:") != std::string::npos ) {
         std::string try_number;
         if (!Extract::split_get_second(lineTokens[i],try_number))
            throw std::runtime_error( "Submittable::read_state failed for try number : " + name());
         tryNo_ = Extract::theInt(try_number,"Submittable::read_state failed for try number");
      }
   }

   // extract aborted reason
   // The line tokens will truncate multiple spaces into a single space chars
   // Hence use the line to extract the abort reason: This will preserve spaces
   size_t first_pos = line.find("abort<:");
   size_t last_pos = line.find(">abort");
   if (first_pos != std::string::npos) {
      if ( last_pos != std::string::npos) {
         abortedReason_ = line.substr(first_pos+7, (last_pos-first_pos-7) );
      }
      else {
         throw std::runtime_error("Submittable::read_state failed for abort reason. Expected abort reason to on single line;");
      }
   }

   Node::read_state(line,lineTokens);
}


bool Submittable::operator==(const Submittable& rhs) const
{
   if ( jobsPassword_ != rhs.jobsPassword_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Submittable::operator==  jobsPassword_(" << jobsPassword_ << ") != rhs.jobsPassword_(" << rhs.jobsPassword_ << ") " << debugNodePath() << "\n";
         std::cout << "Submittable::operator==  state(" << NState::toString(state()) << ")  rhs.state(" << NState::toString(rhs.state()) << ") \n";
         // No point dumping out change numbers, since we don't persist them, hence values will always be zero on client side.
      }
#endif
      return false;
   }

   if ( process_or_remote_id_ != rhs.process_or_remote_id_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Submittable::operator==  process_or_remote_id_(" << process_or_remote_id_ << ") != rhs.process_or_remote_id_(" << rhs.process_or_remote_id_ << ") " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if ( tryNo_ != rhs.tryNo_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Submittable::operator==  tryNo_(" << tryNo_ << ") != rhs.tryNo_(" << rhs.tryNo_ << ") " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if ( abortedReason_ != rhs.abortedReason_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Submittable::operator==  abortedReason_(" << abortedReason_ << ") != rhs.abortedReason_(" << rhs.abortedReason_ << ") " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   return Node::operator==(rhs);
}

string Submittable::tryNo() const
{
   try {
      return boost::lexical_cast< string >( tryNo_ );
   }
   catch ( boost::bad_lexical_cast& e ) {}

   LOG_ASSERT(false,"Submittable::tryNo() corrupt?");
   return string();
}


EcfFile Submittable::locatedEcfFile() const
{
   // Look for scripts file(Task->.ecf | Alias->.usr) in the following order
   // o/ ECF_SCRIPT Generated VARIABLE  absolute path, check if files exists
   //    The variable MUST exist, but the file may not
   // o/ ECF_FETCH (user variable)
   //    if this variable exist, we need to flag it,
   //    So that that file is obtained from running the command. (Output of popen)
   // o/ ECF_SCRIPT_CMD (user variable)
   //    if this variable exist, we need to flag it,
   //    So that that file is obtained from running the command. (Output of popen)
   // o/ ECF_FILES (Typically in the definition file, defines a directory) (backward search)
   // o/ ECF_HOME (backward search)
   //    where: backward search is root_path = (ECF_FILES | ECF_HOME)
   //           <root-path>/suite/family/family2/task.ecf
   //           <root-path>/family/family2/task.ecf
   //           <root-path>/family2/task.ecf
   //           <root-path>/task.ecf
   std::string reasonEcfFileNotFound;
   std::string theAbsNodePath = absNodePath();
   std::string ecf_home;
   findParentUserVariableValue( Str::ECF_HOME(), ecf_home);

   /// Update local ECF_SCRIPT variable, ECF_SCRIPT is a generated variable. IT *MUST* exist
   /// Likewise generated variable like TASK and ECF_NAME as they may occur in ECF_FETCH_CMD/ECF_SCRIPT_CMD
   update_static_generated_variables(ecf_home,theAbsNodePath);

   const Variable& genvar_ecfscript = get_genvar_ecfscript();

#ifdef DEBUG_TASK_LOCATION
   std::cout << "Submittable::locatedEcfFile() Submittable " << name() << " searching ECF_SCRIPT = '" << genvar_ecfscript.theValue() << "'\n";
#endif
   if ( fs::exists( genvar_ecfscript.theValue() ) ) {
#ifdef DEBUG_TASK_LOCATION
      std::cout << "Submittable::locatedEcfFile() Submittable " << name() << " ECF_SCRIPT = '" << genvar_ecfscript.theValue() << "' exists\n";
#endif
      return EcfFile( const_cast<Submittable*>(this), genvar_ecfscript.theValue() );
   }
   else {
      reasonEcfFileNotFound += "   ECF_SCRIPT(";
      reasonEcfFileNotFound += genvar_ecfscript.theValue();
      reasonEcfFileNotFound += ") does not exist:\n";
    }

   // Caution: This is not used in operations or research, equally is has not been tested.
   std::string ecf_fetch_cmd;
   findParentVariableValue( Str::ECF_FETCH(), ecf_fetch_cmd );
   if ( !ecf_fetch_cmd.empty() ) {
#ifdef DEBUG_TASK_LOCATION
      std::cout << "Submittable::locatedEcfFile() Submittable " << name() << " ECF_FETCH = '" << ecf_fetch_cmd << "' variable exists\n";
#endif
      if (variableSubsitution(ecf_fetch_cmd))  {
         return EcfFile( const_cast<Submittable*>(this), ecf_fetch_cmd, EcfFile::ECF_FETCH_CMD);
      }
      else {
         reasonEcfFileNotFound += "   Variable ECF_FETCH(";
         reasonEcfFileNotFound +=  ecf_fetch_cmd;
         reasonEcfFileNotFound +=  ") defined, but variable substitution has failed:\n";
         throw std::runtime_error( reasonEcfFileNotFound ) ;
      }
   }
   else {
      reasonEcfFileNotFound += "   Variable ECF_FETCH not defined:\n";
   }

   std::string ecf_script_cmd; // ECFLOW-427
   findParentVariableValue( "ECF_SCRIPT_CMD", ecf_script_cmd );
   if ( !ecf_script_cmd.empty() ) {
#ifdef DEBUG_TASK_LOCATION
      std::cout << "Submittable::locatedEcfFile() Submittable " << name() << " ECF_SCRIPT_CMD = '" << ecf_script_cmd << "' variable exists\n";
#endif
      if (variableSubsitution(ecf_script_cmd))  {
         return EcfFile( const_cast<Submittable*>(this), ecf_script_cmd, EcfFile::ECF_SCRIPT_CMD);
      }
      else {
         reasonEcfFileNotFound += "   Variable ECF_SCRIPT_CMD(";
         reasonEcfFileNotFound += ecf_script_cmd;
         reasonEcfFileNotFound += ") defined, but variable substitution has failed:\n";
         throw std::runtime_error( reasonEcfFileNotFound ) ;
      }
   }
   else {
      reasonEcfFileNotFound += "   Variable ECF_SCRIPT_CMD not defined:\n";
   }


   std::string ecf_filesDirectory ;
   if (findParentUserVariableValue( Str::ECF_FILES() , ecf_filesDirectory)) {
#ifdef DEBUG_TASK_LOCATION
      std::cout << "Submittable::locatedEcfFile() Submittable " << name() << " searching ECF_FILES = '" << ecf_filesDirectory << "' backwards\n";
#endif
      if ( !ecf_filesDirectory.empty() && fs::is_directory( ecf_filesDirectory ) )
      {
         // If File::backwardSearch fails it returns an empty string, i.e failure to locate script (Task/.ecf || Alias/.usr) file
         std::string searchResult = File::backwardSearch( ecf_filesDirectory, theAbsNodePath, script_extension() );
         if ( searchResult.empty()) {
            reasonEcfFileNotFound += "   Search of directory ECF_FILES(";
            reasonEcfFileNotFound += ecf_filesDirectory;
            reasonEcfFileNotFound += ") failed:\n";
         }
         else  return EcfFile(const_cast<Submittable*>(this), searchResult);
      }
      else {
         // Before failing try again but with variable Subsitution. ECFLOW-788
         std::string original_ecf_filesDirectory = ecf_filesDirectory;
         variableSubsitution(ecf_filesDirectory);
         if ( !ecf_filesDirectory.empty() && fs::is_directory(ecf_filesDirectory))
         {
            // If File::backwardSearch fails it returns an empty string, i.e failure to locate script (Task/.ecf || Alias/.usr) file
            std::string searchResult = File::backwardSearch( ecf_filesDirectory, theAbsNodePath, script_extension() );
            if ( searchResult.empty()) {
               std::stringstream ss; ss << "   Search of directory ECF_FILES(variable substituted)(" << ecf_filesDirectory << ") failed:\n";
               reasonEcfFileNotFound += ss.str();
            }
            else return EcfFile(const_cast<Submittable*>(this), searchResult);
         }
         else {
            std::stringstream ss;
            ss << "   Directory ECF_FILES(" << original_ecf_filesDirectory << ") does not exist:\n";
            if (original_ecf_filesDirectory != ecf_filesDirectory )
               ss << "   Directory ECF_FILES(" << ecf_filesDirectory << ") after variable substitution does not exist:\n";
            reasonEcfFileNotFound += ss.str();
         }
      }
   }
   else {
      reasonEcfFileNotFound += "   Variable ECF_FILES not defined:\n";
   }


#ifdef DEBUG_TASK_LOCATION
   std::cout << "Submittable::locatedEcfFile() Submittable " << name() << " searching ECF_HOME = '" << ecf_home << "' backwards\n";
#endif
   if ( !ecf_home.empty() && fs::is_directory( ecf_home ) )
   {
      // If File::backwardSearch fails it returns an empty string, i.e failure to locate script (Task/.ecf || Alias/.usr) file
      std::string searchResult = File::backwardSearch( ecf_home, theAbsNodePath, script_extension() );
      if ( searchResult.empty()) {
         reasonEcfFileNotFound += "   Search of directory ECF_HOME(";
         reasonEcfFileNotFound += ecf_home;
         reasonEcfFileNotFound += ") failed:\n";
      }
      else {
         return EcfFile(const_cast<Submittable*>(this), searchResult);
      }
   }
   else {
      reasonEcfFileNotFound += "   Directory ECF_HOME(";
      reasonEcfFileNotFound += ecf_home;
      reasonEcfFileNotFound += ") does not exist:\n";
   }

   // failed to find  .ecf file
   std::string error_msg = "   Script for ";
   error_msg += theAbsNodePath;
   error_msg += " can not be found:\n";
   error_msg += reasonEcfFileNotFound;
   throw std::runtime_error( error_msg ) ;
}

void Submittable::set_jobs_password(const std::string& p)
{
   jobsPassword_ = p;
   state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Submittable::set_jobs_password\n";
#endif
}

void Submittable::increment_try_no()
{
   // EVERY time we SUBMIT a job we must:
   // o generate a password, and hence updated generated variable ECF_PASS
   // o increment the try number
   //   This means we also need to regenerate the variables
   //   since ECF_TRYNO , ECF_JOB, ECF_JOBOUT depend on the try number.
   //   ALSO ECF_RID is updated with empty string
   // o Clear the process/remote id. These will be reset by the child commands
   // *** This MUST be done before pre-processing as it uses these variables ***
   tryNo_++;
   process_or_remote_id_.clear();
   abortedReason_.clear();
   jobsPassword_ = Passwd::generate();
   state_change_no_ = Ecf::incr_state_change_no();
   update_generated_variables();
}

void Submittable::clear()
{
   abortedReason_.clear();       // reset reason aborted
   jobsPassword_.clear();        // reset password, it will be regenerated before submission
   process_or_remote_id_.clear();// reset process id
   state_change_no_ = Ecf::incr_state_change_no();
}

bool Submittable::submitJob( JobsParam& jobsParam)
{
   increment_try_no();

   return submit_job_only(jobsParam);
}

bool Submittable::submit_job_only( JobsParam& jobsParam)
{
#ifdef DEBUG_JOB_SUBMISSION
   cerr << "Submittable::submit_job_only for task " << name() << endl;
#endif

   if (state() == NState::ACTIVE  || state() == NState::SUBMITTED ) {
      std::stringstream ss;
      ss << "Submittable::submit_job_only: failed: Submittable " << absNodePath() << " is already " << NState::toString(state()) << " : ";
      jobsParam.errorMsg() += ss.str();
      flag().set(ecf::Flag::EDIT_FAILED);
      return false;
   }

   // If the task is a dummy task, return true
   std::string theValue;
   if (findParentUserVariableValue(Str::ECF_DUMMY_TASK(),theValue)) {
      return true;
   }

   // state change
   flag().clear(ecf::Flag::NO_SCRIPT);
   flag().clear(ecf::Flag::EDIT_FAILED);
   flag().clear(ecf::Flag::JOBCMD_FAILED);
   requeue_labels(); // ECFLOW-195, requeue no longer resets labels on tasks, hence we do it at task run time.

   theValue.clear();
   if (findParentUserVariableValue(Str::ECF_NO_SCRIPT(),theValue)) {
      // The script is based on the ECF_JOB_CMD
      return non_script_based_job_submission(jobsParam);
   }

   return script_based_job_submission(jobsParam);
}

bool Submittable::script_based_job_submission(JobsParam& jobsParam)
{
   try {
      // Locate the ecf files corresponding to the task.
      // Assign lifetime of EcfFile to JobsParam.
      // Minimise memory allocation/deallocation with Job lines and allow include file caching
      jobsParam.set_ecf_file( locatedEcfFile() );

      // Pre-process ecf file (i.e expand includes, remove comments,manual) and perform
      // variable substitution. This will then form the '.job' files.
      // If the job file already exist it is overridden
      // The job file SHOULD be referenced in ECF_JOB_CMD
      try {
         const std::string& job_size = jobsParam.ecf_file().create_job( jobsParam );

         //... make sure ECF_PASS is set on the task, This is substituted in <head.h> file
         //... and hence must be done before variable substitution in ECF_/JOB file
         //... This is used by client->server authentication
         if (createChildProcess(jobsParam)) {
            set_state(NState::SUBMITTED, false, job_size );
            return true;
         }
         // Fall through job submission failed.
      }
      catch ( std::exception& e) {
         flag().set(ecf::Flag::EDIT_FAILED);
         std::string reason = "Submittable::submit_job_only: Job creation failed for task ";
         reason += absNodePath();
         reason += " : \n   ";
         reason += e.what();
         reason += "\n";
         jobsParam.errorMsg() += reason;
         set_aborted_only( reason );
         return false;
      }
   }
   catch (std::exception& e) {
      flag().set(ecf::Flag::NO_SCRIPT);
      std::stringstream ss; ss << "Submittable::submit_job_only: Script location failed for task " << absNodePath() << " :\n" << e.what() << "\n";
      jobsParam.errorMsg() += ss.str();
      set_aborted_only( e.what() ); // remember jobsParam.errorMsg() is accumulated
      return false;
   }

   flag().set(ecf::Flag::JOBCMD_FAILED);
   std::string reason = " Job creation failed for task ";
   reason += absNodePath();
   reason += " could not create child process.";
   jobsParam.errorMsg() += reason;
   set_aborted_only( reason );
   return false;
}

bool Submittable::non_script_based_job_submission(JobsParam& jobsParam)
{
   // No script(i.e .ecf file), hence it is assumed the ECF_JOB_CMD will call:
   //  ECF_PASS=%ECF_PASS%;ECF_PORT=%ECF_PORT%;ECF_HOST=%ECF_HOST%;ECF_NAME=%ECF_NAME%;ECF_TRYNO=%ECF_TRYNO%;
   //  ecflow_client --init %%;
   //     . some user script, or in-line command, should use full path.;
   //     ecflow_client (--meter,--event,--label);
   //  ecflow_client --hcomplete

   if (createChildProcess(jobsParam)) {
      set_state(NState::SUBMITTED, false, Str::EMPTY()/*job size*/ );
      return true;
   }

   // Fall through job submission failed.
   flag().set(ecf::Flag::JOBCMD_FAILED);
   std::string reason = " Job creation failed for task ";
   reason += absNodePath();
   reason += " could not create child process.";
   jobsParam.errorMsg() += reason;
   set_aborted_only( reason );
   return false;
}

class JobCreationTimer : private boost::noncopyable {
public:
   JobCreationTimer(Submittable* sub) : enabled_(false),failed_(false), sub_(sub) {}
   ~JobCreationTimer() {
      if (enabled_) {
         std::cout << " " << sub_->absNodePath();
         if (failed_)  std::cout << " (FAILED)\n";
         else {
            boost::posix_time::time_duration duration = Calendar::second_clock_time() - start_;
            std::cout << " (" << duration.total_microseconds() << " ms)\n";
         }
      }
   }
   void set_enabled() { enabled_ = true; start_ = Calendar::second_clock_time(); }
   void set_failed() { failed_ = true; }
private:
   bool enabled_;
   bool failed_;
   Submittable* sub_;
   boost::posix_time::ptime start_;
};

void Submittable::check_job_creation( job_creation_ctrl_ptr jobCtrl)
{
   JobCreationTimer output_tasks_being_checked(this);
   if ( jobCtrl->verbose() ) output_tasks_being_checked.set_enabled();

   // Typically a valid try number is >=1.
   // Since check_job_creation is only used for testing/python, we will initialise tryNum to -1
   // so than when it is incremented it will by a try_no of zero. *zero is an invalid try_no *
   tryNo_ = -1;

   /// call just before job submission, reset data members, update try_no, and *** generate variable ***
   increment_try_no();

   if ( !jobCtrl->dir_for_job_creation().empty() ) {
      /// Override ECF_JOB, can be done by either adding a new user variable of same name
      /// or the generated variable. We choose generated, since this is automatically reset.
      /// before a real job submission
      std::string tmpLocationForJob = jobCtrl->dir_for_job_creation();
      tmpLocationForJob += absNodePath();
      tmpLocationForJob += File::JOB_EXTN();
      tmpLocationForJob += "0"; // try number of zero ( i.e an invalid try number)
      set_genvar_ecfjob(tmpLocationForJob);
   }

   // Before we had a Local JobsParam  here
   // By using the JobsParam on job_creation_ctrl_ptr, we get the effect of USING a cache.
   // EcfFile stored on JobsParam has cache. This cache avoids opening include files more than once.
   // Since we have a Single job_creation_ctrl_ptr, hence single JobsParam, hence single EcfFile
   // the cache is applied for ALL tasks. See ECFLOW-1210
   jobCtrl->jobsParam().clear(); // allow jobsParam to be re-used by clearing
   LOG_ASSERT(!jobCtrl->jobsParam().spawnJobs() , "spawn jobs should be disabled for check job creation" );
   LOG_ASSERT(!jobCtrl->jobsParam().createJobs(), "create jobs should be disabled for check job creation" );
   if ( submit_job_only( jobCtrl->jobsParam() ) ) {
      return;
   }
   output_tasks_being_checked.set_failed();

   std::string errorMsg = jobCtrl->jobsParam().getErrorMsg();
   LOG_ASSERT( !errorMsg.empty(), "failing to submit must raise an error message" );

   jobCtrl->error_msg() += errorMsg;
   jobCtrl->push_back_failing_submittable( dynamic_pointer_cast<Submittable>(shared_from_this()) );
}

bool Submittable::run(JobsParam& jobsParam, bool force)
{
   if (force || ((state() != NState::SUBMITTED) && (state() != NState::ACTIVE))) {

      if ( jobsParam.createJobs() ) {

         return submitJob(jobsParam);
      }
      else {
         /// This is only for test path. Just return true
         return true;
      }
   }
   std::stringstream ss;
   ss << "Submittable::run: Aborted for task " << absNodePath() << " because state is " << NState::toString(state()) << " and force not set\n";
   jobsParam.errorMsg() += ss.str();
   return false;
}

void Submittable::kill(const std::string& zombie_pid)
{
   std::string ecf_kill_cmd;
   if ( zombie_pid.empty() ) {
      if (state() != NState::ACTIVE && state() != NState::SUBMITTED) {
         return;
      }

      // *** Generated variables are *NOT* persisted.                                     ***
      // *** Hence if we have recovered from a check point file, then they will be empty. ***
      // *** i.e terminate server with active jobs, restart from saved check_pt file
      // *** and then try to kill the active job, will get an exception( see below) since
      // *** Generated variable ECF_RID will be empty.
      if (!sub_gen_variables_) {
         // std::cout << "Generated variables empty, regenerating !!!!!\n";
         update_generated_variables();
      }

      /// If we are in active state, then ECF_RID must have been setup
      /// This is typically used in the KILL CMD, make sure its there
      if (state() == NState::ACTIVE && get_genvar_ecfrid().theValue().empty() ) {
         std::stringstream ss;
         ss << "Submittable::kill: Generated variable ECF_RID is empty for task " << absNodePath();
         throw std::runtime_error( ss.str() );
      }

      if (!findParentUserVariableValue( Str::ECF_KILL_CMD(), ecf_kill_cmd ) ||  ecf_kill_cmd.empty() ) {
         std::stringstream ss;
         ss << "Submittable::kill: ECF_KILL_CMD not defined, for task " << absNodePath() << "\n";
         throw std::runtime_error( ss.str() );
      }
   }
   else {
      // Use input
      if (!findParentUserVariableValue( Str::ECF_KILL_CMD(), ecf_kill_cmd ) ||  ecf_kill_cmd.empty() ) {
         std::stringstream ss;
         ss << "Submittable::kill: ECF_KILL_CMD not defined, for task " << absNodePath() << "\n";
         throw std::runtime_error( ss.str() );
      }

      // replace %ECF_RID% with the input args
      Str::replace(ecf_kill_cmd,"%ECF_RID%", zombie_pid);
   }

   if (!variableSubsitution(ecf_kill_cmd)) {
      std::stringstream ss;
      ss << "Submittable::kill: Variable substitution failed for ECF_KILL_CMD(" << ecf_kill_cmd << ") on task " << absNodePath() << "\n";
      throw std::runtime_error( ss.str() );
   }

   // Please note: this is *non blocking* the output of the command(ECF_KILL_CMD) should be written to %ECF_JOB%.kill
   // The output is accessible via the --file cmd
   // Done as two separate steps as kill command is not blocking on the server
//   LOG(Log::DBG,"Submittable::kill " << absNodePath() << "  " << ecf_kill_cmd );
   std::string errorMsg;
   if (!System::instance()->spawn(ecf_kill_cmd,"", errorMsg)) {
      throw std::runtime_error( errorMsg );
   }
   flag().set(ecf::Flag::KILLED);
}

void Submittable::status()
{
   if (state() != NState::ACTIVE && state() != NState::SUBMITTED) {
      return;
   }

   // *** Generated variables are *NOT* persisted.                                     ***
   // *** Hence if we have recovered from a check point file, then they will be empty. ***
   // *** i.e terminate server with active jobs, restart from saved check_pt file
   // *** and then try to kill/status the active job, will get an exception(see below) since
   // *** Generated variable ECF_RID will be empty.
   if (!sub_gen_variables_) {
      //std::cout << "Generated variables empty, regenerating !!!!!\n";
      update_generated_variables();
   }

   /// If we are in active state, then ECF_RID must have been setup
   if (state() == NState::ACTIVE && get_genvar_ecfrid().theValue().empty()) {
      std::stringstream ss;
      ss << "Submittable::status: Generated variable ECF_RID is empty for task " << absNodePath();
      throw std::runtime_error( ss.str() );
   }

   std::string ecf_status_cmd;
   if (!findParentUserVariableValue( Str::ECF_STATUS_CMD(), ecf_status_cmd ) ||  ecf_status_cmd.empty() ) {
      std::stringstream ss;
      ss << "Submittable::status: ECF_STATUS_CMD not defined, for task " << absNodePath() << "\n";
      throw std::runtime_error( ss.str() );
   }

   if (!variableSubsitution(ecf_status_cmd)) {
      std::stringstream ss;
      ss << "Submittable::status: Variable substitution failed for ECF_STATUS_CMD(" << ecf_status_cmd << ") on task " << absNodePath() << "\n";
      throw std::runtime_error( ss.str() );
   }

   // Please note: this is *non blocking* the output of the command(ECF_STATUS_CMD) should be written to %ECF_JOB%.stat
   // SPAWN process, attach signal to monitor process. returns true
   std::string errorMsg;
   if (!System::instance()->spawn(ecf_status_cmd,"", errorMsg)) {
      throw std::runtime_error( errorMsg );
   }

// flag().set(ecf::Flag::STATUS);
}


bool Submittable::createChildProcess(JobsParam& jobsParam)
{
#ifdef DEBUG_JOB_SUBMISSION
   cout << "Submittable::createChildProcess for task " << name() << endl;
#endif
   std::string ecf_job_cmd;
   findParentUserVariableValue( Str::ECF_JOB_CMD(), ecf_job_cmd );
   if (ecf_job_cmd.empty()) {
      jobsParam.errorMsg() += "Submittable::createChildProcess: Could not find ECF_JOB_CMD : ";
      return false;
   }

   if (!variableSubsitution(ecf_job_cmd)) {
      jobsParam.errorMsg() += "Submittable::createChildProcess: Variable substitution failed for ECF_JOB_CMD(" + ecf_job_cmd + ") :";
      return false;
   }

   // Keep tabs on what was submitted for testing purposes
   jobsParam.push_back_submittable(  this );

   if ( jobsParam.spawnJobs() ) {

      // SPAWN process, attach signal to monitor process. returns true
      return System::instance()->spawn(ecf_job_cmd,absNodePath(),jobsParam.errorMsg());
   }

   // Test path ONLY
   return true;
}


void Submittable::set_aborted_only(const std::string& reason)
{
#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Submittable::set_aborted_only\n";
#endif

   abortedReason_ = reason;
   state_change_no_ = Ecf::incr_state_change_no();

   // Do not use "\n" | ';' in abortedReason_, as this can mess up, --migrate output
   // Which would then affect --load.
   Str::replace(abortedReason_,"\n","");
   Str::replace(abortedReason_,";"," ");

   // This will set the state and bubble up the most significant state
   set_state(NState::ABORTED);
}


void Submittable::update_limits()
{
   NState::State task_state = state();
   std::set<Limit*> limitSet;     // ensure local limit have preference over parent
   if (task_state == NState::COMPLETE) {
      decrementInLimit(limitSet);    // will recurse up
   }
   else if (task_state == NState::ABORTED) {
       decrementInLimit(limitSet);    // will recurse up
   }
   else if (task_state == NState::SUBMITTED) {
       incrementInLimit(limitSet);    // will recurse up
   }
   else if (task_state == NState::ACTIVE) {
      // *** Don't change limits in this state ***
   }
   else {
      // UNKNOWN, QUEUED
      // For all other states, this task should NOT be consuming a limit token.
      // During interactive use a Submittable may get re-queued. In case its consuming
      // a limit token, we decrement the limit. If the we are NOT consuming
      // a token, its still *SAFE* to call decrementInLimit
      decrementInLimit(limitSet);  // will recurse up
   }
}

// Memento ==========================================================
void Submittable::incremental_changes(DefsDelta& changes, compound_memento_ptr& comp) const
{
#ifdef DEBUG_MEMENTO
   std::cout << "Submittable::incremental_changes() " << debugNodePath() << "\n";
#endif

   if (state_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = boost::make_shared<CompoundMemento>(absNodePath());
      comp->add( boost::make_shared<SubmittableMemento>( jobsPassword_,process_or_remote_id_,abortedReason_,tryNo_) );
   }

   // ** if compound memento has children base class, will add it to DefsDelta
   Node::incremental_changes(changes,comp);
}

void Submittable::set_memento(const SubmittableMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only)
{
#ifdef DEBUG_MEMENTO
   std::cout << "Submittable::set_memento(const SubmittableMemento*) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      aspects.push_back(ecf::Aspect::SUBMITTABLE);
      return;
   }

   jobsPassword_ = memento->jobsPassword_;
   process_or_remote_id_ = memento->process_or_remote_id_;
   abortedReason_ = memento->abortedReason_;
   tryNo_ = memento->tryNo_;
}

// Generated variables ---------------------------------------------------------------------------------

void Submittable::update_generated_variables() const
{
   if (!sub_gen_variables_) sub_gen_variables_ = new SubGenVariables(this);
   sub_gen_variables_->update_generated_variables();
   update_repeat_genvar();
}

void Submittable::update_static_generated_variables(const std::string& ecf_home, const std::string& theAbsNodePath) const
{
   if (!sub_gen_variables_) sub_gen_variables_ = new SubGenVariables(this);
   sub_gen_variables_->update_static_generated_variables(ecf_home,theAbsNodePath);
}

const Variable& Submittable::findGenVariable(const std::string& name) const
{
   // AST can reference generated variables. Currently integer based values
   // The task names can be integers, other valid option is try_no
   if (!sub_gen_variables_) update_generated_variables();

   const Variable& gen_var = sub_gen_variables_->findGenVariable(name);
   if (!gen_var.empty()) return gen_var;

   return Node::findGenVariable(name);
}

void Submittable::gen_variables(std::vector<Variable>& vec) const
{
   if (!sub_gen_variables_) update_generated_variables();

   vec.reserve(vec.size() + 9);
   sub_gen_variables_->gen_variables(vec);
   Node::gen_variables(vec);
}

const Variable& Submittable::get_genvar_ecfrid() const
{
   if (!sub_gen_variables_) return Variable::EMPTY();
   return sub_gen_variables_->genvar_ecfrid();
}

const Variable& Submittable::get_genvar_ecfscript() const
{
   if (!sub_gen_variables_) return Variable::EMPTY();
   return sub_gen_variables_->genvar_ecfscript();
}

void Submittable::set_genvar_ecfjob(const std::string& value)
{
   if (!sub_gen_variables_) sub_gen_variables_ = new SubGenVariables(this);
   sub_gen_variables_->set_genvar_ecfjob(value);
}

void Submittable::set_genvar_ecfrid(const std::string& value)
{
   if (!sub_gen_variables_) sub_gen_variables_ = new SubGenVariables(this);
   sub_gen_variables_->set_genvar_ecfrid(value);
}

// Generated Variables ================================================================================================
// The false below is used as a dummy argument to call the Variable constructor that does not
// Check the variable names. i.e we know they are valid
SubGenVariables::SubGenVariables(const Submittable* sub)
: submittable_(sub),
  genvar_ecfjob_(Str::ECF_JOB(), "", false),
  genvar_ecfjobout_(Str::ECF_JOBOUT(), "", false),
  genvar_ecftryno_(Str::ECF_TRYNO(), "", false),
  genvar_task_("TASK", "", false),
  genvar_ecfpass_(Str::ECF_PASS(), "", false),
  genvar_ecfscript_(Str::ECF_SCRIPT(), "", false),
  genvar_ecfname_(Str::ECF_NAME(), "", false),
  genvar_ecfrid_(Str::ECF_RID(), "", false)
  {}

void SubGenVariables::update_generated_variables() const
{
   // cache strings that are used in many variables
   std::string theAbsNodePath = submittable_->absNodePath();
   std::string ecf_home;
   submittable_->findParentUserVariableValue(Str::ECF_HOME(), ecf_home);
   update_static_generated_variables(ecf_home,theAbsNodePath);
   update_dynamic_generated_variables(ecf_home,theAbsNodePath);
}

void SubGenVariables::update_static_generated_variables(const std::string& ecf_home,const std::string& theAbsNodePath) const
{
   // cache strings that are used in many variables
   if (submittable_->isAlias() && submittable_->parent())
      genvar_task_.set_value(submittable_->parent()->name());     // does *not* modify Variable::state_change_no
   else
      genvar_task_.set_value(submittable_->name());

   genvar_ecfname_.set_value(theAbsNodePath);                     // does *not* modify Variable::state_change_no

   genvar_ecfscript_.value_by_ref().reserve( ecf_home.size() + theAbsNodePath.size() + 4 );
   genvar_ecfscript_.value_by_ref() = ecf_home;   // does *not* modify Variable::state_change_no
   genvar_ecfscript_.value_by_ref() += theAbsNodePath;
   genvar_ecfscript_.value_by_ref() += submittable_->script_extension();
}

void SubGenVariables::update_dynamic_generated_variables(const std::string& ecf_home,const std::string& theAbsNodePath) const
{
   // cache strings that are used in many variables
   std::string the_try_no = submittable_->tryNo();

   genvar_ecfrid_.set_value(submittable_->process_or_remote_id_); // does *not* modify Variable::state_change_no
   genvar_ecftryno_.set_value(the_try_no);                        // does *not* modify Variable::state_change_no
   genvar_ecfpass_.set_value(submittable_->jobsPassword_);        // does *not* modify Variable::state_change_no


   /// The directory associated with ECF_JOB is automatically created if it does not exist.
   /// This is Done during Job generation. See EcfFile::doCreateJobFile()
   if (genvar_ecfjob_.value_by_ref().capacity() == 0) {
      genvar_ecfjob_.value_by_ref().reserve( ecf_home.size() + theAbsNodePath.size() + File::JOB_EXTN().size() + the_try_no.size());
   }
   genvar_ecfjob_.value_by_ref() = ecf_home;          // does *not* modify Variable::state_change_no
   genvar_ecfjob_.value_by_ref() += theAbsNodePath;
   genvar_ecfjob_.value_by_ref() += File::JOB_EXTN();
   genvar_ecfjob_.value_by_ref() += the_try_no;


   /// If ECF_OUT is specified the user must ensure the directory exists, along with directories
   /// associated with Suites/Families nodes.
   /// Bottom up. Can be expensive when we have thousands of tasks.
   std::string ecf_out;
   submittable_->findParentUserVariableValue(Str::ECF_OUT(), ecf_out);

   if (ecf_out.empty()) {
      genvar_ecfjobout_.value_by_ref().reserve( ecf_home.size() + theAbsNodePath.size() + 1 + the_try_no.size());
      genvar_ecfjobout_.value_by_ref() = ecf_home;
   }
   else  {
      // For metabuilder, where we use %ECF_HOME% for ECF_OUT
      char micro = '%';
      if (ecf_out.find(micro) != std::string::npos) {
         NameValueMap user_edit_variables;
         submittable_->variable_substitution(ecf_out,user_edit_variables,micro);
      }
      genvar_ecfjobout_.value_by_ref().reserve( ecf_out.size() + theAbsNodePath.size() + 1 + the_try_no.size());
      genvar_ecfjobout_.value_by_ref() = ecf_out;
   }
   genvar_ecfjobout_.value_by_ref() += theAbsNodePath;
   genvar_ecfjobout_.value_by_ref() += ".";
   genvar_ecfjobout_.value_by_ref() += the_try_no;
}

const Variable& SubGenVariables::findGenVariable(const std::string& name) const
{
   if (genvar_ecfjob_.name() == name)     return genvar_ecfjob_;
   if (genvar_ecfjobout_.name() == name)  return genvar_ecfjobout_;
   if (genvar_ecftryno_.name() == name)   return genvar_ecftryno_;
   if (genvar_ecfname_.name() == name)    return genvar_ecfname_;
   if (genvar_task_.name() == name)       return genvar_task_;
   if (genvar_ecfpass_.name() == name)    return genvar_ecfpass_;
   if (genvar_ecfscript_.name() == name)  return genvar_ecfscript_;
   if (genvar_ecfrid_.name() == name)     return genvar_ecfrid_;
   return Variable::EMPTY();
}

void SubGenVariables::gen_variables(std::vector<Variable>& vec) const
{
   vec.push_back(genvar_task_);
   vec.push_back(genvar_ecfjob_);
   vec.push_back(genvar_ecfscript_);
   vec.push_back(genvar_ecfjobout_);
   vec.push_back(genvar_ecftryno_);
   vec.push_back(genvar_ecfrid_);
   vec.push_back(genvar_ecfname_);
   vec.push_back(genvar_ecfpass_);
}
