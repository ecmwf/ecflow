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

LoadDefsCmd::LoadDefsCmd(const defs_ptr& defs, bool force)
 : force_(force)
{
   if (defs) {
      defs->save_as_string(defs_,PrintStyle::MIGRATE);
   }
}

LoadDefsCmd::LoadDefsCmd(const std::string& defs_filename, bool force, bool check_only, bool print,
                         const std::vector<std::pair<std::string,std::string> >& client_env)
: force_(force), defs_filename_(defs_filename)
{
   if (defs_filename_.empty()) {
      std::stringstream ss;
      ss << "LoadDefsCmd::LoadDefsCmd: The pathname to the definition file must be provided\n" << LoadDefsCmd::desc();
      throw std::runtime_error(ss.str());
   }

   // At the end of the parse check the trigger/complete expressions and resolve in-limits
   defs_ptr defs = Defs::create();
   std::string errMsg, warningMsg;
   if (defs->restore(defs_filename_, errMsg , warningMsg) ) {

      defs->set_server().add_or_update_user_variables( client_env ); // use in test environment

      if (print) {
         PrintStyle print_style(PrintStyle::MIGRATE);
         cout << defs;
      }

      defs->save_as_string(defs_,PrintStyle::MIGRATE);

      // Output any warning to standard output
      cout << warningMsg;
   }
   else {
      std::stringstream ss; ss << "\nLoadDefsCmd::LoadDefsCmd. Failed to parse file " << defs_filename_ << "\n";
      ss << errMsg;
      throw std::runtime_error( ss.str() );
   }
}

bool LoadDefsCmd::equals(ClientToServerCmd* rhs) const
{
	LoadDefsCmd* the_rhs = dynamic_cast<LoadDefsCmd*>(rhs);
	if (!the_rhs)  return false;
	if (!UserCmd::equals(rhs))  return false;
	if (defs_  != the_rhs->defs_as_string()) return false;
	return true;
}

STC_Cmd_ptr LoadDefsCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().load_defs_++;
	assert(isWrite()); // isWrite used in handleRequest() to control check pointing

	if (!defs_.empty()) {

	   // Parse the string and load the defs file into memory.
	   std::string errMsg, warningMsg;
	   defs_ptr defs = Defs::create();
	   if ( ! defs->restore_from_string( defs_ , errMsg , warningMsg) ) {
	      std::stringstream ss;
	      ss << "LoadDefsCmd::doHandleRequest : Could not parse file " <<  defs_filename_  << " : " << errMsg;
	      throw std::runtime_error( ss.str() );
	   }

		// After the updateDefs, defs will be left with NO suites.
		// Can't really used defs after this point
	   // *NOTE* Externs are not persisted. Hence calling check() will report
	   // all errors, references are not resolved.
		as->updateDefs(defs,force_);

		LOG_ASSERT(defs->suiteVec().size() == 0,"Expected suites to be transferred to server defs");
 	}
   LOG_ASSERT(as->defs()->externs().size() == 0,"Expected server to have no externs");

	return PreAllocatedReply::ok_cmd();
}

std::ostream& LoadDefsCmd::print(std::ostream& os) const
{
   /// If defs_filename_ is empty, the Defs was a in memory defs.
   if (defs_filename_.empty()) {
      return user_cmd(os,CtsApi::to_string(CtsApi::loadDefs("<in-memory-defs>",force_,false/*check_only*/,false/*print*/)));
   }
   return user_cmd(os,CtsApi::to_string(CtsApi::loadDefs(defs_filename_,force_,false/*check_only*/,false/*print*/)));
}

const char* LoadDefsCmd::arg()  { return CtsApi::loadDefsArg();}
const char* LoadDefsCmd::desc() {
   return   "Check and load definition or checkpoint file into server.\n"
            "The loaded definition will be checked for valid trigger and complete expressions,\n"
            "additionally in-limit references to limits will be validated.\n"
            "If the server already has the 'suites' of the same name, then a error message is issued.\n"
            "The suite's can be overwritten if the force option is used.\n"
            "To just check the definition and not send to server, use 'check_only'\n"
            "This command can also be used to load a checkpoint file into the server\n"
            "  arg1 = path to the definition file or checkpoint file\n"
            "  arg2 = (optional) [ force | check_only | print ]   # default = false for all\n"
            "Usage:\n"
            "--load=/my/home/exotic.def               # will error if suites of same name exists\n"
            "--load=/my/home/exotic.def force         # overwrite suite's of same name in the server\n"
            "--load=/my/home/exotic.def check_only    # Just check, don't send to server\n"
            "--load=host1.3141.check                  # Load checkpoint file to the server\n"
            "--load=host1.3141.check print check_only # print definition to standard out in defs format\n"
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
	bool force = false;
	bool print = false;
	std::string defs_filename;
	for(size_t i = 0; i < args.size(); i++) {
		if (args[i] == "force") force = true;
      else if (args[i] == "check_only") check_only = true;
      else if (args[i] == "print") print = true;
		else defs_filename = args[i];
	}
	if (clientEnv->debug()) cout << "  LoadDefsCmd::create:  Defs file '" <<  defs_filename << "'.\n";

	cmd = LoadDefsCmd::create(defs_filename,force,check_only,print,clientEnv );
}

Cmd_ptr LoadDefsCmd::create(const std::string& defs_filename, bool force, bool check_only, bool print, AbstractClientEnv* clientEnv)
{
   // For test allow the server environment to be changed, i.e. allow us to inject ECF_CLIENT
   // The server will also update the env on the defs, server will override client env, where they clash

   // The constructor can throw if parsing of defs_filename fail's
   boost::shared_ptr<LoadDefsCmd> load_cmd = boost::make_shared<LoadDefsCmd>(defs_filename,force,check_only,print,clientEnv->env());

   // Don't send to server if checking, i.e cmd not set
   if (check_only) return Cmd_ptr();

   return load_cmd;
}

std::ostream& operator<<(std::ostream& os, const LoadDefsCmd& c)  { return c.print(os); }
