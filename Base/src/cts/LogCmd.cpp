/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #30 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "Log.hpp"
#include "CtsApi.hpp"
#include "Str.hpp"
#include "Defs.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

LogCmd::LogCmd(const std::string& path) : api_(NEW),get_last_n_lines_(0),new_path_(path)
{
   // ECFLOW-154, If path to new log file is specified, it should only be checked by the server,
   //             as that could be on a different machine.
}

std::ostream& LogCmd::print(std::ostream& os) const
{
    switch (api_) {
      case LogCmd::GET:   return user_cmd(os,CtsApi::to_string(CtsApi::getLog(get_last_n_lines_)));  break;
      case LogCmd::CLEAR: return user_cmd(os,CtsApi::clearLog()); break;
      case LogCmd::FLUSH: return user_cmd(os,CtsApi::flushLog()); break;
      case LogCmd::NEW:   return user_cmd(os,CtsApi::to_string(CtsApi::new_log(new_path_)));  break;
      case LogCmd::PATH:  return user_cmd(os,CtsApi::get_log_path()); break;
      default : throw std::runtime_error( "LogCmd::print: Unrecognised log api command,") ;
   }
   return os;
}

bool LogCmd::equals(ClientToServerCmd* rhs) const
{
	LogCmd* the_rhs = dynamic_cast< LogCmd* > ( rhs );
	if ( !the_rhs ) return false;
	if (api_ != the_rhs->api()) return false;
	if (get_last_n_lines_ != the_rhs->get_last_n_lines()) return false;
	if (new_path_ != the_rhs->new_path()) return false;
	return UserCmd::equals(rhs);
}

STC_Cmd_ptr LogCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().log_cmd_++;

	if (Log::instance()) {
		switch (api_) {
			case LogCmd::GET:   return PreAllocatedReply::string_cmd( Log::instance()->contents(get_last_n_lines_) ); break;
			case LogCmd::CLEAR: Log::instance()->clear(); break;
			case LogCmd::FLUSH: Log::instance()->flush(); break;
         case LogCmd::NEW:   {
            if (!new_path_.empty()) {
               Log::instance()->new_path(new_path_); // will throw for errors

               // Update the corresponding server variable
               if (as->defs()) {
                  NameValueVec vec;
                  vec.push_back(std::make_pair(Str::ECF_LOG(),Log::instance()->path()));
                  as->defs()->set_server().add_or_update_server_variables(vec);
               }
            }
            else {
               if (as->defs()) {
                  // User could have overridden ECF_LOG variable
                  const std::string& log_file_name = as->defs()->server().find_variable(Str::ECF_LOG());
                  Log::instance()->new_path(log_file_name);  // will throw for errors
               }
               else {
                  // Re-use reload the existing file, flushing will close file, next log command will re-open it
                  Log::instance()->flush();
               }
            }

            as->stats().ECF_LOG_ = Log::instance()->path();  // do NOT update number of requests
            break;
         }
         case LogCmd::PATH:  return PreAllocatedReply::string_cmd(  Log::instance()->path() ); break;
			default : throw std::runtime_error( "Unrecognised log api command,") ;
		}
	}
	return PreAllocatedReply::ok_cmd();
}

const char* LogCmd::arg() { return "log";}
const char* LogCmd::desc() {
   return  "Get,clear,flush or create a new log file.\n"
            "The user must ensure that a valid path is specified.\n"
            "However for *test* we also provide functionality to get the\n"
            "log file.This could be a very large file, and should not generally be used,\n"
            "optionally the number of lines can be specified.\n"
            "  arg1 = [ get | clear | flush | new | path ]\n"
            "         get   -  Outputs the log file to standard out. Not for general usage\n"
            "                  The second argument can specify how many lines to return\n"
            "         clear -  Clear the log file of its contents.\n"
            "         flush -  Flush and close the log file. (only temporary) next time\n"
            "                  server writes to log, it will be opened again. Hence it best\n"
            "                  to halt the server first\n"
            "         new   -  Flush and close the existing log file, and start using the\n"
            "                  the path defined for ECF_LOG. By changing this variable\n"
            "                  a new log file path can be used\n"
            "                  Alternatively an explicit path can also be provided\n"
            "                  in which case ECF_LOG is also updated\n"
            "         path  -  Returns the path name to the existing log file\n"
            "  arg2 = [ new_path | optional last n lines ]\n"
            "         if get specified can specify lines to get. Value must be convertible to an integer\n"
            "         Otherwise if arg1 is 'new' then the second argument must be a path\n"
            "Usage:\n"
            "  --log=get 200                    # Write the last 200 lines of the log file to standard out\n"
            "  --log=clear                      # Clear the log file. The log is now empty\n"
            "  --log=flush                      # Flush and close log file, next request will re-open log file\n"
            "  --log=new /path/to/new/log/file  # Close and flush log file, and create a new log file, updates ECF_LOG\n"
            "  --log=new                        # Close and flush log file, and create a new log file using ECF_LOG variable"
            ;
}

void LogCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( LogCmd::arg(), po::value< vector<string> >()->multitoken(), LogCmd::desc() );
}

void LogCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* ac) const
{
	vector<string> args = vm[ arg() ].as< vector<string> >();

	if (ac->debug())   dumpVecArgs(LogCmd::arg(),args);

	if (!args.empty() && args[0] == "get")  {

		if ( args.size() != 1 && args.size() != 2) {
			std::stringstream ss;
			ss << "LogCmd: Please use '--log==get 100' to get the log file contents from the server\n";
			ss << "optionally an integer can be provide to specify the last number of lines\n";
			throw std::runtime_error( ss.str() );
		}

		if (args.size() == 1 ) {
			// retrieve the full log from the server. Could be a mega file. be warned
			cmd = Cmd_ptr( new LogCmd( LogCmd::GET ) );
			return;
		}

 		int value = 0;
 		if (args.size() == 2) {
 			try { value = boost::lexical_cast<int>(args[1]); }
 			catch (boost::bad_lexical_cast& e) {
 				throw std::runtime_error( "LogCmd: Second argument must be a integer, i.e. --log get 100\n" );
 			}
 		}

	 	cmd = Cmd_ptr( new LogCmd( LogCmd::GET, value ) );
	 	return ;
	}

 	if (!args.empty() && args[0] == "clear")   {

		if (args.size() != 1 ) {
			std::stringstream ss;
			ss << "LogCmd: Too many arguments. Please use " << CtsApi::clearLog() << " to clear the log file\n";
 			throw std::runtime_error( ss.str() );
		}
 		cmd = Cmd_ptr( new LogCmd( LogCmd::CLEAR ) );
 		return;
 	}
 	if (!args.empty() && args[0] == "flush")   {

		if (args.size() != 1 ) {
			std::stringstream ss;
			ss << "LogCmd: Too many arguments. Please use " << CtsApi::flushLog() << " to flush the log file\n";
 			throw std::runtime_error( ss.str() );
		}
 		cmd = Cmd_ptr( new LogCmd( LogCmd::FLUSH ) );
 		return;
 	}
   if (!args.empty() && args[0] == "path")   {

      if (args.size() != 1 ) {
         std::stringstream ss;
         ss << "LogCmd: Too many arguments. Please use " << CtsApi::get_log_path() << " to get the log file path\n";
         throw std::runtime_error( ss.str() );
      }
      cmd = Cmd_ptr( new LogCmd( LogCmd::PATH ) );
      return;
   }

   if (!args.empty() && args[0] == "new")   {

      if (args.size() > 2 ) {
         std::stringstream ss;
         ss << "LogCmd: Too many arguments.  Expected --log=new   OR   --log=new /path/to/newlog/file\n";
         throw std::runtime_error( ss.str() );
      }
      std::string path;
      if ( args.size() == 2 ) {
         path = args[1];
      }
      cmd = Cmd_ptr( new LogCmd( path ) );
      return;
   }

 	std::stringstream ss;
 	ss << "LogCmd: The arguments have not been specified correctly\n" << LogCmd::desc();
   	throw std::runtime_error( ss.str() );
}

std::ostream& operator<<(std::ostream& os, const LogCmd& c) { return c.print(os); }
