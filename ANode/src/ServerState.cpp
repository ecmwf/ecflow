//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #29 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <assert.h>
#include <iostream>
#include "ServerState.hpp"
#include "Str.hpp"
#include "Log.hpp"
#include "Host.hpp"
#include "Ecf.hpp"
#include "Version.hpp"

using namespace ecf;
using namespace std;

// When a Defs is loaded into a server:
//       	o the jobSubmissionInterval_ is set
//       	o the jobGeneration_ is set
ServerState::ServerState() :
	state_change_no_(0),
   variable_state_change_no_(0),
	server_state_( default_state() ),
 	jobSubmissionInterval_( 60 ),
 	jobGeneration_( true )
{
	setup_default_env();
}

ServerState::ServerState(const ServerState& rhs)
{
   state_change_no_ = 0;                  // *not* persisted, only used on server side
   variable_state_change_no_ = 0;         // *not* persisted, only used on server side
   jobSubmissionInterval_ = 60;           // NOT persisted, since set in the server
   jobGeneration_ = true;                 // NOT persisted, since set in the server
   // hostPort_ -> empty on construction; // NOT persisted, set by server hence no need to persist
   server_state_  = rhs.server_state_;
   server_variables_ = rhs.server_variables_;
}

bool ServerState::operator==(const ServerState& rhs) const
{
   if ( get_state() != rhs.get_state()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ServerState::operator== get_state(" << SState::to_string(get_state()) << ") != rhs.get_state(" << SState::to_string(rhs.get_state()) << ")\n";
      }
#endif
      return false;
   }

   if ( user_variables_ != rhs.user_variables_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ServerState::compare user_variables_ != rhs.user_variables_\n";
         std::cout << "user_variables_:\n";
         for(std::vector<Variable>::const_iterator i = user_variables_.begin(); i!=user_variables_.end(); ++i) {
            std::cout << "   " << (*i).name() << " " << (*i).theValue() << "\n";
         }
         std::cout << "rhs.user_variables_:\n";
         for(std::vector<Variable>::const_iterator i = rhs.user_variables_.begin(); i!=rhs.user_variables_.end(); ++i) {
            std::cout << "   " << (*i).name() << " " << (*i).theValue() << "\n";
         }
      }
#endif
      return false;
   }

   /// Check pointing, SAVES server variables, since they are visualised by client like ecflowview
   /// HOWEVER PrintStyle::MIGRATE does not save the server variables, since they should
   /// not take part in migration. However the testing compares migration files with check point files
   /// This would always fail. Hence we do not compare server variables.

   return true;
}

bool ServerState::compare(const ServerState& rhs) const
{
   if ( get_state() != rhs.get_state()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ServerState::compare get_state(" << SState::to_string(get_state()) << ") != rhs.get_state(" << SState::to_string(rhs.get_state()) << ")\n";
      }
#endif
      return false;
   }

   if ( user_variables_ != rhs.user_variables_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ServerState::compare user_variables_ != rhs.user_variables_\n";
         std::cout << "user_variables_:\n";
         for(std::vector<Variable>::const_iterator i = user_variables_.begin(); i!=user_variables_.end(); ++i) {
            std::cout << "   " << (*i).name() << " " << (*i).theValue() << "\n";
         }
         std::cout << "rhs.user_variables_:\n";
         for(std::vector<Variable>::const_iterator i = rhs.user_variables_.begin(); i!=rhs.user_variables_.end(); ++i) {
            std::cout << "   " << (*i).name() << " " << (*i).theValue() << "\n";
         }
      }
#endif
      return false;
   }

   if ( server_variables_ != rhs.server_variables_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ServerState::compare server_variables_ != rhs.server_variables_\n";
         std::cout << "server_variables_:\n";
         for(std::vector<Variable>::const_iterator i = server_variables_.begin(); i!=server_variables_.end(); ++i) {
            std::cout << "   " << (*i).name() << " " << (*i).theValue() << "\n";
         }
         std::cout << "rhs.server_variables_:\n";
         for(std::vector<Variable>::const_iterator i = rhs.server_variables_.begin(); i!=rhs.server_variables_.end(); ++i) {
            std::cout << "   " << (*i).name() << " " << (*i).theValue() << "\n";
         }
      }
#endif
      return false;
   }
   return true;
}

// server variable can NOT be modified or deleted, only overridden
void ServerState::add_or_update_server_variables( const NameValueVec& env)
{
   // n2 could use map to speed things up.
   NameValueVec::const_iterator i;
   NameValueVec::const_iterator theEnd = env.end();
   for(i = env.begin(); i!=theEnd; ++i) {
      add_or_update_server_variables((*i).first, (*i).second);
   }
}
void ServerState::add_or_update_server_variables( const std::string& name, const std::string& value)
{
   std::vector<Variable>::iterator var_end = server_variables_.end();
   for(std::vector<Variable>::iterator i = server_variables_.begin(); i!=var_end; ++i) {
      if ((*i).name() == name) {
         (*i).set_value( value );
//         std::cout << "   Server Variables: Updating " << name << "   " << value << "\n";
         return;
      }
   }
//   std::cout << "   Server Variables: Adding " << name << "   " << value << "\n";
   server_variables_.push_back( Variable(name, value) );
}

void ServerState::set_server_variables(const std::vector<Variable>& e)
{
   server_variables_ = e;
}

void ServerState::delete_server_variable( const std::string& var)
{
   std::vector<Variable>::iterator var_end = server_variables_.end();
   for(std::vector<Variable>::iterator i = server_variables_.begin(); i!=var_end; ++i) {
      if ((*i).name() == var) {
         server_variables_.erase(i);
         break;
      }
   }
}

// ================================================================================

void ServerState::add_or_update_user_variables( const NameValueVec& env)
{
   // n2 could use map to speed things up.
   NameValueVec::const_iterator i;
   NameValueVec::const_iterator theEnd = env.end();
   for(i = env.begin(); i!=theEnd; ++i) {
      add_or_update_user_variables((*i).first, (*i).second);
   }
}

void ServerState::add_or_update_user_variables( const std::vector<Variable>& env)
{
   std::vector<Variable>::const_iterator var_end = env.end();
   for(std::vector<Variable>::const_iterator i = env.begin(); i!=var_end; ++i) {
      add_or_update_user_variables( (*i).name(), (*i).theValue());
   }
}

void ServerState::add_or_update_user_variables( const std::string& name, const std::string& value)
{
   std::vector<Variable>::iterator var_end = user_variables_.end();
   for(std::vector<Variable>::iterator i = user_variables_.begin(); i!=var_end; ++i) {
      if ((*i).name() == name) {
         (*i).set_value( value );
         variable_state_change_no_ = Ecf::incr_state_change_no();
//         std::cout << "   ServerState::add_or_update_user_variables: Updating " << name << "   " << value << "\n";
         return;
      }
   }

//	std::cout << "   ServerState::add_or_update_user_variables: Adding " << name << "   " << value << "\n";
   user_variables_.push_back( Variable(name, value) );
	variable_state_change_no_ = Ecf::incr_state_change_no();
}

void ServerState::delete_user_variable( const std::string& var)
{
   if (var.empty()) {
      // delete all user variables
      user_variables_.clear();
      variable_state_change_no_ = Ecf::incr_state_change_no();
      return;
   }

   std::vector<Variable>::iterator var_end = user_variables_.end();
   for(std::vector<Variable>::iterator i = user_variables_.begin(); i!=var_end; ++i) {
      if ((*i).name() == var) {
         user_variables_.erase(i);
         variable_state_change_no_ = Ecf::incr_state_change_no();
         break;
      }
   }
}

const std::string& ServerState::find_user_variable(const std::string& theVarName) const
{
   std::vector<Variable>::const_iterator user_var_end = user_variables_.end();
   for(std::vector<Variable>::const_iterator i = user_variables_.begin(); i!=user_var_end; ++i) {
      if ((*i).name() == theVarName) {
         LOG_ASSERT(!(*i).theValue().empty(),"");
         return (*i).theValue();
      }
   }
   return Str::EMPTY();
}

const std::string& ServerState::find_variable(const std::string& theVarName) const
{
   // SEARCH USER variables FIRST
   std::vector<Variable>::const_iterator user_var_end = user_variables_.end();
   for(std::vector<Variable>::const_iterator i = user_variables_.begin(); i!=user_var_end; ++i) {
      if ((*i).name() == theVarName) {
//			cerr << "FOUND '" << (*i).first << "'   '" << (*i).second << "'\n";
			LOG_ASSERT(!(*i).theValue().empty(),"");
 			return (*i).theValue();
		}
	}

   // NOW search server variables
   std::vector<Variable>::const_iterator ser_var_end = server_variables_.end();
   for(std::vector<Variable>::const_iterator i = server_variables_.begin(); i!=ser_var_end; ++i) {
      if ((*i).name() == theVarName) {
//       cerr << "FOUND '" << (*i).first << "'   '" << (*i).second << "'\n";
         LOG_ASSERT(!(*i).theValue().empty(),"");
         return (*i).theValue();
      }
   }

//	cerr << "FAILED to FIND '" << theVarName << "'\n";
	return Str::EMPTY();
}

const Variable& ServerState::findVariable(const std::string& name) const
{
   // SEARCH USER variables FIRST
   std::vector<Variable>::const_iterator var_end = user_variables_.end();
   for(std::vector<Variable>::const_iterator i = user_variables_.begin(); i!=var_end; ++i) {
      if ((*i).name() == name) {
         LOG_ASSERT(!(*i).theValue().empty(),"");
         // if ((*i).theValue().empty() )  std::cout << (*i).name() << " has a empty value\n";
         return (*i);
      }
   }

   // NOW search server variables
   std::vector<Variable>::const_iterator ser_var_end = server_variables_.end();
   for(std::vector<Variable>::const_iterator i = server_variables_.begin(); i!=ser_var_end; ++i) {
      if ((*i).name() == name) {
         LOG_ASSERT(!(*i).theValue().empty(),"");
         // if ((*i).theValue().empty() )  std::cout << (*i).name() << " has a empty value\n";
         return (*i);
      }
   }

// cerr << "FAILED to FIND '" << theVarName << "'\n";
   return Variable::EMPTY();
}

bool ServerState::variable_exists(const std::string& name) const
{
   // SEARCH USER variables FIRST
   std::vector<Variable>::const_iterator var_end = user_variables_.end();
   for(std::vector<Variable>::const_iterator i = user_variables_.begin(); i!=var_end; ++i) {
      if ((*i).name() == name) return true;
   }

   // NOW search server variables
   std::vector<Variable>::const_iterator ser_var_end = server_variables_.end();
   for(std::vector<Variable>::const_iterator i = server_variables_.begin(); i!=ser_var_end; ++i) {
      if ((*i).name() == name) return true;
   }

   return false;
}

bool ServerState::variableSubsitution(std::string& cmd) const
{
   // scan the command for variables, and substitute
   // We can also have
   //
   // "%<VAR>:<substitute>% i.e if VAR exist use it, else use substitute
   //
   // ************************************************************************************************************
   // Special case handling for user variables, and generated variables, which take precedence over node variables
   // ************************************************************************************************************
   //
   // i.e VAR is defined as BILL
   //  %VAR:fred --f%  will either be "BILL" or if VAR is not defined "fred --f"
   //
   // Infinite recursion. Its possible to end up with infinite recursion:
   //    edit hello '%hello%'  # string like %hello% will cause infinite recursion
   //    edit fred '%bill%'
   //    edit bill '%fred%'   # should be 10
   // To prevent this we will use a simple count
   char micro = '%';
   const Variable& micro_var = findVariable(Str::ECF_MICRO());
   if (!micro_var.empty() && !micro_var.theValue().empty() ) micro = micro_var.theValue()[0];

   bool double_micro_found = false;
   std::string::size_type pos = 0;
   int count = 0;
   while ( 1 ) {
      // A while loop here is used to:
      //    a/ Allow for multiple substitution on a single line. i.e %ECF_FILES% -I %ECF_INCLUDE%"
      //    b/ Allow for recursive substitution. %fred% -> %bill%--> 10

      size_t firstPercentPos = cmd.find( micro, pos );
      if ( firstPercentPos == string::npos ) break;

      size_t secondPercentPos = cmd.find( micro, firstPercentPos + 1 );
      if ( secondPercentPos == string::npos ) break;

      if ( secondPercentPos - firstPercentPos <= 1 ) {
         // handle %% with no characters in between, skip over
         // i.e to handle "printf %%02d %HOUR:00%" --> "printf %02d 00"   i.e if HOUR not defined
         pos = secondPercentPos + 1;
         double_micro_found = true;
         continue;
      }
      else pos = 0;

      string percentVar( cmd.begin() + firstPercentPos+1, cmd.begin() + secondPercentPos );

      // First search user variable (*ONLY* set when doing user edit's the script)
      // Handle case: cmd = "%fred:bill% and where we have user variable "fred:bill"
      // Handle case: cmd = "%fred%      and where we have user variable "fred"
      // If we fail to find the variable we return false.
      // Note: When a variable is found, it can have an empty value  which is still valid
      const Variable& variable = findVariable( percentVar );
      if (!variable.empty() ) {
         std::string varValue = variable.theValue();
         cmd.replace( firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue );
      }
      else {

         size_t firstColon = percentVar.find( ':' );
         if (firstColon != string::npos) {

            string var(percentVar.begin(), percentVar.begin() + firstColon);

            const Variable& variable2 = findVariable( var );
            if (!variable2.empty() ) {
               std::string varValue = variable2.theValue();
               cmd.replace( firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue );
            }
            else {
               string substitute (percentVar.begin()+ firstColon+1, percentVar.end());
               cmd.replace(firstPercentPos,secondPercentPos-firstPercentPos+1,substitute);
            }
         }
         else {
            // No Colon, Can't find in user variables, or node variable, hence can't go any further
            return false;
         }
      }

      // Simple Check for infinite recursion
      if (count > 100)  return false;
      count++;
   }

   if (double_micro_found) {
      // replace all double micro with a single micro, this must be a single parse
      // date +%%Y%%m%%d" ==> date +%Y%m%d
      // %%%%             ==> %%            // i.e single parse
      std::string doubleEcfMicro;
      doubleEcfMicro += micro;
      doubleEcfMicro += micro;
      size_t last_pos = 0;
      while ( 1 ) {
          string::size_type ecf_double_micro_pos = cmd.find( doubleEcfMicro , last_pos);
          if ( ecf_double_micro_pos != std::string::npos ) {
             cmd.erase( cmd.begin() + ecf_double_micro_pos );
             last_pos = ecf_double_micro_pos + 1;
          }
          else break;
       }
   }

   return true;
}

void ServerState::set_user_variables(const std::vector<Variable>& e)
{
   user_variables_ = e;
   variable_state_change_no_ = Ecf::incr_state_change_no();
}

void ServerState::set_state(SState::State s) {
	server_state_ = s;
	state_change_no_ = Ecf::incr_state_change_no();
}


void ServerState::setup_default_env()
{
	// This environment is required for testing in the absence of the server.
	// When the defs file is begun in the server this environment get *overridden*
	hostPort_ = std::make_pair(Str::LOCALHOST(),Str::DEFAULT_PORT_NUMBER());

	setup_default_server_variables(server_variables_,Str::DEFAULT_PORT_NUMBER());
}

void ServerState::setup_default_server_variables(std::vector<Variable>&  server_variables, const std::string& port)
{
   Host host;
   server_variables.push_back( Variable(Str::ECF_MICRO(), Ecf::MICRO() )); //Preprocessor character for variable substitution and including files
   server_variables.push_back( Variable(Str::ECF_HOME(), string(".")) );
   server_variables.push_back( Variable(string("ECF_JOB_CMD"), Ecf::JOB_CMD() )); //Command to be executed to submit a job
   server_variables.push_back( Variable(string("ECF_KILL_CMD"), Ecf::KILL_CMD() )); // Command to be executed to kill a job
   server_variables.push_back( Variable(string("ECF_STATUS_CMD"), Ecf::STATUS_CMD() )); // Command to be executed to kill a job
   server_variables.push_back( Variable(string("ECF_URL_CMD"), Ecf::URL_CMD() ));
   server_variables.push_back( Variable(string("ECF_URL_BASE"), Ecf::URL_BASE() ));
   server_variables.push_back( Variable(string("ECF_URL"), Ecf::URL() ));
   server_variables.push_back( Variable(string("ECF_LOG"), host.ecf_log_file(port) ));
   server_variables.push_back( Variable(string("ECF_INTERVAL"), string("60") ));            // Check time dependencies and submit any jobs
   server_variables.push_back( Variable(string("ECF_LISTS"), host.ecf_lists_file(port) ));
   server_variables.push_back( Variable(string("ECF_CHECK"), host.ecf_checkpt_file(port) ));
   server_variables.push_back( Variable(string("ECF_CHECKOLD"), host.ecf_backup_checkpt_file(port)));
   server_variables.push_back( Variable(string("ECF_CHECKINTERVAL"), string("120") ));      //The interval in seconds to save check point file
   server_variables.push_back( Variable(string("ECF_CHECKMODE"), string("CHECK_ON_TIME")) );//The check mode, must be one of NEVER, ON_TIME, ALWAYS

   // Number of times a job should rerun if it aborts. If more than one and
   // job aborts, the job is automatically re-run. Useful when jobs are run in
   // an unreliable environments. For example using using commands like ftp(1)
   // in a job can fail easily, but re-running the job will often work
   server_variables.push_back( Variable(Str::ECF_TRIES(), string("2")) );

   server_variables.push_back( Variable(string("ECF_VERSION"),Version::raw()) );// server version

   // Needed to setup client environment.
   // The server sets these variable for use by the client. i.e when creating the jobs
   // The clients then uses them to communicate with the server.
   server_variables.push_back( Variable(Str::ECF_PORT(),port) );
   server_variables.push_back( Variable(Str::ECF_NODE(),Str::LOCALHOST()) );
}

/// determines why the node is not running.
void ServerState::why(std::vector<std::string>& theReasonWhy) const
{
   if (server_state_ == SState::HALTED)   theReasonWhy.push_back("The server is halted");
   if (server_state_ == SState::SHUTDOWN) theReasonWhy.push_back("The server is shutdown");
}
