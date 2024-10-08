/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_TreeGeneration_HPP
#define ecflow_http_TreeGeneration_HPP

#include <vector>

#include "ecflow/http/JSON.hpp"
#include "ecflow/http/TypeToJson.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

namespace ecf::http {

struct BasicTree
{
    BasicTree() : root_(ojson::object({})), stack_{&root_} {}

    void begin_visit(const Suite& suite) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[suite.name()] = ojson::object({});
        stack_.push_back(&current);
    }
    void end_visit(const Suite& suite [[maybe_unused]]) { stack_.pop_back(); }

    void begin_visit(const Family& family) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[family.name()] = ojson::object({});
        stack_.push_back(&current);
    }
    void end_visit(const Family& family [[maybe_unused]]) { stack_.pop_back(); }

    void begin_visit(const Task& task) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[task.name()] = ojson::object({});
        stack_.push_back(&current);
    }
    void end_visit(const Task& task [[maybe_unused]]) { stack_.pop_back(); }

    void begin_visit(const Alias& alias) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[alias.name()] = ojson::object({});
        stack_.push_back(&current);
    }
    void end_visit(const Alias& alias [[maybe_unused]]) { stack_.pop_back(); }

    const ojson& content() const { return root_; }

private:
    ojson root_;
    std::vector<ojson*> stack_;
};

struct FullTree
{
    FullTree(bool with_id, bool with_gen_vars)
        : root_(ojson::object({})),
          stack_{&root_},
          with_id_{with_id},
          with_gen_vars_{with_gen_vars} {}

    void begin_visit(const Suite& suite) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[suite.name()] = ojson::object({});

        current["type"] = "suite";
        if (with_id_) {
            current["id"] = suite.absNodePath();
        }
        publish_state(suite, current);
        publish_attributes(suite, current, with_gen_vars_);

        ojson& children = current["children"] = ojson::object({});
        stack_.push_back(&children);
    }

    void end_visit(const Suite& suite [[maybe_unused]]) { stack_.pop_back(); }

    void begin_visit(const Family& family) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[family.name()] = ojson::object({});

        current["type"] = "family";
        if (with_id_) {
            current["id"] = family.absNodePath();
        }
        publish_state(family, current);
        publish_attributes(family, current, with_gen_vars_);

        ojson& children = current["children"] = ojson::object({});
        stack_.push_back(&children);
    }

    void end_visit(const Family& family [[maybe_unused]]) { stack_.pop_back(); }

    void begin_visit(const Task& task) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[task.name()] = ojson::object({});

        current["type"] = "task";
        if (with_id_) {
            current["id"] = task.absNodePath();
        }
        publish_state(task, current);
        publish_attributes(task, current, with_gen_vars_);

        ojson& children = current["aliases"] = ojson::object({});
        stack_.push_back(&children);
    }

    void end_visit(const Task& task [[maybe_unused]]) { stack_.pop_back(); }

    void begin_visit(const Alias& alias) {
        ojson& parent_ = *stack_.back();
        ojson& current = parent_[alias.name()] = ojson::object({});
        stack_.push_back(&current);

        current["type"] = "alias";
        if (with_id_) {
            current["id"] = alias.absNodePath();
        }
        publish_state(alias, current);
        publish_attributes(alias, current, with_gen_vars_);
    }

    void end_visit(const Alias& alias [[maybe_unused]]) { stack_.pop_back(); }

    const ojson& content() const { return root_; }

private:
    static void publish_state(const Node& node, ojson& parent) {
        ojson& state = parent["state"] = ojson::object({});

        state["node"]    = NState::toString(node.state());
        state["default"] = DState::toString(node.dstate());
    }

    template <typename T, typename... P>
    static ojson publish_atribute(const T& attr, std::string_view type, P... parameters) {
        auto j = ojson::object({});
        to_json(j, attr);
        j["type"] = type;
        if constexpr (sizeof...(P) > 0) {
            ((j[parameters.first] = parameters.second), ...);
        }
        return j;
    }

    static void publish_attributes(const Node& node, ojson& parent, bool with_gen_vars) {
        ojson& array = parent["attributes"] = ojson::array();

        for (const auto& attr : node.labels()) {
            array.emplace_back(publish_atribute(attr, "label"));
        }
        for (const auto& attr : node.meters()) {
            array.emplace_back(publish_atribute(attr, "meter"));
        }
        for (const auto& attr : node.events()) {
            array.emplace_back(publish_atribute(attr, "event"));
        }
        for (const auto& attr : node.variables()) {
            array.emplace_back(publish_atribute(attr, "variable"));
        }
        if (with_gen_vars) {
            for (const auto& attr : node.gen_variables()) {
                array.emplace_back(publish_atribute(attr, "variable", std::make_pair("generated", true)));
            }
        }
        for (const auto& attr : node.limits()) {
            array.emplace_back(publish_atribute(*attr, "limit"));
        }
        for (const auto& attr : node.inlimits()) {
            array.emplace_back(publish_atribute(attr, "inlimit"));
        }
        for (const auto& attr : node.dates()) {
            array.emplace_back(publish_atribute(attr, "date"));
        }
        for (const auto& attr : node.days()) {
            array.emplace_back(publish_atribute(attr, "day"));
        }
        for (const auto& attr : node.crons()) {
            array.emplace_back(publish_atribute(attr, "cron"));
        }
        for (const auto& attr : node.timeVec()) {
            array.emplace_back(publish_atribute(attr, "time"));
        }
        for (const auto& attr : node.todayVec()) {
            array.emplace_back(publish_atribute(attr, "today"));
        }
        {
            if (const auto* attr = node.get_late(); attr) {
                array.emplace_back(publish_atribute(*attr, "late"));
            }
        }
        {
            if (const auto* attr = node.get_autocancel(); attr) {
                array.emplace_back(publish_atribute(*attr, "autocancel"));
            }
        }
        {
            if (const auto* attr = node.get_autoarchive(); attr) {
                array.emplace_back(publish_atribute(*attr, "autoarchive"));
            }
        }
        {
            if (const auto* attr = node.get_autorestore(); attr) {
                array.emplace_back(publish_atribute(*attr, "autorestore"));
            }
        }
        {
            if (const auto& attr = node.repeat(); !attr.empty()) {
                array.emplace_back(publish_atribute(attr, "repeat"));
            }
        }
        {
            if (const auto& attr = node.completeExpression(); !attr.empty()) {
                auto j          = ojson::object({});
                j["expression"] = attr;
                j["type"]       = "complete";
                array.emplace_back(j);
            }
        }
        {
            if (const auto& attr = node.triggerExpression(); !attr.empty()) {
                auto j          = ojson::object({});
                j["expression"] = attr;
                j["type"]       = "trigger";
                array.emplace_back(j);
            }
        }
        for (const auto& attr : node.queues()) {
            array.emplace_back(publish_atribute(attr, "queue"));
        }
        for (const auto& attr : node.zombies()) {
            array.emplace_back(publish_atribute(attr, "zombie"));
        }
        for (const auto& attr : node.generics()) {
            array.emplace_back(publish_atribute(attr, "generic"));
        }

        if (auto flag = node.get_flag(); flag.flag()) {
            array.emplace_back(publish_atribute(flag, "flag"));
        }

        for (const auto& aviso : node.avisos()) {
            array.emplace_back(publish_atribute(aviso, "aviso"));
        }
        for (const auto& mirror : node.mirrors()) {
            array.emplace_back(publish_atribute(mirror, "mirror"));
        }
    }

private:
    ojson root_;
    std::vector<ojson*> stack_;
    bool with_id_;
    bool with_gen_vars_;
};

} // namespace ecf::http

#endif /* ecflow_http_TreeGeneration_HPP */
