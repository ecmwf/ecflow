/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/TaskScriptGenerator.hpp"

#include <stdexcept>

#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Task.hpp"

using namespace std;
using namespace boost;

namespace ecf {

TaskScriptGenerator::TaskScriptGenerator(const Task* task) : task_(task), is_dummy_task_(false) {
    /// if ECF_DUMMY_TASK specified ignore
    std::string theValue;
    is_dummy_task_ = task_->findParentUserVariableValue(ecf::environment::ECF_DUMMY_TASK, theValue);
    if (is_dummy_task_)
        return;

    /// if ECF_FILES specified use this before ECF_HOME
    if (task_->findParentUserVariableValue(ecf::environment::ECF_FILES, ecf_files_)) {
        // Create any missing directories if ECF_FILES is specified
        try {
            fs::create_directories(ecf_files_);
        }
        catch (std::exception& e) {
            std::stringstream ss;
            ss << "TaskScriptGenerator: Could not create directories for ECF_FILES " << ecf_files_ << " " << e.what();
            throw std::runtime_error(ss.str());
        }
    }

    /// Find ECF_HOME and ECF_INCLUDE
    if (!task_->findParentUserVariableValue(ecf::environment::ECF_HOME, ecf_home_)) {
        std::stringstream ss;
        ss << "TaskScriptGenerator: Could not generate scripts for task " << task_->absNodePath()
           << " no ECF_HOME specified\n";
        throw std::runtime_error(ss.str());
    }
    if (!task_->findParentUserVariableValue(ecf::environment::ECF_INCLUDE, ecf_include_)) {
        std::stringstream ss;
        ss << "TaskScriptGenerator: Could not generate scripts for task " << task_->absNodePath()
           << " no ECF_INCLUDE specified\n";
        throw std::runtime_error(ss.str());
    }

    // Create any missing directories,
    try {
        fs::create_directories(ecf_home_);
    }
    catch (std::exception& e) {
        std::stringstream ss;
        ss << "TaskScriptGenerator: Could not create directories for ECF_HOME " << ecf_home_ << " " << e.what();
        throw std::runtime_error(ss.str());
    }

    try {
        fs::create_directories(ecf_include_);
    }
    catch (std::exception& e) {
        std::stringstream ss;
        ss << "TaskScriptGenerator: Could not create directories for ECF_INCLUDE " << ecf_include_ << " " << e.what();
        throw std::runtime_error(ss.str());
    }
}

void TaskScriptGenerator::generate(const std::map<std::string, std::string>& override) {
    // Ignore generation for dummy tasks
    if (is_dummy_task_)
        return;

    // If ECF_FILES was specified use that in preference to ECF_HOME for the ecf files.
    std::string root_directory_for_ecf_files;
    if (!ecf_files_.empty())
        root_directory_for_ecf_files = ecf_files_;
    else
        root_directory_for_ecf_files = ecf_home_;

    // Note: task_->absNodePath() starts with /.
    std::string ecf_file_path = root_directory_for_ecf_files + task_->absNodePath() + task_->script_extension();
    if (fs::exists(ecf_file_path)) {
        std::cout << "Cannot generate. Script file " << ecf_file_path << " already exists\n";
        return;
    }

    if (!File::createMissingDirectories(ecf_file_path)) {
        std::stringstream ss;
        ss << "TaskScriptGenerator::generate: Could not create missing directories '" << ecf_file_path << "' for task "
           << task_->absNodePath();
        throw std::runtime_error(ss.str());
    }

    // Create file head.h and tail.h in directory ECF_INCLUDE, check to see if they exist first
    // If the variable ECF_CLIENT_EXE_PATH is specified use it
    generate_head_file();
    generate_tail_file();

    // Create ECF file with default template or custom  file.
    // cout << "creating ecf file  " << ecf_file_path << "\n";
    std::string contents;
    auto it = override.find(task_->absNodePath());
    if (it == override.end()) {
        contents = getDefaultTemplateEcfFile();
    }
    else {
        contents = (*it).second;
    }

    std::string errorMsg;
    if (!File::create(ecf_file_path, contents, errorMsg)) {
        std::stringstream ss;
        ss << "TaskScriptGenerator::generate: Could not create '.ecf' script for task " << task_->absNodePath() << " "
           << errorMsg;
        throw std::runtime_error(ss.str());
    }
    std::cout << "Generated script file " << ecf_file_path << "\n";
}

static void add_queue(std::string& content, const std::string& client_exe, const std::string& sleep, const Node* node) {
    const std::vector<QueueAttr>& queues = node->queues();
    for (const QueueAttr& queue : queues) {
        content += "\n";
        content += "for i in";
        const std::vector<std::string>& queue_list = queue.list();
        for (const auto& i : queue_list) {
            content += " ";
            content += i;
        }
        content += "\n";
        content += "do\n";
        content += "   step=$(" + client_exe + "--queue=" + queue.name() + " active " + node->absNodePath() + " )\n";
        content += "   echo $step\n";
        content += "   " + sleep;
        content += "   " + client_exe + "--queue=" + queue.name() + " complete $step " + node->absNodePath() + "\n";
        content += "done\n";
    }
}

std::string TaskScriptGenerator::getDefaultTemplateEcfFile() const {
    std::string content;

    std::string sleep, var_sleep;
    if (task_->findParentUserVariableValue("SLEEP", var_sleep))
        sleep = "sleep %SLEEP%\n";
    else
        sleep = "sleep 1\n";

    std::string client_exe = "%ECF_CLIENT_EXE_PATH:";
    client_exe += Ecf::CLIENT_NAME();
    client_exe += "% ";

    content += "%include <head.h>\n";
    content += "%manual\n";
    content += "This is the default **generated** ecf script file\n";
    content += "If the task has events, meters or labels then the associated client\n";
    content += "to server commands are automatically generated.\n";
    content += "Will default to sleep for one second in between calls to the events, meters & labels,\n";
    content += "this can be overridden by adding a variable SLEEP\n";
    content += "%end\n";
    content += "\n";
    content += "%comment\n";
    content += "#============================================================\n";
    content += "# Using angle brackets means we look in directory ECF_INCLUDE\n";
    content += "# and then ECF_HOME\n";
    content += "#============================================================\n";
    content += "%end\n";
    content += "\n";
    content += "echo do some work\n";
    for (const Event& e : task_->events()) {
        if (e.initial_value())
            content += client_exe + "--event=" + e.name_or_number() + " clear\n";
        else
            content += client_exe + "--event=" + e.name_or_number() + "\n"; // same as set
        content += sleep;
    }

    content += "\n";
    for (const Meter& m : task_->meters()) {
        content += "for i in";
        for (int i = m.min(); i <= m.max(); i = i + 1) {
            content += " ";
            content += ecf::convert_to<std::string>(i);
        }
        content += "\n";
        content += "do\n";
        content += "   " + client_exe + "--meter=" + m.name() + " $i\n";
        content += "   " + sleep;
        content += "done\n";
    }
    content += "\n";

    /// labels require at least 2 arguments,
    for (const Label& label : task_->labels()) {

        if (!label.new_value().empty()) {
            content += client_exe + "--label=" + label.name() + " " + label.new_value() + "\n";
        }
        else if (!label.value().empty()) {
            content += client_exe + "--label=" + label.name() + " " + label.value() + "\n";
        }
        content += sleep;
    }

    /// Queues
    add_queue(content, client_exe, sleep, task_);
    Node* parent = task_->parent();
    while (parent) {
        add_queue(content, client_exe, sleep, parent);
        parent = parent->parent();
    }

    content += "\n";
    if (task_->events().empty() && task_->meters().empty() && task_->queues().empty()) {
        content += sleep;
    }
    content += "\necho end of job\n";
    content += "\n%include <tail.h>\n";
    return content;
}

void TaskScriptGenerator::generate_head_file() const {
    std::string path = ecf_include_ + "/head.h";
    if (fs::exists(path)) {
        std::cout << "Skipping generation of head file: " << path << " as it already exists\n";
        return;
    }

    std::string client_exe = "%ECF_CLIENT_EXE_PATH:";
    client_exe += Ecf::CLIENT_NAME();
    client_exe += "% ";

    std::string contents;
    contents += "#!/usr/bin/env bash\n";
    contents += "set -e          # stop the shell on first error X\n";
    contents += "set -u          # fail when using an undefined variable\n";
    contents += "set -o pipefail # fail if last(rightmost) command exits with a non-zero status\n";
    contents += "set -x          # echo script lines as they are executed\n";
    contents += "\n";
    contents += "# Defines the variables that are needed for any communication with ECF\n";
    contents += "export ECF_PORT=%ECF_PORT%    # The server port number\n";
    contents += "export ECF_HOST=%ECF_HOST%    # The name of ecf host that issued this task\n";
    contents += "export ECF_NAME=%ECF_NAME%    # The name of this current task\n";
    contents += "export ECF_PASS=%ECF_PASS%    # A unique password\n";
    contents += "export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task\n";
    contents += "export ECF_RID=$$\n";
    contents += "export ECF_TIMEOUT=300 # Only wait 5 minutes, if the server cannot be contacted (note default is 24 "
                "hours) before failing\n";
    contents += "if [[ \"%ECF_SSL:%\" != \"\" ]] ; then\n";
    contents += "   export ECF_SSL=%ECF_SSL:%\n";
    contents += "fi\n";
    contents += "#export ECF_DEBUG_CLIENT=1\n";
    contents += "\n";
    contents +=
        "# SANITY Check, typically only valid for new platforms. make sure hostname is resolvable to an IP address\n";
    contents += "os_name=$(uname -s)\n";
    contents += "if [[ $os_name = Linux ]] ; then\n";
    contents += "   ping -c 1 %ECF_HOST%\n";
    contents += "fi\n";
    contents += "\n";
    contents += "# Tell ecFlow we have started\n";
    contents += client_exe + "--init=$$\n";
    contents += "\n";
    contents += "# Defined a error handler\n";
    contents += "ERROR() {\n";
    contents += "   echo 'ERROR() called'\n";
    contents += "   set +e                      # Clear -e flag, so we don't fail\n";
    contents += "   wait                        # wait for background process to stop\n";
    contents +=
        "   " + client_exe + "--abort=trap   # Notify ecFlow that something went wrong, using 'trap' as the reason\n";
    contents += "   trap 0                      # Remove the trap\n";
    contents += "   exit 0                      # End the script\n";
    contents += "}\n";
    contents += "\n";
    contents += "# Trap any calls to exit and errors caught by the -e flag\n";
    contents += "trap ERROR 0\n";
    contents += "\n";
    contents += "# Trap any signal that may cause the script to fail\n";
    contents += "trap '{ echo \"Killed by a signal\"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15\n";

    std::string errorMsg;
    if (!File::create(path, contents, errorMsg)) {
        std::stringstream ss;
        ss << "TaskScriptGenerator::generate_tail_file: Could not create head.h " << path << " " << errorMsg;
        throw std::runtime_error(ss.str());
    }

    std::cout << "Generated header file: " << path << "\n";
}

void TaskScriptGenerator::generate_tail_file() const {
    std::string path = ecf_include_ + "/tail.h";
    if (fs::exists(path)) {
        std::cout << "Skipping generation of tail file: " << path << " as it already exists\n";
        return;
    }

    std::string contents = "%ECF_CLIENT_EXE_PATH:";
    contents += Ecf::CLIENT_NAME();
    contents += "% --complete    # Notify ecFlow of a normal end\n";

    contents += "trap 0                 # Remove all traps\n";
    contents += "exit 0                 # End the shell\n";

    std::string errorMsg;
    if (!File::create(path, contents, errorMsg)) {
        std::stringstream ss;
        ss << "TaskScriptGenerator::generate_tail_file: Could not create tail.h " << path << " " << errorMsg;
        throw std::runtime_error(ss.str());
    }

    std::cout << "Generated tail file: " << path << "\n";
}

} // namespace ecf
