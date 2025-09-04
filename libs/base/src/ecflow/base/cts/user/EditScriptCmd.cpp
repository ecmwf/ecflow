/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/EditScriptCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/EcfFile.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/Task.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

bool EditScriptCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<EditScriptCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (path_to_node_ != the_rhs->path_to_node()) {
        return false;
    }
    if (edit_type_ != the_rhs->edit_type()) {
        return false;
    }
    if (alias_ != the_rhs->alias()) {
        return false;
    }
    if (run_ != the_rhs->run()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t EditScriptCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t EditScriptCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

static std::string to_string(EditScriptCmd::EditType et) {
    switch (et) {
        case EditScriptCmd::EDIT:
            return "edit";
            break;
        case EditScriptCmd::PREPROCESS:
            return "pre_process";
            break;
        case EditScriptCmd::SUBMIT:
            return "submit";
            break;
        case EditScriptCmd::PREPROCESS_USER_FILE:
            return "pre_process_file";
            break;
        case EditScriptCmd::SUBMIT_USER_FILE:
            return "submit_file";
            break;
        default:
            assert(false);
            break;
    }
    return "edit";
}
static std::vector<std::string> valid_edit_types() {
    std::vector<std::string> edit_types;
    edit_types.reserve(5);
    edit_types.emplace_back("edit");
    edit_types.emplace_back("pre_process");
    edit_types.emplace_back("submit");
    edit_types.emplace_back("pre_process_file");
    edit_types.emplace_back("submit_file");
    return edit_types;
}

void EditScriptCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::to_string(CtsApi::edit_script(path_to_node_, to_string(edit_type()), "", alias_, run_)));
}

void EditScriptCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::edit_script(path_to_node_, to_string(edit_type()), "", alias_, run_));
}

bool EditScriptCmd::isWrite() const {
    switch (edit_type_) {
        case EditScriptCmd::EDIT:
            return false;
            break;
        case EditScriptCmd::PREPROCESS:
            return false;
            break;
        case EditScriptCmd::PREPROCESS_USER_FILE:
            return false;
            break;
        case EditScriptCmd::SUBMIT:
            return true;
            break;
        case EditScriptCmd::SUBMIT_USER_FILE:
            return true;
            break;
        default:
            assert(false);
            break;
    }
    return false;
}

STC_Cmd_ptr EditScriptCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().edit_script_++;

    node_ptr node =
        find_node_for_edit(as->defs().get(), path_to_node_); // will throw if defs not defined, or node not found
    Submittable* submittable = node->isSubmittable();
    if (!submittable)
        throw std::runtime_error("EditScriptCmd failed. Cannot locate task or alias at path " + path_to_node_);

    /// record any changes made to suite. Needed for incremental updates
    SuiteChangedPtr changed(node.get());

    switch (edit_type_) {
        case EditScriptCmd::EDIT: {

            /// Find all the used variable in the script, i.e 'ecf file'
            EcfFile ecf_file = submittable->locatedEcfFile(); // will throw std::runtime_error for errors

            /// This will throw std::runtime_error  for parse errors
            /// Returns the script with the used variables added at the front of the file
            // **********************************************************************
            // Custom handling of dynamic variables, i.e ECF_TRYNO, ECF_PASS and
            // any variable that embeds a try number, i.e. ECF_JOB, ECF_JOBOUT
            // This is required since the try number is *always* incremented *before* job submission,
            // hence the value extracted from the job file will not be accurate, hence we exclude it.
            // This way at job submission we use the latest/correct value, which is in-sync with JOB OUTPUT
            // Note: Otherwise the job output will not be in sync
            //
            // Custom handling of ECF_PORT,ECF_HOST,ECF_NAME do not show these variables, these veriables
            // including ECF_PASS appear in the script. If the user accidentally edits them,
            // Child communication with the server will be broken. Hence not shown
            //
            // This special handling is done in: EcfFile::get_used_variables
            // **********************************************************************
            std::string ret_file_contents;
            ecf_file.edit_used_variables(ret_file_contents);
            return PreAllocatedReply::string_cmd(ret_file_contents);
        }

        case EditScriptCmd::PREPROCESS: {

            /// PRE_PROCESS the ecf file accessible by the server
            /// We only *ONLY* want %include pre-processed and *NOT* variable substitution
            /// The used variable will be listed at the top, the user can then edit these.
            /// Hence it is NOT until -> EditScriptCmd::SUBMIT_USER_FILE do we need to variable substitute
            EcfFile ecf_file = submittable->locatedEcfFile(); // will throw std::runtime_error for errors

            /// This function can throw std::runtime error if pre_processing fails
            /// This *WILL* also include the used variables
            std::string ret_file_contents;
            ecf_file.pre_process(ret_file_contents);
            return PreAllocatedReply::string_cmd(ret_file_contents);
        }

        case EditScriptCmd::PREPROCESS_USER_FILE: {

            /// The file contents('user_file_contents_') here could have been edited by the user on the client side.
            EcfFile ecf_file = submittable->locatedEcfFile(); // will throw std::runtime_error for errors

            /// This function can throw std::runtime error if pre_processing fails
            /// Does full pre-processing:
            ///   - include pre-processing
            ///   - variable substitution
            ///   - remove comment and manual sections
            ///   - ignore %noop
            std::string ret_file_contents;
            ecf_file.pre_process_user_file(user_file_contents_, ret_file_contents);
            vector<string>().swap(user_file_contents_); // clear user_file_contents_ and minimise its capacity
            return PreAllocatedReply::string_cmd(ret_file_contents);
        }

        case EditScriptCmd::SUBMIT: {

            if (submittable->state() == NState::ACTIVE || submittable->state() == NState::SUBMITTED) {
                std::stringstream ss;
                ss << "Node " << path_to_node_ << " is already " << NState::toString(submittable->state()) << " : ";
                throw std::runtime_error("EditScriptCmd:: failed for submit: " + ss.str());
            }

            /// Using the User edited variables, generate the job using the ECF/USR file accessible from the server
            /// Convert from vector to map
            NameValueMap user_variables_map;
            for (const auto& user_variable : user_variables_) {
                user_variables_map.insert(std::make_pair(user_variable.first, user_variable.second));
            }

            /// Do job submission, but creating .usr file first
            /// This will *NOT* timeout, unlike server Job generation
            JobsParam jobsParam(as->poll_interval(), true /* create jobs */); // spawn_jobs = true
            jobsParam.set_user_edit_variables(user_variables_map);

            // Custom handling of dynamic variables, i.e ECF_TRYNO, ECF_PASS and
            // any variable that embeds a try number, i.e. ECF_JOB, ECF_JOBOUT
            // This is required since the try number is *always* incremented *before* job submission,
            // hence the value extracted from the job file will *not* be accurate, hence we exclude it form user
            // variables
            if (!submittable->submitJob(jobsParam)) {
                throw std::runtime_error("EditScriptCmd:: failed for submit: " + jobsParam.getErrorMsg());
            }
            submittable->get_flag().set(ecf::Flag::USER_EDIT);
            break;
        }

        case EditScriptCmd::SUBMIT_USER_FILE: {
            /// The file contents('user_file_contents_') here could have been edited by the user on the client side.
            /// ECF_HOME
            /// If the selected node is an Alias, Just submit it. Since aliases can *NOT* have other aliases as children
            if (!alias_ || submittable->isAlias()) {
                if (submittable->state() == NState::ACTIVE || submittable->state() == NState::SUBMITTED) {
                    std::stringstream ss;
                    ss << "Node " << path_to_node_ << " is already " << NState::toString(submittable->state()) << " : ";
                    throw std::runtime_error("EditScriptCmd:: failed for submit: " + ss.str());
                }

                /// Convert from vector to map
                NameValueMap user_variables_map;
                for (const auto& user_variable : user_variables_) {
                    user_variables_map.insert(std::make_pair(user_variable.first, user_variable.second));
                }

                /// Do job submission, *USING* the user supplied file , will create .usr file
                /// This will *NOT* timeout, unlike server Job generation
                JobsParam jobsParam(as->poll_interval(), true /* create jobs */); //  spawn_jobs = true
                jobsParam.set_user_edit_variables(user_variables_map);
                jobsParam.set_user_edit_file(user_file_contents_);

                if (!submittable->submitJob(jobsParam)) {
                    vector<string>().swap(user_file_contents_); // clear user_file_contents_ and minimise its capacity
                    throw std::runtime_error("EditScriptCmd::SUBMIT_USER_FILE: failed : " + jobsParam.getErrorMsg());
                }
                submittable->get_flag().set(ecf::Flag::USER_EDIT);
            }
            else {
                // CREATE a Child Alias. The create alias and parent it to the Task., and choose to run or not.
                Task* task = submittable->isTask();
                if (!task) {
                    vector<string>().swap(user_file_contents_); // clear user_file_contents_ and minimise its capacity
                    throw std::runtime_error("EditScriptCmd::SUBMIT_USER_FILE: Aliases can only be created for a task. "
                                             "Selected path is a Alias. Please select a Task path");
                }
                alias_ptr alias = task->add_alias(user_file_contents_, user_variables_);

                if (run_) {
                    /// This will *NOT* timeout, unlike server Job generation
                    JobsParam jobsParam(as->poll_interval(), true /* create jobs */); // spawn jobs = true
                    if (!alias->submitJob(jobsParam)) {
                        vector<string>().swap(
                            user_file_contents_); // clear user_file_contents_ and minimise its capacity
                        throw std::runtime_error("EditScriptCmd::SUBMIT_USER_FILE: failed for Alias : " +
                                                 jobsParam.getErrorMsg());
                    }
                    alias->get_flag().set(ecf::Flag::USER_EDIT);
                }
            }
            break;
        }

        default:
            assert(false);
            break;
    }

    // Clear up memory allocated *ASAP*
    // When dealing with several thousands strings, this makes a *HUGE* difference
    vector<string>().swap(user_file_contents_); // clear user_file_contents_ and minimise its capacity

    return PreAllocatedReply::ok_cmd();
}

// bool EditScriptCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
//     return do_authenticate(as, cmd, path_to_node_);
// }

const char* EditScriptCmd::arg() {
    return CtsApi::edit_script_arg();
}
const char* EditScriptCmd::desc() {
    /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
    return "Allows user to edit, pre-process and submit the script.\n"
           "Will allow pre-processing of arbitrary file with 'pre_process_file' option\n"
           " arg1 = path to task  # The path to the task/alias\n"
           " arg2 = [ edit | pre_process | submit | pre_process_file | submit_file ]\n"
           "    edit : will return the script file to standard out. The script will\n"
           "           include used variables enclosed between %comment/%end at the\n"
           "           start of the file\n"
           "    pre_process: Will return the script file to standard out.The script will\n"
           "                 include used variables enclosed between %comment/%end at the\n"
           "                 start of the file and with all %include expanded\n"
           "    submit: Will extract the used variables from the supplied file, i.e\n"
           "            between the %comment/%end and use these them to generate the\n"
           "            job using the ecf file accessible from the server\n"
           "    pre_process_file: Will pre process the user supplied file.\n"
           "                      Will expand includes,variable substitution,\n"
           "                      remove manual & comment sections.\n"
           "    submit_file: Like submit, but the supplied file, is submitted by the server\n"
           "                 The last 2 options allow complete freedom to debug the script file\n"
           " arg3 = [ path_to_script_file ]\n"
           "          needed for option [  pre_process_file | submit_file ]\n"
           " arg4 = create_alias (optional) default value is false, for use with 'submit_file' option\n"
           " arg5 = no_run (optional) default value is false, i.e immediately run the alias\n"
           "        is no_run is specified the alias in only created\n"
           "Usage:\n"
           "--edit_script=/path/to/task edit > script_file\n"
           "   server returns script with the used variables to standard out\n"
           "   The user can choose to edit this file\n\n"
           "--edit_script=/path/to/task pre_process > pre_processed_script_file\n"
           "  server will pre process the ecf file accessible from the server\n"
           "  (i.e expand all %includes) and return the file to standard out\n\n"
           "--edit_script=/path/to/task submit script_file\n"
           "  Will extract the used variables in the 'script_file' and will uses these\n"
           "  variables during variable substitution of the ecf file accessible by the\n"
           "  server. This is then submitted as a job\n\n"
           "--edit_script=/path/to/task pre_process_file file_to_pre_process\n"
           "  The server will pre-process the user supplied file and return the contents\n"
           "  to standard out. This pre-processing is the same as job file processing,\n"
           "  but on a arbitrary file\n\n"
           "--edit_script=/path/to/task submit_file file_to_submit\n"
           "  Will extract the used variables in the 'file_to_submit' and will uses these\n"
           "  variables during variable substitution, the file is then submitted for job\n"
           "  generation by the server\n\n"
           "--edit_script=/path/to/task submit_file file_to_submit create_alias\n"
           "  Like the the previous example but will create and run as an alias";
}

void EditScriptCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(EditScriptCmd::arg(), po::value<vector<string>>()->multitoken(), EditScriptCmd::desc());
}

void EditScriptCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (ac->debug())
        dumpVecArgs(EditScriptCmd::arg(), args);

    std::stringstream ss;
    if (args.size() < 2) {
        ss << "EditScriptCmd:At least 2 arguments required:\n" << EditScriptCmd::desc();
        throw std::runtime_error(ss.str());
    }

    string path_to_task               = args[0];
    string edit_type_str              = args[1];
    EditScriptCmd::EditType edit_type = EditScriptCmd::EDIT;

    /// Check edit_type is valid
    bool ok                             = false;
    std::vector<std::string> edit_types = valid_edit_types();
    for (const auto& i : edit_types) {
        if (edit_type_str == i) {
            if (edit_type_str == "edit")
                edit_type = EditScriptCmd::EDIT;
            else if (edit_type_str == "pre_process")
                edit_type = EditScriptCmd::PREPROCESS;
            else if (edit_type_str == "submit")
                edit_type = EditScriptCmd::SUBMIT;
            else if (edit_type_str == "pre_process_file")
                edit_type = EditScriptCmd::PREPROCESS_USER_FILE;
            else if (edit_type_str == "submit_file")
                edit_type = EditScriptCmd::SUBMIT_USER_FILE;
            else {
                assert(false);
            }
            ok = true;
            break;
        }
    }

    if (!ok) {
        ss << "The second argument(" << args[1] << ") to edit_script must be one of [ ";
        for (size_t i = 0; i < edit_types.size(); ++i) {
            if (i != 0)
                ss << " | ";
            ss << edit_types[i];
        }
        ss << "]\n" << EditScriptCmd::desc();
        throw std::runtime_error(ss.str());
    }

    if (args.size() == 2) {
        if (edit_type == EditScriptCmd::EDIT || edit_type == EditScriptCmd::PREPROCESS) {
            cmd = std::make_shared<EditScriptCmd>(path_to_task, edit_type);
            return;
        }
        else {
            ss << "When two arguments specified, the second argument must be one of [ edit | pre_process ]\n"
               << EditScriptCmd::desc();
            throw ss.str();
        }
    }

    bool create_alias = false; // for use with "submit_file" option only
    bool run_alias    = true;  // for use with "submit_file" option only
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 2 && args[i] == "create_alias")
            create_alias = true;
        if (i > 2 && args[i] == "no_run")
            run_alias = false;
    }
    if ((create_alias || !run_alias) && edit_type != EditScriptCmd::SUBMIT_USER_FILE) {
        ss << "The create_alias option is only valid when the second argument is 'submit_file' \n"
           << EditScriptCmd::desc();
        throw ss.str();
    }

    if (args.size() >= 3 && args.size() <= 5) {
        string path_to_script = args[2];
        std::vector<std::string> script_lines;

        if (!fs::exists(path_to_script)) {
            ss << "The script file specified '" << path_to_script << "' does not exist\n";
            throw std::runtime_error(ss.str());
        }
        if (!File::splitFileIntoLines(path_to_script, script_lines)) {
            ss << "Could not open script file " << path_to_script << " (" << strerror(errno) << ")";
            throw std::runtime_error(ss.str());
        }

        if (edit_type == EditScriptCmd::SUBMIT || edit_type == EditScriptCmd::SUBMIT_USER_FILE) {
            // extract the Used variables from the script file
            NameValueMap used_variables_as_map;
            EcfFile::extract_used_variables(used_variables_as_map, script_lines);

            // Convert MAP to Vec
            NameValueVec used_variables_as_vec;
            for (const NameValueMap::value_type& pair : used_variables_as_map) {
                used_variables_as_vec.push_back(std::make_pair(pair.first, pair.second));
            }

            if (edit_type == EditScriptCmd::SUBMIT) {
                cmd = std::make_shared<EditScriptCmd>(path_to_task, used_variables_as_vec); // SUMBIT
                return;
            }

            cmd = std::make_shared<EditScriptCmd>(
                path_to_task, used_variables_as_vec, script_lines, create_alias, run_alias); // SUBMIT_USER_FILE
            return;
        }
        else if (edit_type == EditScriptCmd::PREPROCESS_USER_FILE) {

            cmd = std::make_shared<EditScriptCmd>(path_to_task, script_lines);
            return;
        }
    }

    ss << "Wrong number of arguments specified\n" << EditScriptCmd::desc();
    throw std::runtime_error(ss.str());
}

std::ostream& operator<<(std::ostream& os, const EditScriptCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(EditScriptCmd)
CEREAL_REGISTER_DYNAMIC_INIT(EditScriptCmd)
