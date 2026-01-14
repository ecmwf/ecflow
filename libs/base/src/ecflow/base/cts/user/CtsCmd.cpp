/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/CtsCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/Gnuplot.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"

using namespace ecf;
namespace po = boost::program_options;

// *IMPORTANT*: STATS_RESET was introduced in release 4.0.5

void CtsCmd::print(std::string& os) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            user_cmd(os, CtsApi::zombieGet());
            break;
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            user_cmd(os, CtsApi::restoreDefsFromCheckPt());
            break;
        case CtsCmd::RESTART_SERVER:
            user_cmd(os, CtsApi::restartServer());
            break;
        case CtsCmd::SHUTDOWN_SERVER:
            user_cmd(os, CtsApi::shutdownServer());
            break;
        case CtsCmd::HALT_SERVER:
            user_cmd(os, CtsApi::haltServer());
            break;
        case CtsCmd::TERMINATE_SERVER:
            user_cmd(os, CtsApi::terminateServer());
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            user_cmd(os, CtsApi::reloadwsfile());
            break;
        case CtsCmd::RELOAD_PASSWD_FILE:
            user_cmd(os, CtsApi::reloadpasswdfile());
            break;
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            user_cmd(os, CtsApi::reloadcustompasswdfile());
            break;
        case CtsCmd::FORCE_DEP_EVAL:
            user_cmd(os, CtsApi::forceDependencyEval());
            break;
        case CtsCmd::PING:
            user_cmd(os, CtsApi::pingServer());
            break;
        case CtsCmd::STATS:
            user_cmd(os, CtsApi::stats());
            break;
        case CtsCmd::STATS_SERVER:
            user_cmd(os, CtsApi::stats_server());
            break;
        case CtsCmd::STATS_RESET:
            user_cmd(os, CtsApi::stats_reset());
            break;
        case CtsCmd::SUITES:
            user_cmd(os, CtsApi::suites());
            break;
        case CtsCmd::DEBUG_SERVER_ON:
            user_cmd(os, CtsApi::debug_server_on());
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            user_cmd(os, CtsApi::debug_server_off());
            break;
        case CtsCmd::SERVER_LOAD:
            user_cmd(os, CtsApi::server_load(""));
            break;
        case CtsCmd::NO_CMD:
            assert(false);
            os += "CtsCmdCtsCmd::NO_CMD  !!!!";
            break;
        default:
            assert(false);
            os += "CtsCmd did not match api_ !!!!";
            break;
    }
}

void CtsCmd::print_only(std::string& os) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            os += CtsApi::zombieGet();
            break;
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            os += CtsApi::restoreDefsFromCheckPt();
            break;
        case CtsCmd::RESTART_SERVER:
            os += CtsApi::restartServer();
            break;
        case CtsCmd::SHUTDOWN_SERVER:
            os += CtsApi::shutdownServer();
            break;
        case CtsCmd::HALT_SERVER:
            os += CtsApi::haltServer();
            break;
        case CtsCmd::TERMINATE_SERVER:
            os += CtsApi::terminateServer();
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            os += CtsApi::reloadwsfile();
            break;
        case CtsCmd::RELOAD_PASSWD_FILE:
            os += CtsApi::reloadpasswdfile();
            break;
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            os += CtsApi::reloadcustompasswdfile();
            break;
        case CtsCmd::FORCE_DEP_EVAL:
            os += CtsApi::forceDependencyEval();
            break;
        case CtsCmd::PING:
            os += CtsApi::pingServer();
            break;
        case CtsCmd::STATS:
            os += CtsApi::stats();
            break;
        case CtsCmd::STATS_SERVER:
            os += CtsApi::stats_server();
            break;
        case CtsCmd::STATS_RESET:
            os += CtsApi::stats_reset();
            break;
        case CtsCmd::SUITES:
            os += CtsApi::suites();
            break;
        case CtsCmd::DEBUG_SERVER_ON:
            os += CtsApi::debug_server_on();
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            os += CtsApi::debug_server_off();
            break;
        case CtsCmd::SERVER_LOAD:
            os += CtsApi::server_load("");
            break;
        case CtsCmd::NO_CMD:
            assert(false);
            os += "CtsCmdCtsCmd::NO_CMD  !!!!";
            break;
        default:
            assert(false);
            os += "CtsCmd did not match api_ !!!!";
            break;
    }
}

bool CtsCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CtsCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (api_ != the_rhs->api()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t CtsCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t CtsCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

bool CtsCmd::isWrite() const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            return false;
            break; // read only
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            return true;
            break; // requires write privilege
        case CtsCmd::RESTART_SERVER:
            return true;
            break; // requires write privilege
        case CtsCmd::SHUTDOWN_SERVER:
            return true;
            break; // requires write privilege
        case CtsCmd::HALT_SERVER:
            return true;
            break; // requires write privilege
        case CtsCmd::TERMINATE_SERVER:
            return true;
            break; // requires write privilege
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            return true;
            break; // requires write privilege
        case CtsCmd::RELOAD_PASSWD_FILE:
            return true;
            break; // requires write privilege
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            return true;
            break; // requires write privilege
        case CtsCmd::FORCE_DEP_EVAL:
            return true;
            break; // requires write privilege
        case CtsCmd::PING:
            return false;
            break; // read only
        case CtsCmd::STATS:
            return false;
            break; // read only
        case CtsCmd::STATS_SERVER:
            return false;
            break; // read only
        case CtsCmd::STATS_RESET:
            return true;
            break; // requires write privilege
        case CtsCmd::SUITES:
            return false;
            break; // read only
        case CtsCmd::DEBUG_SERVER_ON:
            return false;
            break; // read only
        case CtsCmd::DEBUG_SERVER_OFF:
            return false;
            break; // read only
        case CtsCmd::SERVER_LOAD:
            return false;
            break; // read only
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
    assert(false);
    return false;
}

bool CtsCmd::cmd_updates_defs() const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            return false;
            break;
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            return true;
            break;
        case CtsCmd::RESTART_SERVER:
            return true;
            break;
        case CtsCmd::SHUTDOWN_SERVER:
            return true;
            break;
        case CtsCmd::HALT_SERVER:
            return true;
            break;
        case CtsCmd::TERMINATE_SERVER:
            return true;
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            return false;
            break;
        case CtsCmd::RELOAD_PASSWD_FILE:
            return false;
            break;
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            return false;
            break;
        case CtsCmd::FORCE_DEP_EVAL:
            return true;
            break;
        case CtsCmd::PING:
            return false;
            break;
        case CtsCmd::STATS:
            return false;
            break;
        case CtsCmd::STATS_SERVER:
            return false;
            break;
        case CtsCmd::STATS_RESET:
            return false;
            break;
        case CtsCmd::SUITES:
            return false;
            break;
        case CtsCmd::DEBUG_SERVER_ON:
            return false;
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            return false;
            break;
        case CtsCmd::SERVER_LOAD:
            return false;
            break;
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
    assert(false);
    return false;
}

int CtsCmd::timeout() const {
    if (api_ == CtsCmd::PING) {
        return 10;
    }
    return ClientToServerCmd::timeout();
}

const char* CtsCmd::theArg() const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            return CtsApi::zombieGetArg();
            break;
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            return CtsApi::restoreDefsFromCheckPtArg();
            break;
        case CtsCmd::RESTART_SERVER:
            return CtsApi::restartServerArg();
            break;
        case CtsCmd::SHUTDOWN_SERVER:
            return CtsApi::shutdownServerArg();
            break;
        case CtsCmd::HALT_SERVER:
            return CtsApi::haltServerArg();
            break;
        case CtsCmd::TERMINATE_SERVER:
            return CtsApi::terminateServerArg();
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            return CtsApi::reloadwsfileArg();
            break;
        case CtsCmd::RELOAD_PASSWD_FILE:
            return CtsApi::reloadpasswdfile_arg();
            break;
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            return CtsApi::reloadcustompasswdfile_arg();
            break;
        case CtsCmd::FORCE_DEP_EVAL:
            return CtsApi::forceDependencyEvalArg();
            break;
        case CtsCmd::PING:
            return CtsApi::pingServerArg();
            break;
        case CtsCmd::STATS:
            return CtsApi::statsArg();
            break;
        case CtsCmd::STATS_SERVER:
            return CtsApi::stats_server_arg();
            break;
        case CtsCmd::STATS_RESET:
            return CtsApi::stats_reset_arg();
            break;
        case CtsCmd::SUITES:
            return CtsApi::suitesArg();
            break;
        case CtsCmd::DEBUG_SERVER_ON:
            return CtsApi::debug_server_on_arg();
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            return CtsApi::debug_server_off_arg();
            break;
        case CtsCmd::SERVER_LOAD:
            return CtsApi::server_load_arg();
            break;
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
    assert(false);
    return nullptr;
}

STC_Cmd_ptr CtsCmd::doHandleRequest(AbstractServer* as) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES: {
            as->update_stats().zombie_get_++;
            return PreAllocatedReply::zombie_get_cmd(as);
        }

        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT: {
            as->update_stats().restore_defs_from_checkpt_++;
            as->restore_defs_from_checkpt(); // this can throw, i.e. if server not halted, or defs has suites, etc
            break;
        }

        case CtsCmd::RESTART_SERVER: {
            as->update_stats().restart_server_++;
            as->restart();
            return doJobSubmission(as);
        }
        case CtsCmd::SHUTDOWN_SERVER:
            as->update_stats().shutdown_server_++;
            as->shutdown();
            break;
        case CtsCmd::HALT_SERVER:
            as->update_stats().halt_server_++;
            as->halted();
            break;
        case CtsCmd::TERMINATE_SERVER:
            as->checkPtDefs();
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE: {
            as->update_stats().reload_white_list_file_++;
            std::string errorMsg;
            if (!as->reloadWhiteListFile(errorMsg)) {
                throw std::runtime_error(errorMsg);
            }
            break;
        }
        case CtsCmd::RELOAD_PASSWD_FILE: {
            as->update_stats().reload_passwd_file_++;
            std::string errorMsg;
            if (!as->reloadPasswdFile(errorMsg)) {
                throw std::runtime_error(errorMsg);
            }
            break;
        }
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE: {
            as->update_stats().reload_passwd_file_++;
            std::string errorMsg;
            if (!as->reloadCustomPasswdFile(errorMsg)) {
                throw std::runtime_error(errorMsg);
            }
            break;
        }
        case CtsCmd::FORCE_DEP_EVAL: {
            // The Default JobsParam does *not* allow Job creation, & hence => does not submit jobs
            // The default does *not* allow job spawning
            Jobs jobs(as->defs());
            JobsParam jobsParam; // create jobs =  false, spawn_jobs = false
            if (!jobs.generate(jobsParam)) {
                throw std::runtime_error(jobsParam.getErrorMsg());
            }
            break;
        }
        case CtsCmd::PING:
            as->update_stats().ping_++;
            break;
        case CtsCmd::STATS: {
            as->update_stats().stats_++;
            std::stringstream ss;
            as->stats().update_for_serialisation();
            as->stats().no_of_suites_ = as->defs()->suiteVec().size();
            as->stats().show(ss); // ECFLOW-880, allow stats to be changed in server, by only returning string
            return PreAllocatedReply::string_cmd(ss.str());
            break;
        }
        case CtsCmd::STATS_SERVER: {
            as->update_stats().stats_++;
            return PreAllocatedReply::stats_cmd(as);
            break; // Only to be used in test, as subject to change, returns Stats struct
        }
        case CtsCmd::STATS_RESET:
            as->update_stats().reset();
            break; // we could have done as->update_stats().stats_++, to honor reset, we dont
        case CtsCmd::SUITES:
            as->update_stats().suites_++;
            return PreAllocatedReply::suites_cmd(as);
            break;
        case CtsCmd::DEBUG_SERVER_ON:
            as->update_stats().debug_server_on_++;
            as->debug_server_on();
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            as->update_stats().debug_server_off_++;
            as->debug_server_off();
            break;
        case CtsCmd::SERVER_LOAD: {
            as->update_stats().server_load_cmd_++;
            return PreAllocatedReply::server_load_cmd(Log::instance()->path());
        }
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }

    return PreAllocatedReply::ok_cmd();
}

static const char* server_load_desc() {
    /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
    return "Generates gnuplot files that show the server load graphically.\n"
           "This is done by parsing the log file. If no log file is provided,\n"
           "then the log file path is obtained from the server. If the returned\n"
           "log file path is not accessible an error is returned\n"
           "This command produces a three files in the CWD.\n"
           "    o <host>.<port>.gnuplot.dat\n"
           "    o <host>.<port>.gnuplot.script\n"
           "    o <host>.<port>.png\n\n"
           "The generated script can be manually changed, to see different rendering\n"
           "effects. i.e. just run 'gnuplot <host>.<port>.gnuplot.script'\n\n"
           "  arg1 = <optional> path to log file\n\n"
           "If the path to log file is known, it is *preferable* to use this,\n"
           "rather than requesting the log path from the server.\n\n"
           "Usage:\n"
           "   --server_load=/path/to_log_file  # Parses log and generate gnuplot files\n"
           "   --server_load                    # Log file path is requested from server\n"
           "                                    # which is then used to generate gnuplot files\n"
           "                                    # *AVOID* if log file path is accessible\n\n"
           "Now use any png viewer to see the output i.e\n\n"
           "> display   <host>.<port>.png\n"
           "> feh       <host>.<port>.png\n"
           "> eog       <host>.<port>.png\n"
           "> xdg-open  <host>.<port>.png\n"
           "> w3m       <host>.<port>.png\n";
}

void CtsCmd::addOption(boost::program_options::options_description& desc) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES: {
            desc.add_options()(CtsApi::zombieGetArg(),
                               "Returns the list of zombies from the server.\n"
                               "Results reported to standard output.");
            break;
        }
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT: {
            desc.add_options()(CtsApi::restoreDefsFromCheckPtArg(),
                               "Ask the server to load the definition from an check pt file.\n"
                               "The server must be halted and the definition in the server must be deleted\n"
                               "first, otherwise an error is returned");
            break;
        }
        case CtsCmd::RESTART_SERVER: {
            desc.add_options()(CtsApi::restartServerArg(),
                               "Start job scheduling, communication with jobs, and respond to all requests.\n"
                               "The following table shows server behaviour in the different states.\n"
                               "|----------------------------------------------------------------------------------|\n"
                               "| Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |\n"
                               "|--------------|--------------|--------------|---------------|---------------------|\n"
                               "|     RUNNING  |    yes       |      yes     |      yes      |      yes            |\n"
                               "|    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |\n"
                               "|      HALTED  |    yes       |      no      |      no       |      no             |\n"
                               "|--------------|--------------|--------------|---------------|---------------------|");
            break;
        }
        case CtsCmd::SHUTDOWN_SERVER: {
            desc.add_options()(CtsApi::shutdownServerArg(),
                               po::value<std::string>()->implicit_value(std::string("")),
                               "Stop server from scheduling new jobs.\n"
                               "  arg1 = yes(optional) # use to bypass confirmation prompt,i.e\n"
                               "  --shutdown=yes\n"
                               "The following table shows server behaviour in the different states.\n"
                               "|----------------------------------------------------------------------------------|\n"
                               "| Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |\n"
                               "|--------------|--------------|--------------|---------------|---------------------|\n"
                               "|     RUNNING  |    yes       |      yes     |      yes      |      yes            |\n"
                               "|    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |\n"
                               "|      HALTED  |    yes       |      no      |      no       |      no             |\n"
                               "|--------------|--------------|--------------|---------------|---------------------|");
            break;
        }
        case CtsCmd::HALT_SERVER: {
            desc.add_options()(CtsApi::haltServerArg(),
                               po::value<std::string>()->implicit_value(std::string("")),
                               "Stop server communication with jobs, and new job scheduling.\n"
                               "Also stops automatic check pointing\n"
                               "  arg1 = yes(optional) # use to bypass confirmation prompt,i.e.\n"
                               "  --halt=yes\n"
                               "The following table shows server behaviour in the different states.\n"
                               "|----------------------------------------------------------------------------------|\n"
                               "| Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |\n"
                               "|--------------|--------------|--------------|---------------|---------------------|\n"
                               "|     RUNNING  |    yes       |      yes     |      yes      |      yes            |\n"
                               "|    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |\n"
                               "|      HALTED  |    yes       |      no      |      no       |      no             |\n"
                               "|--------------|--------------|--------------|---------------|---------------------|");
            break;
        }
        case CtsCmd::TERMINATE_SERVER: {
            desc.add_options()(CtsApi::terminateServerArg(),
                               po::value<std::string>()->implicit_value(std::string("")),
                               "Terminate the server.\n"
                               "  arg1 = yes(optional) # use to bypass confirmation prompt.i.e\n"
                               "  --terminate=yes");
            break;
        }
        case CtsCmd::RELOAD_WHITE_LIST_FILE: {
            desc.add_options()(
                CtsApi::reloadwsfileArg(),
                "Reload the white list file.\n"
                "\n"
                "The white list file (authorisation) is used to verify if a 'user' is allowed to perform a\n"
                "specific command.\n"
                "\n"
                "The file path is specified as the ECF_LISTS variable, and loaded only once by the server\n"
                "(on *startup*). This means that the file contents can be updated, but the file location\n"
                "cannot change during the server execution.\n"
                "\n"
                "The ECF_LISTS variable can be used as follows:\n"
                "  - if ECF_LISTS is not specified, or if it is specified with value `ecf.lists`,\n"
                "    then the server will use the value `<host>.<port>.ecf.lists`\n"
                "  - if ECF_LISTS is specified to be a path, such as /var/tmp/ecf.lists,\n"
                "    then the server will use this path to reload the white list file\n"
                "\n"
                "The server automatically loads the white list file content as part of the startup procedure,\n"
                "considering that if the file is not present or is empty (i.e., just contains the version\n"
                "number) then all users have read/write access.\n"
                "\n"
                "The reload operation will fail if file does not exist or if the content is invalid.\n"
                "\n"
                "Expected format for this file is:\n"
                "\n"
                "\n"
                "# all characters after the first # in a line are considered comments and are discarded\n"
                "# empty lines are also discarded\n"
                "\n"
                "4.4.14  # the version number is mandatory, even if no users are specified\n"
                "# Users with read/write access\n"
                "user1\n"
                "user2   # comment\n"
                "*       # use this form if you want all users to have read/write access\n"
                "\n"
                "# Users with read  access, must have - before user name\n"
                "-user3  # comment\n"
                "-user4\n"
                "-*      # use this form if you want all users to have read access\n"
                "\n"
                "\n"
                "Usage:\n"
                " --reloadwsfile");
            break;
        }
        case CtsCmd::RELOAD_PASSWD_FILE: {
            desc.add_options()(
                CtsApi::reloadpasswdfile_arg(),
                "Reload the server password file.\n"
                "\n"
                "The password file (authentication) is used by the server to authenticate a 'user' by\n"
                "verifying if the password provided by the user matches the one held by the server.\n"
                "The password file is also used on the client to automatically load the password for the\n"
                "'user' when connecting to the server.\n"
                "\n"
                "When the server is configured to use a password file, then ALL users must have a password.\n"
                "\n"
                "The file path is specified as the ECF_PASSWD environment variable, both for the client and\n"
                "server, and is loaded only by the server on *startup*. This means that the file contents\n"
                "can be updated (i.e., add/remove users), but the file location cannot change during the\n"
                "server execution.\n"
                "\n"
                "The server automatically loads the password file content as part of the startup procedure.\n"
                "\n"
                "The ECF_PASSWD environment variable is used to specify the password file location,\n"
                "considering that\n"
                " - On the server, the default file name is <host>.<port>.ecf.passwd\n"
                " - On the client, the default file name is ecf.passwd\n"
                "\n"
                "The format of the file is same for client and server:\n"
                "\n"
                "\n"
                "4.5.0\n"
                "# comment\n"
                "<user> <host> <port> <passwd> # comment\n"
                "\n"
                "The following is an example\n"
                "\n"
                "4.5.0 # the version\n"
                "fred machine1 3142 xxyyyd\n"
                "fred machine2 3133 xxyyyd # comment\n"
                "bill machine2 3133 xxyggyyd\n"
                "\n"
                "\n"
                "Notice that the same user may appear multiple times (associated with different host/port).\n"
                "This allows the client to use the same password file to contact multiple servers.\n"
                "\n"
                "For the password authentication to work, ensure the following:\n"
                " - The password is defined for the client and server\n"
                " - On the server, add at least the server administrator to the password file\n"
                "   Note: If an empty password file (i.e., containing just the version) is used,\n"
                "         no user is allowed access.\n"
                " - On the client, the password file should be readable only by the 'user' itself\n"
                "\n"
                "Usage:\n"
                " --reloadpasswdfile");
            break;
        }
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE: {
            desc.add_options()(
                CtsApi::reloadcustompasswdfile_arg(),
                "Reload the server custom password file.\n"
                "\n"
                "The custom password file (authentication) is used by the server to authenticate a 'user' by\n"
                "verifying if the password provided by the user matches the one held by the server. This\n"
                "particular file is used for authentication of users that explicitly specify the user name\n"
                "(either via the environment variable ECF_USER or the --user option).\n"
                "\n"
                "This mechanism should be used when most users use the machine login name, but a few users\n"
                "specify their own user name, in which case the password must also be explicitly provided.\n"
                "\n"
                "The file path is specified as the ECF_CUSTOM_PASSWD environment variable, both for the\n"
                "client and server, and is loaded only by the server on *startup*. This means that the file\n"
                "contents can be updated (i.e., add/remove users), but the file location cannot change during\n"
                "the server execution.\n"
                "\n"
                "The server automatically loads the password file content as part of the startup procedure.\n"
                "\n"
                "The ECF_CUSTOM_PASSWD environment variable is used to specify the password file location,\n"
                "considering that\n"
                " - On the server the default file name is <host>.<port>.ecf.custom_passwd\n"
                " - On the client the default file name is ecf.custom_passwd\n"
                "\n"
                "The format of the file is same for client and server:\n\n"
                "\n"
                "4.5.0\n"
                "# comment\n"
                "<user> <host> <port> <passwd> # comment\n"
                "\n"
                "The following is an example\n"
                "\n"
                "4.5.0 # the version\n"
                "fred machine1 3142 xxyyyd\n"
                "fred machine2 3133 xxyyyd # comment\n"
                "bill machine2 3133 xxyggyyd\n"
                "\n"
                "Notice that the same user may appear multiple times (associated with different host/port).\n"
                "This allows the client to use the same password file to contact multiple servers.\n"
                "\n"
                "For the password authentication to work, ensure the following:\n"
                " - The password is defined for the client and server\n"
                " - On the server, add at least the server administrator to the password file\n"
                "   Note: If an empty password file (i.e., containing just the version) is used,\n"
                "         no user is allowed access.\n"
                " - On the client, the password file should be readable only by the 'user' itself\n"
                "\n"
                "Usage:\n"
                " --reloadcustompasswdfile");
            break;
        }

        case CtsCmd::FORCE_DEP_EVAL: {
            desc.add_options()(CtsApi::forceDependencyEvalArg(), "Force dependency evaluation. Used for DEBUG only.");
            break;
        }
        case CtsCmd::PING: {
            desc.add_options()(
                CtsApi::pingServerArg(),
                "Check if server is running on given host/port. Result reported to standard output.\n"
                "Usage:\n"
                "  --ping --host=mach --port=3144  # Check if server alive on host mach & port 3144\n"
                "  --ping --host=fred              # Check if server alive on host fred and port ECF_PORT,\n"
                "                                  # otherwise default port of 3141\n"
                "  --ping                          # Check if server alive by using environment variables\n"
                "                                  # ECF_HOST and ECF_PORT\n"
                "If ECF_HOST not defined uses 'localhost', if ECF_PORT not defined assumes 3141");
            break;
        }
        case CtsCmd::STATS: {
            desc.add_options()(CtsApi::statsArg(), "Returns the server statistics as a string.");
            break;
        }
        case CtsCmd::STATS_SERVER: {
            desc.add_options()(CtsApi::stats_server_arg(),
                               "Returns the server statistics as a struct and string. For test use only.");
            break;
        }
        case CtsCmd::STATS_RESET: {
            desc.add_options()(CtsApi::stats_reset_arg(), "Resets the server statistics.");
            break;
        }
        case CtsCmd::SUITES: {
            desc.add_options()(CtsApi::suitesArg(), "Returns the list of suites, in the order defined in the server.");
            break;
        }
        case CtsCmd::DEBUG_SERVER_ON: {
            desc.add_options()(CtsApi::debug_server_on_arg(), "Enables debug output from the server");
            break;
        }
        case CtsCmd::DEBUG_SERVER_OFF: {
            desc.add_options()(CtsApi::debug_server_off_arg(), "Disables debug output from the server");
            break;
        }
        case CtsCmd::SERVER_LOAD: {
            desc.add_options()(CtsApi::server_load_arg(),
                               po::value<std::string>()->implicit_value(std::string("")),
                               server_load_desc());
            break;
        }
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
}

bool CtsCmd::handleRequestIsTestable() const {
    if (api_ == CtsCmd::TERMINATE_SERVER) {
        return false;
    }
    if (api_ == CtsCmd::RESTORE_DEFS_FROM_CHECKPT) {
        return false;
    }
    return true;
}

void CtsCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    if (ac->debug()) {
        std::cout << "  CtsCmd::create api = '" << api_ << "'.\n";
    }

    assert(api_ != CtsCmd::NO_CMD);

    if (api_ == CtsCmd::HALT_SERVER || api_ == CtsCmd::SHUTDOWN_SERVER || api_ == CtsCmd::TERMINATE_SERVER) {

        std::string do_prompt = vm[theArg()].as<std::string>();
        if (do_prompt.empty()) {
            if (api_ == CtsCmd::HALT_SERVER) {
                prompt_for_confirmation("Are you sure you want to halt the server ? ");
            }
            else if (api_ == CtsCmd::SHUTDOWN_SERVER) {
                prompt_for_confirmation("Are you sure you want to shut down the server ? ");
            }
            else {
                prompt_for_confirmation("Are you sure you want to terminate the server ? ");
            }
        }
        else if (do_prompt != "yes") {
            throw std::runtime_error(
                "Halt, shutdown and terminate expected 'yes' as the only argument to bypass the confirmation prompt");
        }
    }
    else if (api_ == CtsCmd::SERVER_LOAD) {

        std::string log_file = vm[theArg()].as<std::string>();
        if (ac->debug()) {
            std::cout << "  CtsCmd::create CtsCmd::SERVER_LOAD " << log_file << "\n";
        }

        if (!log_file.empty()) {

            // testing client interface
            if (ac->under_test()) {
                return;
            }

            // No need to call server. Parse the log file to create gnu_plot file.
            Gnuplot gnuplot(log_file, ac->host(), ac->port());
            gnuplot.show_server_load();

            return; // Don't create command, since with log file, it is client specific only
        }
    }
    cmd = std::make_shared<CtsCmd>(api_);
}

std::ostream& operator<<(std::ostream& os, const CtsCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(CtsCmd)
CEREAL_REGISTER_DYNAMIC_INIT(CtsCmd)
