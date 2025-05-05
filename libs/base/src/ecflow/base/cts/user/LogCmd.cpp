/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/LogCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"

using namespace ecf;
using namespace std;
namespace po = boost::program_options;

LogCmd::LogCmd(LogApi a, int get_last_n_lines) : api_(a), get_last_n_lines_(get_last_n_lines) {
    if (get_last_n_lines_ == 0)
        get_last_n_lines_ = Log::get_last_n_lines_default();
}

LogCmd::LogCmd() : get_last_n_lines_(Log::get_last_n_lines_default()) {
}

LogCmd::LogCmd(const std::string& path)
    : api_(NEW),
      get_last_n_lines_(Log::get_last_n_lines_default()),
      new_path_(path) {
    // ECFLOW-154, If path to new log file is specified, it should only be checked by the server,
    //             as that could be on a different machine.
    // ECFLOW-174, Never get the full log, as this can make server consume to much memory
    //             default taken from get_last_n_lines_default
    // ECFLOW-377, should remove leading/trailing spaces from path
    ecf::algorithm::trim(new_path_);
}

void LogCmd::print(std::string& os) const {
    switch (api_) {
        case LogCmd::GET:
            user_cmd(os, CtsApi::to_string(CtsApi::getLog(get_last_n_lines_)));
            break;
        case LogCmd::CLEAR:
            user_cmd(os, CtsApi::clearLog());
            break;
        case LogCmd::FLUSH:
            user_cmd(os, CtsApi::flushLog());
            break;
        case LogCmd::NEW:
            user_cmd(os, CtsApi::to_string(CtsApi::new_log(new_path_)));
            break;
        case LogCmd::PATH:
            user_cmd(os, CtsApi::get_log_path());
            break;
        default:
            throw std::runtime_error("LogCmd::print: Unrecognised log api command,");
    }
}
void LogCmd::print_only(std::string& os) const {
    switch (api_) {
        case LogCmd::GET:
            os += CtsApi::to_string(CtsApi::getLog(get_last_n_lines_));
            break;
        case LogCmd::CLEAR:
            os += CtsApi::clearLog();
            break;
        case LogCmd::FLUSH:
            os += CtsApi::flushLog();
            break;
        case LogCmd::NEW:
            os += CtsApi::to_string(CtsApi::new_log(new_path_));
            break;
        case LogCmd::PATH:
            os += CtsApi::get_log_path();
            break;
        default:
            throw std::runtime_error("LogCmd::print: Unrecognised log api command,");
    }
}

bool LogCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<LogCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (api_ != the_rhs->api())
        return false;
    if (get_last_n_lines_ != the_rhs->get_last_n_lines())
        return false;
    if (new_path_ != the_rhs->new_path())
        return false;
    return UserCmd::equals(rhs);
}

// changed for release 4.1.0
bool LogCmd::isWrite() const {
    switch (api_) {
        case LogCmd::GET:
            return false;
            break;
        case LogCmd::CLEAR:
            return false;
            break;
        case LogCmd::FLUSH:
            return false;
            break;
        case LogCmd::NEW:
            return true;
            break;
        case LogCmd::PATH:
            return false;
            break;
        default:
            throw std::runtime_error("LogCmd::isWrite: Unrecognised log api command,");
    }
    return false;
}

STC_Cmd_ptr LogCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().log_cmd_++;

    switch (api_) {
        case LogCmd::GET:
            return PreAllocatedReply::string_cmd(Log::instance()->contents(get_last_n_lines_));
        case LogCmd::CLEAR:
            Log::instance()->clear();
            break;
        case LogCmd::FLUSH:
            Log::instance()->flush();
            break;
        case LogCmd::NEW: {
            if (!new_path_.empty()) {
                Log::instance()->new_path(new_path_); // will throw for errors

                // *NOTE* calling --log=new <path> should be treated the *SAME* as editing ECF_LOG in the GUI
                //        This is done adding it as a *USER* variable. This overloads the server variables
                //        It also allows us to see the change in GUI. Note: Defs/server_variables are not synced
                // ECFLOW-376
                as->defs()->set_server().add_or_update_user_variables(ecf::environment::ECF_LOG,
                                                                      Log::instance()->path());
            }
            else {
                // User could have overridden ECF_LOG variable
                // *FIRST* look at user variables, then look at *server* variables.
                std::string log_file_name = as->defs()->server().find_variable(ecf::environment::ECF_LOG);

                // ECFLOW-377 should remove leading/trailing spaces from path
                ecf::algorithm::trim(log_file_name);

                Log::instance()->new_path(log_file_name); // will throw for errors
            }

            as->stats().ECF_LOG_ = Log::instance()->path(); // do NOT update number of requests
            break;
        }
        case LogCmd::PATH:
            return PreAllocatedReply::string_cmd(Log::instance()->path());
        default:
            throw std::runtime_error("Unrecognised log api command,");
    }
    return PreAllocatedReply::ok_cmd();
}

const char* LogCmd::arg() {
    return "log";
}
const char* LogCmd::desc() {
    return "Get,clear,flush or create a new log file.\n"
           "The user must ensure that a valid path is specified.\n"
           "Specifying '--log=get' with a large number of lines from the server,\n"
           "can consume a lot of **memory**. The log file can be a very large file,\n"
           "hence we use a default of 100 lines, optionally the number of lines can be specified.\n"
           " arg1 = [ get | clear | flush | new | path ]\n"
           "  get -   Outputs the log file to standard out.\n"
           "          defaults to return the last 100 lines\n"
           "          The second argument can specify how many lines to return\n"
           "  clear - Clear the log file of its contents.\n"
           "  flush - Flush and close the log file. (only temporary) next time\n"
           "          server writes to log, it will be opened again. Hence it best\n"
           "          to halt the server first\n"
           "  new -   Flush and close the existing log file, and start using the\n"
           "          the path defined for ECF_LOG. By changing this variable\n"
           "          a new log file path can be used\n"
           "          Alternatively an explicit path can also be provided\n"
           "          in which case ECF_LOG is also updated\n"
           "  path -  Returns the path name to the existing log file\n"
           " arg2 = [ new_path | optional last n lines ]\n"
           "         if get specified can specify lines to get. Value must be convertible to an integer\n"
           "         Otherwise if arg1 is 'new' then the second argument must be a path\n"
           "Usage:\n"
           "  --log=get                        # Write the last 100 lines of the log file to standard out\n"
           "  --log=get 200                    # Write the last 200 lines of the log file to standard out\n"
           "  --log=clear                      # Clear the log file. The log is now empty\n"
           "  --log=flush                      # Flush and close log file, next request will re-open log file\n"
           "  --log=new /path/to/new/log/file  # Close and flush log file, and create a new log file, updates ECF_LOG\n"
           "  --log=new                        # Close and flush log file, and create a new log file using ECF_LOG "
           "variable\n";
}

void LogCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(LogCmd::arg(), po::value<vector<string>>()->multitoken(), LogCmd::desc());
}

void LogCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (ac->debug())
        dumpVecArgs(LogCmd::arg(), args);

    if (!args.empty() && args[0] == "get") {

        if (args.size() != 1 && args.size() != 2) {
            std::stringstream ss;
            ss << "LogCmd: Please use '--log==get 100' to get the log file contents from the server\n";
            ss << "optionally an integer can be provide to specify the last number of lines\n";
            throw std::runtime_error(ss.str());
        }

        if (args.size() == 1) {
            // This will retrieve Log::get_last_n_lines_default() lines from the log file.
            cmd = Cmd_ptr(new LogCmd(LogCmd::GET, Log::get_last_n_lines_default()));
            return;
        }

        int value = Log::get_last_n_lines_default();
        if (args.size() == 2) {
            try {
                value = ecf::convert_to<int>(args[1]);
            }
            catch (const ecf::bad_conversion&) {
                throw std::runtime_error("LogCmd: Second argument must be a integer, i.e. --log get 100\n");
            }
        }

        cmd = std::make_shared<LogCmd>(LogCmd::GET, value);
        return;
    }

    if (!args.empty() && args[0] == "clear") {

        if (args.size() != 1) {
            std::stringstream ss;
            ss << "LogCmd: Too many arguments. Please use " << CtsApi::clearLog() << " to clear the log file\n";
            throw std::runtime_error(ss.str());
        }
        cmd = std::make_shared<LogCmd>(LogCmd::CLEAR);
        return;
    }
    if (!args.empty() && args[0] == "flush") {

        if (args.size() != 1) {
            std::stringstream ss;
            ss << "LogCmd: Too many arguments. Please use " << CtsApi::flushLog() << " to flush the log file\n";
            throw std::runtime_error(ss.str());
        }
        cmd = std::make_shared<LogCmd>(LogCmd::FLUSH);
        return;
    }
    if (!args.empty() && args[0] == "path") {

        if (args.size() != 1) {
            std::stringstream ss;
            ss << "LogCmd: Too many arguments. Please use " << CtsApi::get_log_path() << " to get the log file path\n";
            throw std::runtime_error(ss.str());
        }
        cmd = std::make_shared<LogCmd>(LogCmd::PATH);
        return;
    }
    if (!args.empty() && args[0] == "new") {

        if (args.size() > 2) {
            std::stringstream ss;
            ss << "LogCmd: Too many arguments.  Expected --log=new   OR   --log=new /path/to/newlog/file\n";
            throw std::runtime_error(ss.str());
        }
        std::string path;
        if (args.size() == 2) {
            path = args[1];
        }
        cmd = std::make_shared<LogCmd>(path);
        return;
    }

    std::stringstream ss;
    ss << "LogCmd: The arguments have not been specified correctly\n" << LogCmd::desc();
    throw std::runtime_error(ss.str());
}

std::ostream& operator<<(std::ostream& os, const LogCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(LogCmd)
CEREAL_REGISTER_DYNAMIC_INIT(LogCmd)
