/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_TaskScriptGenerator_HPP
#define ecflow_node_TaskScriptGenerator_HPP

#include <map>
#include <string>
class Task;

namespace ecf {

class TaskScriptGenerator {
public:
    explicit TaskScriptGenerator(const Task*);

    // Disable copy (and move) semantics
    TaskScriptGenerator(const TaskScriptGenerator&)                  = delete;
    const TaskScriptGenerator& operator=(const TaskScriptGenerator&) = delete;

    void generate(const std::map<std::string, std::string>& override);

private:
    void generate_head_file() const;
    void generate_tail_file() const;
    std::string getDefaultTemplateEcfFile() const;

private:
    const Task* task_;
    bool is_dummy_task_;
    std::string ecf_files_;
    std::string ecf_home_;
    std::string ecf_include_;
};

} // namespace ecf

#endif /* ecflow_node_TaskScriptGenerator_HPP */
