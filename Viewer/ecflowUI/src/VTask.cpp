/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VTask.hpp"

#include "VNode.hpp"
#include "VReply.hpp"
#include "VTaskObserver.hpp"
#include "ecflow/attribute/Zombie.hpp"

VTask::VTask(Type t, VTaskObserver* obs)
    : type_(t),
      status_(NOSTATUS),
      targetPath_("/"),
      node_(nullptr),
      reply_(nullptr) {
    if (obs) {
        observers_.push_back(obs);
    }
    reply_ = new VReply();
}

VTask::VTask(Type t, VNode* node, VTaskObserver* obs) : type_(t), status_(NOSTATUS), node_(node), reply_(nullptr) {
    if (node_) {
        targetPath_ = node_->absNodePath();
    }

    if (obs) {
        observers_.push_back(obs);
    }

    reply_ = new VReply();
}

VTask::~VTask() {
    delete reply_;
}

// Task for a server
VTask_ptr VTask::create(Type t, VTaskObserver* obs) {
    return VTask_ptr(new VTask(t, obs));
}

// Task for a node
VTask_ptr VTask::create(Type t, VNode* node, VTaskObserver* obs) {
    return VTask_ptr(new VTask(t, node, obs));
}

const std::string& VTask::typeString() const {
    static std::map<Type, std::string> names;
    if (names.empty()) {
        names[NoTask]                = "notask";
        names[CommandTask]           = "command";
        names[OverviewTask]          = "overview";
        names[WhySyncTask]           = "why_sync";
        names[ManualTask]            = "manual";
        names[ScriptTask]            = "script";
        names[JobTask]               = "job";
        names[MessageTask]           = "message";
        names[OutputTask]            = "output";
        names[StatsTask]             = "stats";
        names[NewsTask]              = "news";
        names[SyncTask]              = "sync";
        names[ResetTask]             = "reset";
        names[SuiteAutoRegisterTask] = "suite_auto_register";
        names[SuiteListTask]         = "suite_list";
        names[ScriptEditTask]        = "script_edit";
        names[ScriptPreprocTask]     = "script_preproc";
        names[ScriptSubmitTask]      = "script_submit";
        names[HistoryTask]           = "history";
        names[LogOutTask]            = "logout";
        names[ZombieListTask]        = "zombie_list";
        names[ZombieCommandTask]     = "zombie_command";
    }

    if (names.find(type_) != names.end()) {
        return names[type_];
    }

    return names[NoTask];
}

const std::string& VTask::param(const std::string& key) const {
    static std::string emptyStr("");
    auto it = params_.find(key);
    if (it != params_.end()) {
        return it->second;
    }
    return emptyStr;
}

void VTask::status(Status s, bool broadcastIt) {
    status_ = s;
    if (broadcastIt) {
        broadcast();
    }
}

void VTask::setZombie(const Zombie& z) {
    zombie_ = z;
}

void VTask::removeObserver(VTaskObserver* o) {
    auto it = std::find(observers_.begin(), observers_.end(), o);
    if (it != observers_.end()) {
        observers_.erase(it);
    }
}

void VTask::broadcast() {
    // VTask always exists as a shared pointer, so it is safe to pass on
    // this shared pointer to the observer.
    for (auto& observer : observers_) {
        observer->taskChanged(shared_from_this());
    }
}
