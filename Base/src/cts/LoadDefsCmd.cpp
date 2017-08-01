/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #43 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/make_shared.hpp>
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "Str.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "File.hpp"
#include "PrintStyle.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;


LoadDefsCmd::LoadDefsCmd(const std::string& defs_filename, bool force, bool check_only)
: force_(force), defs_(Defs::create()), defs_filename_(defs_filename)
{
   if (defs_filename_.empty()) {
      std::stringstream ss;
      ss << "LoadDefsCmd::LoadDefsCmd: The pathname to the definition file must be provided\n" << LoadDefsCmd::desc();
      throw std::runtime_error(ss.str());
   }

   // At the end of the parse check the trigger/complete expressions and resolve in-limits
   std::string errMsg, warningMsg;
   if (defs_->restore(defs_filename_, errMsg , warningMsg) ) {
      // Dump out the in memory Node tree
      // std::cout << defs_.get();

      // Output any warning to standard output
      cout << warningMsg;
   }
   else {
      // Check if its a boost file format. (could be old checkpoint file)
      // When default version of ecflow is 4.7  this section could be removed. TODO
      std::string error_msg;
      std::string first_line = File::get_first_n_lines(defs_filename_, 1, error_msg);
      if (!first_line.empty() && error_msg.empty()) {
         if (first_line.find("22 serialization::archive") == 0) {   // boost file format
            // Can be use to check for corruption in boost based checkpoint files.
            defs_->boost_restore_from_checkpt(defs_filename_);
            if (check_only) {
               PrintStyle print_style(PrintStyle::MIGRATE);
               cout << defs_;
            }
            return;
         }
      }

      std::stringstream ss; ss << "\nLoadDefsCmd::LoadDefsCmd.  Failed to parse file " << defs_filename_ << "\n";
      ss << errMsg;
      throw std::runtime_error( ss.str() );
   }
}

bool LoadDefsCmd::equals(ClientToServerCmd* rhs) const
{
	LoadDefsCmd* the_rhs = dynamic_cast<LoadDefsCmd*>(rhs);
	if (!the_rhs)  return false;

	if (!UserCmd::equals(rhs))  return false;

	if (defs_ == NULL && the_rhs->theDefs() == NULL) return true;
	if (defs_ == NULL && the_rhs->theDefs() != NULL) return false;
	if (defs_ != NULL && the_rhs->theDefs() == NULL) return false;

	return (*defs_ == *(the_rhs->theDefs()));
}

STC_Cmd_ptr LoadDefsCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().load_defs_++;

	assert(isWrite()); // isWrite used in handleRequest() to control check pointing
	if (defs_) {

		// After the updateDefs, defs_ will be left with NO suites.
		// Can't really used defs_ after this point
	   // *NOTE* Externs are not persisted. Hence calling check() will report
	   // all errors, references are not resolved.
		as->updateDefs(defs_,force_);
 	}

	return PreAllocatedReply::ok_cmd();
}

std::ostream& LoadDefsCmd::print(std::ostream& os) const
{
   /// If defs_filename_ is empty, the Defs was a in memory defs.
   if (defs_filename_.empty()) {
      return user_cmd(os,CtsApi::to_string(CtsApi::loadDefs("<in-memory-defs>",force_,false/*check_only*/)));
   }
   return user_cmd(os,CtsApi::to_string(CtsApi::loadDefs(defs_filename_,force_,false/*check_only*/)));
}

const char* LoadDefsCmd::arg()  { return CtsApi::loadDefsArg();}
const char* LoadDefsCmd::desc() {
   return   "Check and load definition file into server.\n"
            "The loaded definition will be checked for valid trigger and complete expressions,\n"
            "additionally in-limit references to limits will be validated.\n"
            "If the server already has the 'suites' of the same name, then a error message is issued.\n"
            "The suite's can be overwritten if the force option is used.\n"
            "To just check the definition and not send to server, use 'check_only'\n"
            "  arg1 = path to the definition file\n"
            "  arg2 = (optional) [ force | check_only ]   # default = false for both\n"
            "Usage:\n"
            "--load=/my/home/exotic.def             # will error if suites of same name exists\n"
            "--load=/my/home/exotic.def force       # overwrite suite's of same name\n"
            "--load=/my/home/exotic.def check_only  # Just check, don't send to server"
            ;
}

void LoadDefsCmd::addOption(boost::program_options::options_description& desc) const{
	desc.add_options()( LoadDefsCmd::arg(),  po::value< vector<string> >()->multitoken(),  LoadDefsCmd:: desc() );
}

void LoadDefsCmd::create( 	Cmd_ptr& cmd,
							boost::program_options::variables_map& vm,
							AbstractClientEnv* clientEnv ) const
{
	vector<string> args = vm[  arg() ].as< vector<string> >();
	if (clientEnv->debug()) dumpVecArgs(LoadDefsCmd::arg(),args);

	bool check_only = false;
	bool force =  false;
	std::string defs_filename;
	for(size_t i = 0; i < args.size(); i++) {
		if (args[i] == "force") force = true;
		else if (args[i] == "check_only") check_only = true;
		else defs_filename = args[i];
	}
	if (clientEnv->debug()) cout << "  LoadDefsCmd::create:  Defs file '" <<  defs_filename << "'.\n";

	cmd = LoadDefsCmd::create(defs_filename,force, check_only,clientEnv );
}

Cmd_ptr LoadDefsCmd::create(const std::string& defs_filename, bool force, bool check_only, AbstractClientEnv* clientEnv)
{
   // The constructor can throw if parsing of defs_filename fail's
   boost::shared_ptr<LoadDefsCmd> load_cmd = boost::make_shared<LoadDefsCmd>(defs_filename,force,check_only);

   // Don't send to server if checking, i.e cmd not set
   if (check_only) return Cmd_ptr();

   // For test allow the server environment to be changed, i.e. allow us to inject ECF_CLIENT
   // The server will also update the env on the defs, server will override client env, where they clash
   load_cmd->theDefs()->set_server().add_or_update_user_variables( clientEnv->env() );

   return load_cmd;
}


std::ostream& operator<<(std::ostream& os, const LoadDefsCmd& c)  { return c.print(os); }
