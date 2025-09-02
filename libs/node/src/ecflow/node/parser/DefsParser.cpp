/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/DefsParser.hpp"

#include <stdexcept>

#include "ecflow/core/Str.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/AutoArchiveParser.hpp"
#include "ecflow/node/parser/AutoCancelParser.hpp"
#include "ecflow/node/parser/AutoRestoreParser.hpp"
#include "ecflow/node/parser/AvisoParser.hpp"
#include "ecflow/node/parser/CalendarParser.hpp"
#include "ecflow/node/parser/ClockParser.hpp"
#include "ecflow/node/parser/CronParser.hpp"
#include "ecflow/node/parser/DateParser.hpp"
#include "ecflow/node/parser/DayParser.hpp"
#include "ecflow/node/parser/DefsStateParser.hpp"
#include "ecflow/node/parser/DefsStatusParser.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/node/parser/EventParser.hpp"
#include "ecflow/node/parser/ExternParser.hpp"
#include "ecflow/node/parser/GenericParser.hpp"
#include "ecflow/node/parser/InlimitParser.hpp"
#include "ecflow/node/parser/LabelParser.hpp"
#include "ecflow/node/parser/LateParser.hpp"
#include "ecflow/node/parser/LimitParser.hpp"
#include "ecflow/node/parser/MeterParser.hpp"
#include "ecflow/node/parser/MirrorParser.hpp"
#include "ecflow/node/parser/QueueParser.hpp"
#include "ecflow/node/parser/RepeatParser.hpp"
#include "ecflow/node/parser/TimeParser.hpp"
#include "ecflow/node/parser/TodayParser.hpp"
#include "ecflow/node/parser/TriggerParser.hpp"
#include "ecflow/node/parser/VariableParser.hpp"
#include "ecflow/node/parser/VerifyParser.hpp"
#include "ecflow/node/parser/ZombieAttrParser.hpp"

using namespace ecf;
using namespace std;

// #define DEBUG_PARSER 1

template <class T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, ","));
    return os;
}

class AliasParser : public Parser {
public:
    explicit AliasParser(DefsStructureParser* p) : Parser(p) {
        reserve_vec(21);
        addParser(new VariableParser(p));
        addParser(new LabelParser(p));
        addParser(new MeterParser(p));
        addParser(new EventParser(p));
        addParser(new TriggerParser(p));
        addParser(new InlimitParser(p));
        addParser(new LateParser(p));
        addParser(new DefsStatusParser(p));
        addParser(new CompleteParser(p));
        addParser(new TimeParser(p));
        addParser(new RepeatParser(p));
        addParser(new TodayParser(p));
        addParser(new CronParser(p));
        addParser(new LimitParser(p));
        addParser(new DateParser(p));
        addParser(new DayParser(p));
        addParser(new AutoCancelParser(p));
        addParser(new VerifyParser(p));
        addParser(new ZombieAttrParser(p));
        addParser(new QueueParser(p));
        addParser(new GenericParser(p));
        addParser(new AvisoParser(p));
        addParser(new MirrorParser(p));
    }

    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override {

        const char* first_token = lineTokens[0].c_str();
        if (Str::local_strcmp(first_token, keyword()) == 0) {

            if (lineTokens.size() < 2)
                throw std::runtime_error("Alias name missing.");

            addAlias(line, lineTokens);

            return true;
        }
        else if (Str::local_strcmp(first_token, "endalias") == 0) { // required at the end
            popNode();
            return true;
        }
        return Parser::doParse(line, lineTokens);
    }

    const char* keyword() const override { return "alias"; }

private:
    void addAlias(const std::string& line, std::vector<std::string>& lineTokens) const {
        bool check = (rootParser()->get_file_type() != PrintStyle::NET);

        if (nodeStack().empty() && rootParser()->parsing_node_string()) {
            alias_ptr alias = Alias::create(lineTokens[1], check);
            if (rootParser()->get_file_type() != PrintStyle::DEFS)
                alias->read_state(line, lineTokens);
            nodeStack().emplace(alias.get(), this);
            rootParser()->set_node_ptr(alias);
            return;
        }

        // bad test data can mean that last node is not a suite family or task, will fail parse
        if (nodeStack().empty())
            throw std::runtime_error("Add alias failed empty node stack");

        // alias can only be added to tasks
        Task* lastAddedTask = nodeStack_top()->isTask();
        if (lastAddedTask) {

            alias_ptr alias = lastAddedTask->add_alias(lineTokens[1]);
            alias->read_state(line, lineTokens);
            nodeStack().emplace(alias.get(), this);
        }
        else {
            if (nodeStack_top()->isAlias()) {

                // Alias can _only_ be added to tasks pop the top Node to get to the task
                popNode();
                addAlias(line, lineTokens);
            }
            else
                throw std::runtime_error("Add alias failed, expected task on node stack");
        }
    }
};

//================================================================================

class TaskParser : public Parser {
public:
    explicit TaskParser(DefsStructureParser* p) : Parser(p) {
        reserve_vec(25);
        addParser(new VariableParser(p));
        addParser(new TriggerParser(p));
        addParser(new LabelParser(p));
        addParser(new InlimitParser(p));
        addParser(new EventParser(p));
        addParser(new LateParser(p));
        addParser(new MeterParser(p));
        addParser(new DefsStatusParser(p));
        addParser(new CompleteParser(p));
        addParser(new TimeParser(p));
        addParser(new RepeatParser(p));
        addParser(new TodayParser(p));
        addParser(new CronParser(p));
        addParser(new LimitParser(p));
        addParser(new DateParser(p));
        addParser(new DayParser(p));
        addParser(new AutoCancelParser(p));
        addParser(new VerifyParser(p));
        addParser(new ZombieAttrParser(p));
        addParser(new AliasParser(p));
        addParser(new QueueParser(p));
        addParser(new AutoRestoreParser(p));
        addParser(new GenericParser(p));
        addParser(new AvisoParser(p));
        addParser(new MirrorParser(p));
    }

    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override {

        const char* first_token = lineTokens[0].c_str();
        if (Str::local_strcmp(first_token, keyword()) == 0) {
            if (lineTokens.size() < 2)
                throw std::runtime_error("Task name missing.");
            addTask(line, lineTokens);
            return true;
        }
        else if (Str::local_strcmp(first_token, "endfamily") == 0) {
            if (parent())
                return parent()->doParse(line, lineTokens);
        }
        else if (Str::local_strcmp(first_token, "endtask") == 0) { // optional
            popToContainerNode();
            return true;
        }
        return Parser::doParse(line, lineTokens);
    }

    const char* keyword() const override { return "task"; }

private:
    void addTask(const std::string& line, std::vector<std::string>& lineTokens) const {
        bool check = (rootParser()->get_file_type() != PrintStyle::NET);

        if (nodeStack().empty() && rootParser()->parsing_node_string()) {
            task_ptr task = Task::create(lineTokens[1], check);
            if (rootParser()->get_file_type() != PrintStyle::DEFS)
                task->read_state(line, lineTokens);
            nodeStack().emplace(task.get(), this);
            rootParser()->set_node_ptr(task);
            return;
        }

        // bad test data can mean that last node is not a suite family or task, will fail parse
        if (nodeStack().empty())
            throw std::runtime_error("Add task failed empty node stack");

        // end task is optional, so if we get another task, whilst in a task pop the parser
        if (nodeStack_top()->isTask()) {
            popToContainerNode(); // pop the node stack
        }

        NodeContainer* lastAddedContainer = nodeStack_top()->isNodeContainer();
        if (lastAddedContainer) {

            task_ptr task = Task::create(lineTokens[1], check);
            if (rootParser()->get_file_type() != PrintStyle::DEFS)
                task->read_state(line, lineTokens);
            nodeStack().emplace(task.get(), this);
            lastAddedContainer->addTask(task);
        }
        else {
            if (nodeStack_top()->isTask()) {

                // Task can _only_ be added to suite and families.
                // pop the top Node to get to the Container(suite or family), then call recursively to add task
                popNode();
                addTask(line, lineTokens);
            }
        }
    }
};

//================================================================================

class FamilyParser : public Parser {
public:
    explicit FamilyParser(DefsStructureParser* p) : Parser(p) {
        reserve_vec(25);
        addParser(new VariableParser(p));
        addParser(new TaskParser(p));
        addParser(new TriggerParser(p));
        addParser(new InlimitParser(p));
        addParser(new DefsStatusParser(p));
        addParser(new LimitParser(p));
        addParser(new CompleteParser(p));
        addParser(new MeterParser(p));
        addParser(new TimeParser(p));
        addParser(new LabelParser(p));
        addParser(new RepeatParser(p));
        addParser(new LateParser(p));
        addParser(new EventParser(p));
        addParser(new TodayParser(p));
        addParser(new CronParser(p));
        addParser(new DateParser(p));
        addParser(new DayParser(p));
        addParser(new AutoCancelParser(p));
        addParser(new VerifyParser(p));
        addParser(new ZombieAttrParser(p));
        addParser(new QueueParser(p));
        addParser(new AutoArchiveParser(p));
        addParser(new AutoRestoreParser(p));
        addParser(new GenericParser(p));
        addParser(new AvisoParser(p));
        addParser(new MirrorParser(p));
    }

    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override {

        const char* first_token = lineTokens[0].c_str();
        if (Str::local_strcmp(first_token, keyword()) == 0) {

            if (lineTokens.size() < 2)
                throw std::runtime_error("Family name missing.");

            addFamily(line, lineTokens);

            return true;
        }
        else if (Str::local_strcmp(first_token, "endfamily") == 0) {

            popFamily();
            return true;
        }
        else if (Str::local_strcmp(first_token, "endtask") == 0) { // optional
            popNode();
            return true;
        }
        return Parser::doParse(line, lineTokens);
    }

    const char* keyword() const override { return "family"; }

private:
    void addFamily(const std::string& line, const std::vector<std::string>& lineTokens) const {
        bool check = (rootParser()->get_file_type() != PrintStyle::NET);

        if (nodeStack().empty() && rootParser()->parsing_node_string()) {
            family_ptr family = Family::create(lineTokens[1], check);
            rootParser()->set_node_ptr(family);
            if (rootParser()->get_file_type() != PrintStyle::DEFS)
                family->read_state(line, lineTokens);
            nodeStack().emplace(family.get(), this);
        }
        else {
            assert(!nodeStack().empty());
            Suite* lastAddedSuite = nodeStack_top()->isSuite();
            if (lastAddedSuite) {

                family_ptr family = Family::create(lineTokens[1], check);
                if (rootParser()->get_file_type() != PrintStyle::DEFS)
                    family->read_state(line, lineTokens);

                nodeStack().emplace(family.get(), this);
                lastAddedSuite->addFamily(family);
            }
            else {
                // support hierarchical families
                Family* lastAddedFamily = nodeStack_top()->isFamily();
                if (lastAddedFamily) {

                    family_ptr family = Family::create(lineTokens[1], check);
                    if (rootParser()->get_file_type() != PrintStyle::DEFS)
                        family->read_state(line, lineTokens);

                    nodeStack().emplace(family.get(), this);
                    lastAddedFamily->addFamily(family);
                }
                else {
                    Task* lastAddedTask = nodeStack_top()->isTask();
                    if (lastAddedTask) {
                        // Pop the node, since tasks don't always have end task
                        popNode();
                        addFamily(line, lineTokens);
                    }
                }
            }
        }
    }

    void popFamily() const {
        // Compensate for the fact that Task don't have endtask, hence when we pop for a
        // family, the top should be a suite/family
        if (nodeStack_top()->isTask()) {
            nodeStack().pop(); // pop the task
            nodeStack().pop(); // pop the family to get to suite/family
        }
        else {
            nodeStack().pop(); // pop the family to get to suite/family
        }
    }
};

//================================================================================

// See ECFLOW-106, and SUP-1198, why we don't allow time,today,date,day ate the suite level.
class SuiteParser : public Parser {
public:
    explicit SuiteParser(DefsStructureParser* p) : Parser(p), started_(false) {
        reserve_vec(25);
        addParser(new VariableParser(p));
        addParser(new FamilyParser(p));
        addParser(new TaskParser(p));
        addParser(new LimitParser(p));
        addParser(new DefsStatusParser(p));
        addParser(new ClockParser(p));
        addParser(new InlimitParser(p));
        addParser(new RepeatParser(p));
        addParser(new LateParser(p));
        addParser(new CronParser(p));
        addParser(new AutoCancelParser(p));
        addParser(new VerifyParser(p));
        addParser(new ZombieAttrParser(p));
        addParser(new EventParser(p));
        addParser(new LabelParser(p));
        addParser(new CalendarParser(p));
        addParser(new MeterParser(p));
        addParser(new EndClockParser(p));
        addParser(new QueueParser(p));
        addParser(new AutoArchiveParser(p));
        addParser(new AutoRestoreParser(p));
        addParser(new GenericParser(p));
        addParser(new AvisoParser(p));
        addParser(new MirrorParser(p));
    }

    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override {

        const char* first_token = lineTokens[0].c_str();
        if (Str::local_strcmp(first_token, keyword()) == 0) {

            if (started_)
                throw std::runtime_error("Can't have hierarchical suites.");
            if (lineTokens.size() < 2)
                throw std::runtime_error("Suite name missing.");
            started_ = true;

            addSuite(line, lineTokens);

            return true;
        }
        else if (Str::local_strcmp(first_token, "endsuite") == 0) {

            if (!started_) {
                throw std::runtime_error("Misplaced endsuite..");
            }

            // ... process end suite
            while (!nodeStack().empty())
                nodeStack().pop();
            started_ = false; // since this parser is reused
            return true;
        }
        return Parser::doParse(line, lineTokens);
    }

    const char* keyword() const override { return "suite"; }

private:
    void addSuite(const std::string& line, std::vector<std::string>& lineTokens) const {

        if (!nodeStack().empty()) {
            throw std::runtime_error("SuiteParser::addSuite node stack should be empty");
        }

        bool check      = (rootParser()->get_file_type() != PrintStyle::NET);
        suite_ptr suite = Suite::create(lineTokens[1], check);
        if (rootParser()->get_file_type() != PrintStyle::DEFS)
            suite->read_state(line, lineTokens);

        nodeStack().emplace(suite.get(), this);
        if (defsfile())
            defsfile()->addSuite(suite);
        rootParser()->set_node_ptr(suite);
    }

    bool started_;
};

//================================================================================

DefsParser::DefsParser(DefsStructureParser* p) : Parser(p) {
    reserve_vec(5);
    addParser(new HistoryParser(p));
    addParser(new SuiteParser(p));
    addParser(new VariableParser(p, true));
    addParser(new ExternParser(p));

    // for defs stat only
    addParser(new DefsStateParser(p));
}

DefsParser::DefsParser(DefsStructureParser* p, bool /*node_parser_only*/) : Parser(p) {
    reserve_vec(4);
    addParser(new TaskParser(p));
    addParser(new FamilyParser(p));
    addParser(new SuiteParser(p));
    addParser(new AliasParser(p));
}
