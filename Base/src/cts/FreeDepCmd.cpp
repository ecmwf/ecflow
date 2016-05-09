/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #28 $ 
//
// Copyright 2009-2016 ECMWF. 
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
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Node.hpp"
#include "SuiteChanged.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//=======================================================================================

bool FreeDepCmd::equals(ClientToServerCmd* rhs) const
{
	FreeDepCmd* the_rhs = dynamic_cast<FreeDepCmd*>(rhs);
	if (!the_rhs)  return false;
	if ( paths_   != the_rhs->paths())   { return false;}
	if ( all_     != the_rhs->all())     { return false; }
	if ( trigger_ != the_rhs->trigger()) { return false; }
	if ( date_    != the_rhs->date())    { return false; }
	if ( time_    != the_rhs->time())    { return false; }
	return UserCmd::equals(rhs);
}

std::ostream& FreeDepCmd::print(std::ostream& os) const
{
	return user_cmd(os,CtsApi::to_string(CtsApi::freeDep(paths_,trigger_,all_,date_,time_)));
}

std::ostream& FreeDepCmd::print(std::ostream& os, const std::string& path) const
{
   std::vector<std::string> paths(1,path);
	return user_cmd(os,CtsApi::to_string(CtsApi::freeDep(paths,trigger_,all_,date_,time_)));
}

STC_Cmd_ptr FreeDepCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().free_dep_++;

   std::stringstream ss;
	size_t vec_size = paths_.size();
	for(size_t i = 0; i < vec_size; i++) {

	   node_ptr node = find_node_for_edit_no_throw(as,paths_[i]);
      if (!node.get()) {
         ss << "FreeDepCmd: Could not find node at path " << paths_[i] << "\n";
         LOG(Log::ERR,"FreeDepCmd: Could not find node at path " << paths_[i]);
         continue;
      }

	   SuiteChanged0 changed(node);
	   if (all_) {
	      node->freeTrigger();
	      node->freeHoldingDateDependencies();
	      node->freeHoldingTimeDependencies();
	   }
	   else {
	      if (trigger_) node->freeTrigger();
	      if (date_)    node->freeHoldingDateDependencies();
	      if (time_)    node->freeHoldingTimeDependencies();
	   }
	}

   std::string error_msg = ss.str();
   if (!error_msg.empty()) {
      throw std::runtime_error( error_msg ) ;
   }

   return doJobSubmission( as );
}

const char* FreeDepCmd::arg()  { return CtsApi::freeDepArg();}
const char* FreeDepCmd::desc() {
   return
            "Free dependencies for a node. Defaults to triggers\n"
            "After freeing the time related dependencies (i.e time,today,cron)\n"
            "the next time slot will be missed.\n"
            "  arg1 = (optional) trigger\n"
            "  arg2 = (optional) all\n"
            "         Free trigger, date and all time dependencies\n"
            "  arg3 = (optional) date\n"
            "         Free date dependencies\n"
            "  arg4 = (optional) time\n"
            "         Free all time dependencies i.e time, day, today, cron\n"
            "  arg5 = List of paths. At least one required. Must start with a leading '/'\n"
            "Usage:\n"
            "  --free-dep=/s1/t1 /s2/t2   # free trigger dependencies for task's t1,t2\n"
            "  --free-dep=all /s1/f1/t1   # free all dependencies of /s1/f1/t1\n"
            "  --free-dep=date /s1/f1     # free holding date dependencies of /s1/f1"
            ;
}

void FreeDepCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( FreeDepCmd::arg(),po::value< vector<string> >()->multitoken(), FreeDepCmd::desc() );
}
void FreeDepCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv*  ac) const
{
	vector<string> args = vm[  arg() ].as< vector<string> >();

	if (ac->debug()) dumpVecArgs(FreeDepCmd::arg(),args);

	if (args.size() < 1 ) {
		std::stringstream ss;
		ss << "FreeDepCmd: At least one arguments expected for Free dependencies. Found " << args.size() << "\n" << FreeDepCmd::desc() << "\n";
		throw std::runtime_error( ss.str() );
	}

   std::vector<std::string> options,paths;
   split_args_to_options_and_paths(args,options,paths); // relative order is still preserved
   if (paths.empty()) {
      std::stringstream ss;
      ss << "FreeDepCmd: No paths specified. Paths must begin with a leading '/' character\n" << FreeDepCmd::desc() << "\n";
      throw std::runtime_error( ss.str() );
   }

	bool trigger = options.empty(); // If no options default to freeing trigger dependencies
	bool all = false;
	bool date = false;
	bool time = false;
   size_t vec_size = options.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (options[i] == "trigger")    trigger = true;
      else if ( options[i] == "all")  all  = true;
      else if ( options[i] == "date") date = true;
      else if ( options[i] == "time") time = true;
      else {
         std::stringstream ss;
         ss << "FreeDepCmd: Invalid argument(" << options[i] << ")\n" << FreeDepCmd::desc() << "\n";
         throw std::runtime_error( ss.str() );
      }
   }
   assert(trigger || all || date ||  time); // at least one must be true
	cmd = Cmd_ptr( new FreeDepCmd(paths, trigger, all, date , time) );
}

std::ostream& operator<<(std::ostream& os, const FreeDepCmd& c) { return c.print(os); }
