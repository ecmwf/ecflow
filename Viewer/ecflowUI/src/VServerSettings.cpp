/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VServerSettings.hpp"

#include <cassert>

#include <boost/property_tree/json_parser.hpp>

#include "DirectoryHandler.hpp"
#include "ServerHandler.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"
#include "SessionHandler.hpp"
#include "SuiteFilter.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VSettings.hpp"
#include "ecflow/core/Filesystem.hpp"

std::map<VServerSettings::Param, std::string> VServerSettings::notifyIds_;
std::map<VServerSettings::Param, std::string> VServerSettings::parNames_;
VProperty* VServerSettings::globalProp_ = nullptr;

VServerSettings::VServerSettings(ServerHandler* server) : server_(server), prop_(nullptr), guiProp_(nullptr) {
    if (parNames_.empty()) {
        parNames_[AutoUpdate]              = "server.update.autoUpdate";
        parNames_[UpdateRate]              = "server.update.updateRateInSec";
        parNames_[AdaptiveUpdate]          = "server.update.adaptiveUpdate";
        parNames_[AdaptiveUpdateIncrement] = "server.update.adaptiveUpdateIncrementInSec";
        parNames_[MaxAdaptiveUpdateRate]   = "server.update.maxAdaptiveUpdateRateInMin";
        parNames_[AdaptiveUpdateMode]      = "server.update.adaptiveUpdateMode";

        parNames_[MaxOutputFileLines] = "server.files.maxOutputFileLines";
        parNames_[ReadFromDisk]       = "server.files.readFilesFromDisk";
        parNames_[UserLogServerHost]  = "server.files.logServerHost";
        parNames_[UserLogServerPort]  = "server.files.logServerPort";

        parNames_[NodeMenuMode]             = "server.menu.nodeMenuMode";
        parNames_[NodeMenuModeForDefStatus] = "server.menu.defStatusMenuModeControl";
        parNames_[UidForServerLogTransfer]  = "server.files.uidForServerLogTransfer";
        parNames_[MaxSizeForTimelineData]   = "server.files.maxSizeForTimelineData";

        parNames_[NotifyAbortedEnabled] = "server.notification.aborted.enabled";
        parNames_[NotifyAbortedPopup]   = "server.notification.aborted.popup";
        parNames_[NotifyAbortedSound]   = "server.notification.aborted.sound";

        parNames_[NotifyRestartedEnabled] = "server.notification.restarted.enabled";
        parNames_[NotifyRestartedPopup]   = "server.notification.restarted.popup";
        parNames_[NotifyRestartedSound]   = "server.notification.restarted.sound";

        parNames_[NotifyLateEnabled] = "server.notification.late.enabled";
        parNames_[NotifyLatePopup]   = "server.notification.late.popup";
        parNames_[NotifyLateSound]   = "server.notification.late.sound";

        parNames_[NotifyZombieEnabled] = "server.notification.zombie.enabled";
        parNames_[NotifyZombiePopup]   = "server.notification.zombie.popup";
        parNames_[NotifyZombieSound]   = "server.notification.zombie.sound";

        parNames_[NotifyAliasEnabled] = "server.notification.alias.enabled";
        parNames_[NotifyAliasPopup]   = "server.notification.alias.popup";
        parNames_[NotifyAliasSound]   = "server.notification.alias.sound";

        notifyIds_[NotifyAbortedEnabled]   = "aborted";
        notifyIds_[NotifyRestartedEnabled] = "restarted";
        notifyIds_[NotifyLateEnabled]      = "late";
        notifyIds_[NotifyZombieEnabled]    = "zombie";
        notifyIds_[NotifyAliasEnabled]     = "alias";
    }

    assert(globalProp_);

    prop_ = globalProp_->clone(false, true, true); // they all use their master by default!

    for (auto& parName : parNames_) {
        if (VProperty* p = prop_->find(parName.second)) {
            parToProp_[parName.first] = p;
            propToPar_[p]             = parName.first;
            p->addObserver(this);
        }
        else {
            UiLog().dbg() << "VServerSettings - could not find property: " << parName.second;
            assert(0);
        }
    }

    guiProp_ = VConfig::instance()->cloneServerGui(prop_);
    assert(guiProp_);
}

VServerSettings::~VServerSettings() {
    delete prop_;
    delete guiProp_;
}

int VServerSettings::intValue(Param par) const {
    return property(par)->value().toInt();
}

bool VServerSettings::boolValue(Param par) const {
    return property(par)->value().toBool();
}

QString VServerSettings::stringValue(Param par) const {
    return property(par)->value().toString();
}

VProperty* VServerSettings::property(Param par) const {
    auto it = parToProp_.find(par);
    if (it != parToProp_.end()) {
        return it->second;
    }
    else {
        assert(0);
    }

    return nullptr;
}

void VServerSettings::notifyChange(VProperty* p) {
    auto it = propToPar_.find(p);
    if (it != propToPar_.end()) {
        server_->confChanged(it->second, it->first);
    }
    else {
        assert(0);
    }
}

std::string VServerSettings::notificationId(Param par) {
    auto it = notifyIds_.find(par);
    if (it != notifyIds_.end()) {
        return it->second;
    }
    return {};
}

VServerSettings::Param VServerSettings::notificationParam(const std::string& id) {
    for (auto& notifyId : notifyIds_) {
        if (notifyId.second == id) {
            return notifyId.first;
        }
    }

    return UnknownParam;
}

bool VServerSettings::notificationsEnabled() const {
    for (auto& notifyId : notifyIds_) {
        if (boolValue(notifyId.first)) {
            return true;
        }
    }
    return false;
}

void VServerSettings::loadSettings() {
    SessionItem* cs   = SessionHandler::instance()->current();
    std::string fName = cs->serverFile(server_->name());

    // Load settings stored in VProperty
    VConfig::instance()->loadSettings(fName, guiProp_, false);

    // Some  settings are read through VSettings
    if (fs::exists(fName)) {
        VSettings vs(fName);
        vs.read(false);
        vs.beginGroup("suite_filter");
        server_->suiteFilter()->readSettings(&vs);
        vs.endGroup();
    }
}

void VServerSettings::saveSettings() {
    SessionItem* cs   = SessionHandler::instance()->current();
    std::string fName = cs->serverFile(server_->name());

    // We save the suite filter through VSettings
    VSettings vs("");
    vs.beginGroup("suite_filter");
    server_->suiteFilter()->writeSettings(&vs);
    vs.endGroup();

    VConfig::instance()->saveSettings(fName, guiProp_, &vs, false);
}

void VServerSettings::importRcFiles() {
    SessionItem* cs = SessionHandler::instance()->current();

    for (int i = 0; i < ServerList::instance()->count(); i++) {
        std::string name = ServerList::instance()->itemAt(i)->name();

        std::string rcFile(DirectoryHandler::concatenate(DirectoryHandler::rcDir(), name + ".options"));

        using boost::property_tree::ptree;
        ptree pt;

        if (VConfig::instance()->readRcFile(rcFile, pt)) {
            std::string jsonName = cs->serverFile(name);
            write_json(jsonName, pt);
        }
    }
}

// Called from VConfigLoader
void VServerSettings::load(VProperty* p) {
    globalProp_ = p;
}

static SimpleLoader<VServerSettings> loader("server");
