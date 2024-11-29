/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/ServerEnvironment.hpp"

#include <iostream>

#include <boost/program_options.hpp>

#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Pid.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/JobProfiler.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/server/ServerOptions.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

static std::string the_check_mode(ecf::CheckPt::Mode mode) {
    switch (mode) {
        case ecf::CheckPt::NEVER:
            return "CHECK_NEVER";
            break;
        case ecf::CheckPt::ON_TIME:
            return "CHECK_ON_TIME";
            break;
        case ecf::CheckPt::ALWAYS:
            return "CHECK_ALWAYS";
            break;
        case ecf::CheckPt::UNDEFINED:
            return "UNDEFINED";
            break;
    }
    cout << "ServerEnvironment.cpp theCheckMode: assert failed\n";
    assert(false);
    return std::string();
}

// This can be overridden by calling "server --ecfinterval 3" for test purposes
const int defaultSubmitJobsInterval = 60;

////////////////////////////////////////////////////////////////////////////////////////////
// class ServerEnvironment:
///////////////////////////////////////////////////////////////////////////////////////////

ServerEnvironment::ServerEnvironment(const CommandLine& cl, const std::string& path_to_config_file)
    : serverHost_(host_name_.name()),
      serverPort_(0),
      checkPtInterval_(0),
      checkpt_save_time_alarm_(CheckPt::default_save_time_alarm()),
      submitJobsInterval_(defaultSubmitJobsInterval),
      ecf_prune_node_log_(0),
      jobGeneration_(true),
      debug_(false),
      help_option_(false),
      version_option_(false),
      checkMode_(ecf::CheckPt::ON_TIME),
      tcp_protocol_(boost::asio::ip::tcp::v4()) {
    Ecf::set_server(true);

    init(cl, path_to_config_file);
    if (debug_) {
        std::cout << dump() << "\n";
    }
}

ServerEnvironment::ServerEnvironment(int argc, char* argv[]) : ServerEnvironment(CommandLine(argc, argv)) {
}

ServerEnvironment::ServerEnvironment(int argc, char* argv[], const std::string& path_to_config_file)
    : ServerEnvironment(CommandLine(argc, argv), path_to_config_file) {
}

void ServerEnvironment::init(const CommandLine& cl, const std::string& path_to_config_file) {
    std::string log_file_name;
    try {
        read_config_file(log_file_name, path_to_config_file);
        read_environment_variables(log_file_name); // overrides any settings in the config file
    }
    catch (std::exception& e) {
        std::string msg           = "Exception in ServerEnvironment::ServerEnvironment() : ";
        std::string exception_msg = e.what();
        // On ecgate, wrong locale, causes an exception where msg is empty
        if (exception_msg.empty())
            msg += "Invalid locale? Check locale using 'locale -a', then export/set LANG environment";
        else
            msg += e.what();
        throw ServerEnvironmentException(msg);
    }

    // get server process id. This may be visualised in xecf. makes it easier to kill server
    try {
        ecf_pid_ = Pid::getpid();
    }
    catch (...) {
        throw ServerEnvironmentException("ServerEnvironment::ServerEnvironment:: Could not convert PID to a string\n");
    }
    // std::cout << "PID = " << ecf_pid_ << "\n";

    // The options(argc/argv) must be read after the environment, since they override everything else
    ServerOptions options(cl, this);
    help_option_ = options.help_option();
    if (help_option_)
        return; // User is printing the help
    version_option_ = options.version_option();
    if (version_option_)
        return; // User is printing the version

    /// Config, Environment, or Options may have updated port, update port dependent file names
    /// If we have default names make unique, by prefixing host and port
    assert(!ecf_checkpt_file_.empty());        // expect name of form "ecf.check"
    assert(!ecf_backup_checkpt_file_.empty()); // expect name of form "ecf.check.b"
    assert(!log_file_name.empty());            // expect name of form "ecf.log"
    assert(!ecf_white_list_file_.empty());     // expect name of form "ecf.lists"
    assert(!ecf_passwd_file_.empty());         // expect name of form "ecf.passwd"
    assert(!ecf_passwd_custom_file_.empty());  // expect name of form "ecf.custom_passwd"

    std::string port = ecf::convert_to<std::string>(serverPort_);

    // If path is absolute leave as is
    if (ecf_checkpt_file_ == Ecf::CHECKPT())
        ecf_checkpt_file_ = host_name_.prefix_host_and_port(port, ecf_checkpt_file_);
    if (ecf_checkpt_file_[0] != '/') {
        // Prepend with ECF_HOME
        std::string check_pt = ecf_home();
        check_pt += Str::PATH_SEPERATOR();
        check_pt += ecf_checkpt_file_;
        ecf_checkpt_file_ = check_pt;
    }

    // If path is absolute leave as is
    if (ecf_backup_checkpt_file_ == Ecf::BACKUP_CHECKPT())
        ecf_backup_checkpt_file_ = host_name_.prefix_host_and_port(port, ecf_backup_checkpt_file_);
    if (ecf_backup_checkpt_file_[0] != '/') {
        std::string check_pt = ecf_home();
        check_pt += Str::PATH_SEPERATOR();
        check_pt += ecf_backup_checkpt_file_;
        ecf_backup_checkpt_file_ = check_pt;
    }

    if (ecf_white_list_file_ == Str::WHITE_LIST_FILE())
        ecf_white_list_file_ = host_name_.prefix_host_and_port(port, ecf_white_list_file_);

    if (ecf_passwd_file_ == ecf::environment::ECF_PASSWD)
        ecf_passwd_file_ = host_name_.prefix_host_and_port(port, ecf_passwd_file_);

    if (ecf_passwd_custom_file_ == ecf::environment::ECF_CUSTOM_PASSWD)
        ecf_passwd_custom_file_ = host_name_.prefix_host_and_port(port, ecf_passwd_custom_file_);

    // Change directory to ECF_HOME and check thats its accessible
    change_dir_to_ecf_home_and_check_accesibility();

    // LOG FILE ================================================================================
    if (log_file_name == Ecf::LOG_FILE())
        log_file_name = host_name_.prefix_host_and_port(port, log_file_name);

    // Create the Log file. The log file is obtained from the environment. Hence **must** be done last.
    // From ecflow version 4.9.0 we no longer flush for each command. This can enabled/disabled
    Log::create(log_file_name);

    // Init log file:
    LOG(Log::MSG, ""); // previous log may not end in newline
    LOG(Log::MSG, "Started at " << to_simple_string(Calendar::second_clock_time()) << " universal time");
    if (tcp_protocol_.family() == 2 /*PF_INET*/)
        LOG(Log::MSG, "Host(" << hostPort().first << ")  Port(" << hostPort().second << ") using TCP/IP v4");
    else
        LOG(Log::MSG, "Host(" << hostPort().first << ")  Port(" << hostPort().second << ") using TCP/IP v6");
    LOG(Log::MSG, "ECF_HOME " << ecf_home());
    LOG(Log::MSG, "Job scheduling interval: " << submitJobsInterval_);
}

ServerEnvironment::~ServerEnvironment() {
    /// Destroy singleton to avoid valgrind from complaining
    Log::destroy();
    System::destroy();
}

bool ServerEnvironment::valid(std::string& errorMsg) const {
    /// This must be called *AFTER* the constructor
    //   cout << "*****************************************************************************\n";
    //   cout << "ServerEnvironment::valid\n";
    //   cout << "*****************************************************************************\n";

    if (serverHost_.empty()) {
        errorMsg = "Could not determine the server host.";
        return false;
    }
    std::stringstream ss;
    if (serverPort_ == 0 || serverPort_ <= 1023 || serverPort_ >= 49151) {

        ss << "Server port " << serverPort_ << " not set correctly. \n";
        ss << "The port numbers are divided into three ranges.\n";
        ss << "   o the Well Known Ports, (require root permission)      0 -1023\n";
        ss << "   o the Registered Ports,                             1024 -49151\n";
        ss << "   o Dynamic and/or Private Ports.                    49151 -65535\n\n";
        ss << "Please set in the range 1024-49151 via argument or \n";
        ss << "Server/src/environment.cfg.h or set the environment variable ECF_PORT. \n";
        errorMsg = ss.str();
        return false;
    }
    if (ecfHome_.empty()) {
        ss << "No ECF_HOME specified. Please set in Server/server_environment.cfg or\n";
        ss << "set the environment variable ECF_HOME\n";
        errorMsg = ss.str();
        return false;
    }
    if (!Log::instance() || Log::instance()->path().empty()) {
        ss << "No log file name specified. Please set in Server/server_environment.cfg or\n";
        ss << "set the environment variable ECF_LOG\n";
        errorMsg = ss.str();
        return false;
    }
    if (checkPtInterval_ == 0) {
        ss << "Checkpoint interval not set. Please set in Server/server_environment.cfg or\n";
        ss << "set the environment variable ECF_CHECKINTERVAL. The value must be\n";
        ss << "convertible to a integer\n";
        errorMsg = ss.str();
        return false;
    }
    if (submitJobsInterval_ < 1 || submitJobsInterval_ > 60) {
        ss << "Submit jobs interval not set correctly. Please set in Server/server_environment.cfg\n";
        ss << "or provide as an argument. It must be in the range [1-60] \n";
        errorMsg = ss.str();
        return false;
    }
    if (ecf_prune_node_log_ < 0) {
        ss << "ECF_PRUNE_NODE_LOG not set correctly. Please set in Server/server_environment.cfg\n";
        ss << "or via environment variable of same name. It must be in the range [1-356] \n";
        errorMsg = ss.str();
        return false;
    }
    if (ecf_checkpt_file_.empty()) {
        ss << "No checkpoint file name specified. Please set in Server/server_environment.cfg or\n";
        ss << "set the environment variable ECF_CHECK\n";
        errorMsg = ss.str();
        return false;
    }
    if (ecf_backup_checkpt_file_.empty()) {
        ss << "No old checkpoint file name specified. Please set in Server/server_environment.cfg or\n";
        ss << "set the environment variable ECF_CHECKOLD\n";
        errorMsg = ss.str();
        return false;
    }
    if (checkMode_ == ecf::CheckPt::UNDEFINED) {
        ss << "No ECF_CHECKMODE specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (ecf_cmd_.empty()) {
        ss << "No ECF_JOB_CMD specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (killCmd_.empty()) {
        ss << "No ECF_KILL_CMD specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (statusCmd_.empty()) {
        ss << "No ECF_STATUS_CMD specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (checkCmd_.empty()) {
        ss << "No ECF_CHECK_CMD specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (urlCmd_.empty()) {
        ss << "No ECF_URL_CMD specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (urlBase_.empty()) {
        ss << "No ECF_URL_BASE specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (url_.empty()) {
        ss << "No ECF_URL specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }
    if (ecf_micro_.empty()) {
        ss << "No ECF_MICRODEF specified. Please set in Server/server_environment.cfg\n";
        errorMsg = ss.str();
        return false;
    }

    if (!ecf_passwd_file_.empty() && fs::exists(ecf_passwd_file_)) {
        if (!passwd_file_.load(ecf_passwd_file_, debug(), errorMsg)) {
            std::cout << "Error: could not parse ECF_PASSWD file " << ecf_passwd_file_ << "\n" << errorMsg << "\n";
            return false;
        }
        if (!passwd_file_.check_at_least_one_user_with_host_and_port(serverHost_, the_port())) {
            std::cout << "Error: password file " << ecf_passwd_file_;
            std::cout << " does not contain any users, which match the host and port of this server\n";
            return false;
        }
    }

    if (!ecf_passwd_custom_file_.empty() && fs::exists(ecf_passwd_custom_file_)) {
        if (!passwd_custom_file_.load(ecf_passwd_custom_file_, debug(), errorMsg)) {
            std::cout << "Error: could not parse ECF_CUSTOM_PASSWD file " << ecf_passwd_custom_file_ << "\n"
                      << errorMsg << "\n";
            return false;
        }
        if (!passwd_custom_file_.check_at_least_one_user_with_host_and_port(serverHost_, the_port())) {
            std::cout << "Error: custom password file " << ecf_passwd_custom_file_;
            std::cout << " does not contain any users, which match the host and port of this server\n";
            return false;
        }
    }

    // If the white list file is empty or does not exist, *ON* server start, its perfectly valid
    // i.e any user is free to access the server
    if (ecf_white_list_file_.empty())
        return true;
    if (!fs::exists(ecf_white_list_file_))
        return true;

    /// read in the ecf white list file that specifies valid users and their access rights
    /// If the file can't be opened returns false and an error message and false;
    /// Automatically add server admin(user) with write access, as this will allow admin reload
    return load_whitelist_file(errorMsg);
}

std::pair<std::string, std::string> ServerEnvironment::hostPort() const {
    return std::make_pair(serverHost_, the_port());
}

std::string ServerEnvironment::the_port() const {
    return ecf::convert_to<std::string>(serverPort_);
}

void ServerEnvironment::variables(std::vector<std::pair<std::string, std::string>>& theRetVec) const {
    // Variables read in from the environment
    // Need to setup client environment.
    // The server sets these variable for use by the client. i.e when creating the jobs
    // The clients then uses them to communicate back with the server.
    theRetVec.emplace_back(ecf::environment::ECF_PORT, the_port());
    theRetVec.emplace_back(ecf::environment::ECF_HOST, serverHost_);

    theRetVec.emplace_back(ecf::environment::ECF_HOME, ecfHome_);
    theRetVec.emplace_back(std::string("ECF_LOG"), Log::instance()->path());
    theRetVec.emplace_back(std::string("ECF_CHECK"), ecf_checkpt_file_);
    theRetVec.emplace_back(std::string("ECF_CHECKOLD"), ecf_backup_checkpt_file_);
    theRetVec.emplace_back(std::string("ECF_INTERVAL"), ecf::convert_to<std::string>(submitJobsInterval_));

    // These variable are read in from the environment, but are not exposed
    // since they only affect the server
    // ECF_CHECKINTERVAL

    theRetVec.emplace_back(std::string("ECF_LISTS"),
                           ecf_white_list_file_); // read only variable, changing it has no effect
    theRetVec.emplace_back(std::string("ECF_PASSWD"),
                           ecf_passwd_file_); // read only variable, changing it has no effect
    theRetVec.emplace_back(std::string("ECF_CUSTOM_PASSWD"),
                           ecf_passwd_custom_file_); // read only variable, changing it has no effect

    // variables that can be overridden, in the suite definition
    theRetVec.emplace_back(std::string("ECF_JOB_CMD"), ecf_cmd_);
    theRetVec.emplace_back(std::string("ECF_KILL_CMD"), killCmd_);
    theRetVec.emplace_back(std::string("ECF_STATUS_CMD"), statusCmd_);
    theRetVec.emplace_back(std::string("ECF_CHECK_CMD"), checkCmd_);
    theRetVec.emplace_back(std::string("ECF_URL_CMD"), urlCmd_);
    theRetVec.emplace_back(std::string("ECF_URL_BASE"), urlBase_);
    theRetVec.emplace_back(std::string("ECF_URL"), url_);
    theRetVec.emplace_back(std::string("ECF_MICRO"), ecf_micro_);

    // Reference variable, these should be read only
    theRetVec.emplace_back(std::string("ECF_PID"), ecf_pid_);           // server PID
    theRetVec.emplace_back(std::string("ECF_VERSION"), Version::raw()); // server version

#ifdef ECF_OPENSSL
    if (ssl_.enabled()) {
        theRetVec.emplace_back(
            std::string("ECF_SSL"),
            ssl_.ssl()); // 1 means shared ssl certificate, <host>.<port> means server specific certificates
    }
#endif
}

bool ServerEnvironment::reloadWhiteListFile(std::string& errorMsg) {
    if (debug())
        cout << "ServerEnvironment::reloadWhiteListFile:(" << ecf_white_list_file_ << ") CWD("
             << fs::current_path().string() << ")\n";
    if (ecf_white_list_file_.empty()) {
        errorMsg += "The ECF_LISTS file ";
        errorMsg += ecf_white_list_file_;
        errorMsg += " has not been specified.";
        return false;
    }
    if (!fs::exists(ecf_white_list_file_)) {
        errorMsg += "The ECF_LISTS file ";
        errorMsg += ecf_white_list_file_;
        errorMsg += " does not exist. Server CWD : " + fs::current_path().string();
        return false;
    }

    return load_whitelist_file(errorMsg);
}

bool ServerEnvironment::load_whitelist_file(std::string& errorMsg) const {
    // Only override valid users if we successfully opened and parsed file
    if (white_list_file_.load(ecf_white_list_file_, debug(), errorMsg)) {
        // If user accidentally remove the server/user from white list,
        // they will not be able reload white list, since it requires write access.
        // (Requires terminate, modify white list, restart to fix)
        // Hence always allow server user write access *IF* required for non empty file
        white_list_file_.allow_write_access_for_server_user();
        return true;
    }
    return false;
}

bool ServerEnvironment::reloadPasswdFile(std::string& errorMsg) {
    if (debug())
        cout << "ServerEnvironment::reloadPasswdFile:(" << ecf_passwd_file_ << ") CWD(" << fs::current_path().string()
             << ")\n";
    if (ecf_passwd_file_.empty()) {
        errorMsg += "The ECF_PASSWD file ";
        errorMsg += ecf_passwd_file_;
        errorMsg += " has not been specified.";
        return false;
    }
    if (!fs::exists(ecf_passwd_file_)) {
        errorMsg += "The ECF_PASSWD file ";
        errorMsg += ecf_passwd_file_;
        errorMsg += " does not exist. Server CWD : " + fs::current_path().string();
        return false;
    }

    // Only override valid users if we successfully opened and parsed file
    return passwd_file_.load(ecf_passwd_file_, debug(), errorMsg);
}

bool ServerEnvironment::reloadCustomPasswdFile(std::string& errorMsg) {
    if (debug())
        cout << "ServerEnvironment::reloadPasswdFile:(" << ecf_passwd_custom_file_ << ") CWD("
             << fs::current_path().string() << ")\n";
    if (ecf_passwd_custom_file_.empty()) {
        errorMsg += "The ECF_CUSTOM_PASSWD file ";
        errorMsg += ecf_passwd_custom_file_;
        errorMsg += " has not been specified.";
        return false;
    }
    if (!fs::exists(ecf_passwd_custom_file_)) {
        errorMsg += "The ECF_CUSTOM_PASSWD file ";
        errorMsg += ecf_passwd_custom_file_;
        errorMsg += " does not exist. Server CWD : " + fs::current_path().string();
        return false;
    }

    // Only override valid users if we successfully opened and parsed file
    return passwd_custom_file_.load(ecf_passwd_custom_file_, debug(), errorMsg);
}

bool ServerEnvironment::authenticateReadAccess(const std::string& user,
                                               bool custom_user,
                                               const std::string& passwd) const {
    if (!custom_user) {
        if (!passwd_file_.authenticate(user, passwd))
            return false;
    }
    else {
        if (!passwd_custom_file_.authenticate(user, passwd))
            return false;
    }

    // if *NO* users specified then all users are valid
    return white_list_file_.verify_read_access(user);
}

bool ServerEnvironment::authenticateReadAccess(const std::string& user,
                                               bool custom_user,
                                               const std::string& passwd,
                                               const std::string& path) const {
    if (!custom_user) {
        if (!passwd_file_.authenticate(user, passwd))
            return false;
    }
    else {
        if (!passwd_custom_file_.authenticate(user, passwd))
            return false;
    }

    return white_list_file_.verify_read_access(user, path);
}

bool ServerEnvironment::authenticateReadAccess(const std::string& user,
                                               bool custom_user,
                                               const std::string& passwd,
                                               const std::vector<std::string>& paths) const {
    if (!custom_user) {
        if (!passwd_file_.authenticate(user, passwd))
            return false;
    }
    else {
        if (!passwd_custom_file_.authenticate(user, passwd))
            return false;
    }

    return white_list_file_.verify_read_access(user, paths);
}

bool ServerEnvironment::authenticateWriteAccess(const std::string& user) const {
    // if *NO* users specified then all users have write access
    return white_list_file_.verify_write_access(user);
}
bool ServerEnvironment::authenticateWriteAccess(const std::string& user, const std::string& path) const {
    return white_list_file_.verify_write_access(user, path);
}
bool ServerEnvironment::authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths) const {
    return white_list_file_.verify_write_access(user, paths);
}

// ============================================================================================
// Privates:
// ============================================================================================

void ServerEnvironment::read_config_file(std::string& log_file_name, const std::string& path_to_config_file) {
    if (debug())
        cout << "ServerEnvironment::read_config_file() current_path = " << fs::current_path() << "\n";

    try {
        std::string theCheckMode;
        int the_task_threshold = 0;

        // read the environment from the config file.
        // **** Port *must* be read before log file, and check pt files
        po::options_description config_file_options("Configuration");
        config_file_options.add_options()(
            "ECF_HOME", po::value<std::string>(&ecfHome_)->default_value("."), "ECF_HOME, the home for all ECF files")(
            "ECF_PORT",
            po::value<int>(&serverPort_)->default_value(3141),
            "The port number. Clients must use same port.")(
            "ECF_CHECK",
            po::value<std::string>(&ecf_checkpt_file_)->default_value(Ecf::CHECKPT()),
            "Check point file name")(
            "ECF_CHECKOLD",
            po::value<std::string>(&ecf_backup_checkpt_file_)->default_value(Ecf::BACKUP_CHECKPT()),
            "Backup checkpoint file name")(
            "ECF_LOG", po::value<std::string>(&log_file_name)->default_value(Ecf::LOG_FILE()), "Log file name")(
            "ECF_CHECKINTERVAL",
            po::value<int>(&checkPtInterval_)->default_value(CheckPt::default_interval()),
            "The interval in seconds to save check point file")(
            "ECF_INTERVAL",
            po::value<int>(&submitJobsInterval_)->default_value(defaultSubmitJobsInterval),
            "Check time dependencies and submit any jobs")(
            "ECF_CHECKMODE",
            po::value<std::string>(&theCheckMode),
            "The check mode, must be one of CHECK_NEVER, CHECK_ON_TIME, CHECK_ALWAYS")(
            "ECF_JOB_CMD",
            po::value<std::string>(&ecf_cmd_)->default_value(Ecf::JOB_CMD()),
            "Command to be executed to submit a job.")(
            "ECF_KILL_CMD",
            po::value<std::string>(&killCmd_)->default_value(Ecf::KILL_CMD()),
            "Command to be executed to kill a job.")(
            "ECF_STATUS_CMD",
            po::value<std::string>(&statusCmd_)->default_value(Ecf::STATUS_CMD()),
            "Command to be obtain the job status from server.")(
            "ECF_CHECK_CMD",
            po::value<std::string>(&checkCmd_)->default_value(Ecf::CHECK_CMD()),
            "Command to be obtain the job status from client.")(
            "ECF_URL_CMD",
            po::value<std::string>(&urlCmd_)->default_value(Ecf::URL_CMD()),
            "Command to be obtain url.")(
            "ECF_URL_BASE", po::value<std::string>(&urlBase_)->default_value(Ecf::URL_BASE()), "Defines url base.")(
            "ECF_URL", po::value<std::string>(&url_)->default_value(Ecf::URL()), "The default url.")(
            "ECF_MICRODEF",
            po::value<std::string>(&ecf_micro_)->default_value(Ecf::MICRO()),
            "Preprocessor character for variable substitution and including files")(
            "ECF_LISTS",
            po::value<std::string>(&ecf_white_list_file_)->default_value(Str::WHITE_LIST_FILE()),
            "Path name to file the list valid users and their access rights")(
            "ECF_PASSWD",
            po::value<std::string>(&ecf_passwd_file_)->default_value(ecf::environment::ECF_PASSWD),
            "Path name to passwd file")(
            "ECF_CUSTOM_PASSWD",
            po::value<std::string>(&ecf_passwd_custom_file_)->default_value(ecf::environment::ECF_CUSTOM_PASSWD),
            "Path name to custom passwd file, for user who don't use login name")(
            "ECF_TASK_THRESHOLD",
            po::value<int>(&the_task_threshold)->default_value(JobProfiler::task_threshold_default()),
            "The defaults thresholds when profiling job generation")(
            "ECF_PRUNE_NODE_LOG",
            po::value<int>(&ecf_prune_node_log_)->default_value(30),
            "Node log, older than 180 days automatically pruned when checkpoint file loaded");

        ifstream ifs(path_to_config_file.c_str());
        if (!ifs) {
            if (debug())
                cout << "Could not load server_environment.cfg " << path_to_config_file << "\n";
        }

        /// This is *NOT* redundant, when the file is empty/not present, then the default value
        /// defined above are set.
        po::variables_map vm;
        po::store(parse_config_file(ifs, config_file_options), vm);
        po::notify(vm);

        if (theCheckMode == "CHECK_ON_TIME")
            checkMode_ = ecf::CheckPt::ON_TIME;
        else if (theCheckMode == "CHECK_NEVER")
            checkMode_ = ecf::CheckPt::NEVER;
        else if (theCheckMode == "CHECK_ALWAYS")
            checkMode_ = ecf::CheckPt::ALWAYS;

        if (the_task_threshold != 0) {
            JobProfiler::set_task_threshold(the_task_threshold);
        }
    }
    catch (std::exception& e) {
        cerr << "ServerEnvironment::read_config_file() " << e.what() << "\n";
    }
}

void ServerEnvironment::read_environment_variables(std::string& log_file_name) {
    if (debug())
        cout << "ServerEnvironment::read_environment_variables()\n";

    if (auto var = ecf::environment::fetch<std::string>(ecf::environment::ECF_PORT); var) {
        std::string port = var.value();
        try {
            serverPort_ = ecf::convert_to<int>(port);
        }
        catch (const ecf::bad_conversion&) {
            std::stringstream ss;
            ss << "ServerEnvironment::read_environment_variables(): ECF_PORT is defined(" << port
               << ") but value is *not* convertible to an integer\n";
            throw ServerEnvironmentException(ss.str());
        }
    }

    if (auto var = ecf::environment::fetch<std::string>("ECF_CHECKINTERVAL"); var) {
        std::string interval = var.value();
        try {
            checkPtInterval_ = ecf::convert_to<int>(interval);
        }
        catch (const ecf::bad_conversion&) {
            std::stringstream ss;
            ss << "ServerEnvironment::read_environment_variables(): ECF_CHECKINTERVAL is defined(" << interval
               << ") but value is *not* convertible to an integer\n";
            throw ServerEnvironmentException(ss.str());
        }
    }

    ecf::environment::get(ecf::environment::ECF_HOME, ecfHome_);
    if (ecfHome_ == ".") { // expand to absolute paths
        ecfHome_ = fs::current_path().string();
    }

    ecf::environment::get(ecf::environment::ECF_LOG, log_file_name);

    ecf::environment::get("ECF_CHECK", ecf_checkpt_file_);

    ecf::environment::get("ECF_CHECKOLD", ecf_backup_checkpt_file_);

    ecf::environment::get("ECF_LISTS", ecf_white_list_file_);

    ecf::environment::get(ecf::environment::ECF_PASSWD, ecf_passwd_file_);

    ecf::environment::get(ecf::environment::ECF_CUSTOM_PASSWD, ecf_passwd_custom_file_);

    if (auto var = ecf::environment::fetch("ECF_PRUNE_NODE_LOG"); var) {
        try {
            ecf_prune_node_log_ = ecf::convert_to<int>(var.value());
        }
        catch (const ecf::bad_conversion&) {
            std::stringstream ss;
            ss << "ServerEnviroment::read_environment_variables: ECF_PRUNE_NODE_LOG must be convertible to an integer, "
                  "But found: "
               << var.value();
            throw ServerEnvironmentException(ss.str());
        }
    }

    ecf::environment::get("ECF_DEBUG_SERVER", debug_);

#ifdef ECF_OPENSSL
    // IF ECF_SSL= 1 search server.crt
    // ELSE          search <host>.<port>.crt
    ssl_.enable_if_defined(serverHost_, the_port());
#endif

    if (auto var = ecf::environment::fetch("ECF_TASK_THRESHOLD"); var) {
        std::string task_threshold = var.value();
        try {
            JobProfiler::set_task_threshold(ecf::convert_to<int>(task_threshold));
        }
        catch (...) {
            std::stringstream ss;
            ss << "ServerEnvironment::read_environment_variables(): ECF_TASK_THRESHOLD is defined(" << var.value()
               << ") but value is *not* convertible to an integer\n";
            throw ServerEnvironmentException(ss.str());
        }
    }
}

void ServerEnvironment::change_dir_to_ecf_home_and_check_accesibility() {
    if (chdir(ecfHome_.c_str()) != 0) {
        std::stringstream ss;
        ss << "Can't chdir to ECF_HOME " << ecfHome_ << "\n";
        throw ServerEnvironmentException(ss.str());
    }
    if (access(ecfHome_.c_str(), X_OK | R_OK | W_OK) != 0) {
        // R_OK test for read permission
        // W_OK test for write permission
        // X_OK test for execute or search permission
        // F_OK  test whether the directories leading to the file can be searched and the file exists.
        std::stringstream ss;
        ss << "Access restriction on ECF_HOME " << ecfHome_ << "\n";
        throw ServerEnvironmentException(ss.str());
    }
}

std::string ServerEnvironment::check_mode_str() const {
    return the_check_mode(checkMode_);
}

std::string ServerEnvironment::dump() const {
    std::stringstream ss;
    ss << "ECF_HOME = '" << ecfHome_ << "'\n";
    ss << "ECF_LOG = '" << Log::instance()->path() << "'\n";
    ss << "ECF_PORT = '" << serverPort_ << "'\n";
    ss << "ECF_CHECK = '" << ecf_checkpt_file_ << "'\n";
    ss << "ECF_CHECKOLD = '" << ecf_backup_checkpt_file_ << "'\n";
    ss << "ECF_CHECKINTERVAL = '" << checkPtInterval_ << "'\n";
    ss << "ECF_INTERVAL = '" << submitJobsInterval_ << "'\n";
    ss << "ECF_CHECKMODE = '" << the_check_mode(checkMode_) << "'\n";
    ss << "ECF_JOB_CMD = '" << ecf_cmd_ << "'\n";
    ss << "ECF_KILL_CMD = '" << killCmd_ << "'\n";
    ss << "ECF_STATUS_CMD = '" << statusCmd_ << "'\n";
    ss << "ECF_CHECK_CMD = '" << checkCmd_ << "'\n";
    ss << "ECF_URL_CMD = '" << urlCmd_ << "'\n";
    ss << "ECF_URL_BASE = '" << urlBase_ << "'\n";
    ss << "ECF_URL = '" << url_ << "'\n";
    ss << "ECF_MICRO = '" << ecf_micro_ << "'\n";
    ss << "ECF_PRUNE_NODE_LOG = '" << ecf_prune_node_log_ << "'\n";
    ss << "check pt save time alarm " << checkpt_save_time_alarm_ << "\n";
    ss << "Job generation " << jobGeneration_ << "\n";
    ss << "Server host name " << serverHost_ << "\n";
    ss << "ECF_PASSWD = " << ecf_passwd_file_ << "\n";
    ss << "ECF_CUSTOM_PASSWD = " << ecf_passwd_custom_file_ << "\n";
    if (tcp_protocol_.family() == 2 /*PF_INET*/)
        ss << "TCP Protocol  v4 \n";
    else if (tcp_protocol_.family() == 10 /*PF_INET6*/)
        ss << "TCP Protocol  v6 \n";

#ifdef ECF_OPENSSL
    ss << "ECF_SSL = " << ssl_ << "\n";
#endif

    ss << white_list_file_.dump_valid_users();
    return ss.str();
}

std::vector<std::string> ServerEnvironment::expected_variables() {
    std::vector<std::string> expected_variables;
    expected_variables.push_back(ecf::environment::ECF_HOME);
    expected_variables.emplace_back("ECF_LOG");
    expected_variables.emplace_back("ECF_CHECK");
    expected_variables.emplace_back("ECF_CHECKOLD");
    expected_variables.emplace_back("ECF_JOB_CMD");
    expected_variables.emplace_back("ECF_KILL_CMD");
    expected_variables.emplace_back("ECF_STATUS_CMD");
    expected_variables.emplace_back("ECF_CHECK_CMD");
    expected_variables.emplace_back("ECF_URL_CMD");
    expected_variables.emplace_back("ECF_URL_BASE");
    expected_variables.emplace_back("ECF_URL");
    expected_variables.emplace_back("ECF_MICRO");
    expected_variables.emplace_back("ECF_PID");
    expected_variables.emplace_back("ECF_VERSION");
    expected_variables.emplace_back("ECF_LISTS");
    expected_variables.push_back(ecf::environment::ECF_PORT);
    expected_variables.push_back(ecf::environment::ECF_HOST);
    expected_variables.emplace_back("ECF_INTERVAL");
    expected_variables.emplace_back("ECF_PASSWD");
    expected_variables.emplace_back("ECF_CUSTOM_PASSWD");
#ifdef ECF_OPENSSL
    if (ecf::environment::has("ECF_SSL")) {
        expected_variables.emplace_back("ECF_SSL");
    }
#endif
    return expected_variables;
}
