/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
    TaskScriptGenerator(TaskScriptGenerator&&)                       = delete;
    TaskScriptGenerator& operator=(TaskScriptGenerator&&)            = delete;

    ~TaskScriptGenerator() = default;

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
