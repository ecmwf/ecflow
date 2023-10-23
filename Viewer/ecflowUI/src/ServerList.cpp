//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerList.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <QTimer>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "Converter.hpp"
#include "DirectoryHandler.hpp"
#include "MainWindow.hpp"
#include "ServerItem.hpp"
#include "Str.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VProperty.hpp"
#include "VReply.hpp"

ServerList* ServerList::instance_ = nullptr;

#define _UI_SERVERLIST_DEBUG
#define _UI_SERVERSYSTEMLIST_DEBUG

//==================================
//
// ServerListTmpItem
//
//==================================

ServerListTmpItem::ServerListTmpItem(ServerItem* item) : name_(item->name()), host_(item->host()), port_(item->port()) {
}

//======================================
//
// ServerListSystemFileManager
//
//======================================

ServerListSystemFileManager::~ServerListSystemFileManager() {
    if (prop_) {
        delete prop_;
    }
}

void ServerListSystemFileManager::clear() {
    files_.clear();
    fetchedFiles_.clear();
    unfetchedFiles_.clear();

    for (auto& item : changedItems_) {
        delete item;
    }
    changedItems_.clear();
    pendingItems_.clear();
}

std::vector<std::string> ServerListSystemFileManager::buildFileList() {
    if (!prop_) {
        std::vector<std::string> propVec;
        propVec.emplace_back("server.systemList.paths");
        assert(!prop_);
        prop_ = new PropertyMapper(propVec, this);
        assert(prop_);
    }

    std::vector<std::string> paths;

    // Ui config settings take precedence
    auto p       = VConfig::instance()->find("server.systemList.paths");
    bool useProp = false;
    if (p) {
        UiLog().info() << UI_FN_INFO << "take paths from Preferences";
        auto s = p->valueAsStdString();
        if (!s.empty()) {
            useProp = true;
            std::vector<std::string> sVec;
            ecf::Str::split(s, sVec, ":");
            for (auto v : sVec) {
                if (std::find(paths.begin(), paths.end(), v) == paths.end()) {
                    paths.push_back(v);
                }
            }
        }
    }

    // otherwise the env var is used
    if (!useProp) {
        if (char* ch = getenv("ECFLOW_SYSTEM_SERVERS_LIST")) {
            UiLog().info() << UI_FN_INFO << "take paths from $ECFLOW_SYSTEM_SERVERS_LIST";
            auto s = std::string(ch);
            if (!s.empty()) {
                std::vector<std::string> sVec;
                ecf::Str::split(s, sVec, ":");
                for (auto v : sVec) {
                    if (std::find(paths.begin(), paths.end(), v) == paths.end()) {
                        paths.push_back(v);
                    }
                }
            }
        }
    }

    return paths;
}

bool ServerListSystemFileManager::fileListSameAs(const std::vector<std::string>& v1,
                                                 const std::vector<std::string>& v2) const {
    // see if there was a change
    if (v1.size() == v2.size()) {
        for (std::size_t i = 0; i < v1.size(); i++) {
            if (v1[i] != v2[i]) {
                return false;
            }
        }
#ifdef _UI_SERVERSYSTEMLIST_DEBUG
        UiLog().dbg() << UI_FN_INFO << "new paths are the same as the old ones";
#endif
        return true;
    }
    return false;
}

// called on startup (maybe in a delayed mode)
void ServerListSystemFileManager::sync() {
    if (state_ == FetchState) {
        // TODO: handle it
        return;
    }

    clear();
    files_ = buildFileList();
    if (!files_.empty()) {
#ifdef _UI_SERVERSYSTEMLIST_DEBUG
        UiLog().dbg() << UI_FN_INFO << "start fetch";
#endif
        state_ = FetchState;
        fetchManager_->fetchFiles(files_);
    }
    else {
        state_ = SyncedState;
    }
}

// called when the settings changed
void ServerListSystemFileManager::syncInternal(const std::vector<std::string>& newFiles) {
    clear();
    files_ = newFiles;
    if (!files_.empty()) {
#ifdef _UI_SERVERSYSTEMLIST_DEBUG
        UiLog().dbg() << UI_FN_INFO << "start fetch";
#endif
        state_ = FetchState;
        fetchManager_->fetchFiles(files_);
    }
    else {
        state_ = SyncedState;
    }
}

void ServerListSystemFileManager::concludeSync() {
    state_ = SyncedState;

    // if there is nothing more to fetch we load all the pending items
    if (!pendingItems_.empty()) {
        loadPending();
    }
    MainWindow::initServerSyncTb();
}

// callbacks from the file provider
void ServerListSystemFileManager::fileFetchFinished(VReply* r) {
#ifdef _UI_SERVERSYSTEMLIST_DEBUG
    UiLog().dbg() << UI_FN_INFO;
#endif

    std::vector<std::string> paths;
    if (!r->tmpFiles().empty()) {
        syncDate_ = QDateTime::currentDateTime();
        for (auto f : r->tmpFiles()) {
            if (f) {
#ifdef _UI_SERVERSYSTEMLIST_DEBUG
                UiLog().dbg() << " f=" << f->sourcePath();
#endif
                paths.emplace_back(f->path());
                if (std::find(fetchedFiles_.begin(), fetchedFiles_.end(), f->sourcePath()) == fetchedFiles_.end()) {
                    fetchedFiles_.emplace_back(f->sourcePath());
                }
            }
        }
    }

    for (auto f : fetchManager_->filesToFetch()) {
        if (std::find(fetchedFiles_.begin(), fetchedFiles_.end(), f) == fetchedFiles_.end()) {
            unfetchedFiles_.emplace_back(f);
        }
    }

    if (!paths.empty()) {
        loadFiles(paths);
    }
}

void ServerListSystemFileManager::fileFetchFailed(VReply* /*r*/) {
    concludeSync();
}

void ServerListSystemFileManager::notifyChange(VProperty* p) {
    // cannot do
    if (fetchManager_->isEmpty() && prop_ && p && p->path() == "server.systemList.paths") {
        auto newFiles = buildFileList();
        if (!fileListSameAs(files_, newFiles)) {
            syncInternal(newFiles);
        }
    }
}

void ServerListSystemFileManager::delayedFetchFiles(const std::vector<std::string>& paths) {
    fetchManager_->fetchFiles(paths);
}

// load one set of files
void ServerListSystemFileManager::loadFiles(const std::vector<std::string>& paths) {
    std::vector<std::string> includedPaths;
    for (auto f : paths) {
        loadFile(f, pendingItems_, includedPaths);
    }

    // included paths
    if (!includedPaths.empty()) {
        std::vector<std::string> mp;
        for (auto f : includedPaths) {
            if (std::find(fetchedFiles_.begin(), fetchedFiles_.end(), f) == fetchedFiles_.end()) {
                mp.emplace_back(f);
            }
        }
        if (!mp.empty()) {
#ifdef _UI_SERVERSYSTEMLIST_DEBUG
            UiLog().dbg() << UI_FN_INFO << "includedPaths=" << mp;
#endif
            // we need to allow time for the manager to clean up
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
            QTimer::singleShot(0, [mp, this]() { this->delayedFetchFiles(mp); });
#endif
            return;
        }
    }

    // if there is nothing to fetch we load all the pending items
    concludeSync();
}

// load a given file
void ServerListSystemFileManager::loadFile(const std::string& fPath,
                                           std::vector<ServerListTmpItem>& sysVec,
                                           std::vector<std::string>& includedPaths) {
    std::ifstream in(fPath.c_str());

    if (in.good()) {
        std::string line;
        while (getline(in, line)) {
            std::string buf = boost::trim_left_copy(line);
            if (buf.size() > 0 && buf[0] == '#')
                continue;

            if (buf.size() > 0 && buf[0] == '/') {
                std::string p = boost::trim_right_copy(buf);
                if (std::find(includedPaths.begin(), includedPaths.end(), p) == includedPaths.end()) {
                    includedPaths.emplace_back(p);
                }
                continue;
            }

            std::stringstream ssdata(line);
            std::vector<std::string> vec;

            while (ssdata >> buf) {
                vec.push_back(buf);
            }

            if (vec.size() >= 3) {
                std::string errStr, name = vec[0], host = vec[1], port = vec[2];
                if (ServerList::instance()->checkItemToAdd(name, host, port, false, errStr)) {
                    sysVec.emplace_back(vec[0], vec[1], vec[2]);
                }
            }
        }
    }
    in.close();
}

void ServerListSystemFileManager::loadPending() {
    state_ = SyncedState;

    // nothing to sync
    if (pendingItems_.empty()) {
        return;
    }

    ServerList::instance()->loadSystemItems(pendingItems_, changedItems_);
}

//==================================
//
// ServerList
//
//==================================

ServerList::ServerList() {
    sysFileManager_ = new ServerListSystemFileManager();
}

ServerList::~ServerList() {
    if (sysFileManager_) {
        delete sysFileManager_;
    }
}

// Singleton instance method
ServerList* ServerList::instance() {
    if (!instance_)
        instance_ = new ServerList();
    return instance_;
}

//===========================================================
// Managing server items
//===========================================================

ServerItem* ServerList::itemAt(int index) {
    return (index >= 0 && index < static_cast<int>(items_.size())) ? items_.at(index) : nullptr;
}

ServerItem* ServerList::find(const std::string& name) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if ((*it)->name() == name)
            return *it;
    }
    return nullptr;
}

ServerItem* ServerList::find(const std::string& name, const std::string& host, const std::string& port) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if ((*it)->name() == name && (*it)->host() == host && (*it)->port() == port)
            return *it;
    }
    return nullptr;
}

ServerItem* ServerList::add(const std::string& name,
                            const std::string& host,
                            const std::string& port,
                            const std::string& user,
                            bool favourite,
                            bool ssl,
                            bool saveIt) {
    std::string errStr;
    if (!checkItemToAdd(name, host, port, true, errStr)) {
        throw std::runtime_error(errStr);
        return nullptr;
    }

    auto* item = new ServerItem(name, host, port, user, favourite, ssl);

    items_.push_back(item);

    if (saveIt)
        save();

    broadcastChanged();

    return item;
}

void ServerList::remove(ServerItem* item) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        items_.erase(it);
        // item->broadcastDeletion();
        delete item;

        save();
        broadcastChanged();
    }
}

ServerItem* ServerList::reset(ServerItem* item,
                              const std::string& name,
                              const std::string& host,
                              const std::string& port,
                              const std::string& user,
                              bool ssl) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        // Check if there is an item with the same name. Names have to be unique!
        if (item->name() != name && find(name))
            return nullptr;

        if (host != item->host() || port != item->port()) {
            items_.erase(it);
            broadcastChanged();
            delete item;
            item = add(name, host, port, user, false, ssl, true);
            save();
            broadcastChanged();
        }
        else {
            assert(host == item->host());
            assert(port == item->port());
            item->reset(name, host, port, user, ssl);
            save();
            broadcastChanged();
        }
    }
    return item;
}

void ServerList::setFavourite(ServerItem* item, bool b) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        item->setFavourite(b);
        for (auto it = observers_.begin(); it != observers_.end(); ++it)
            (*it)->notifyServerListFavouriteChanged(item);
    }
}

std::string ServerList::uniqueName(const std::string& name) {
    bool hasIt = false;
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if ((*it)->name() == name) {
            hasIt = true;
            break;
        }
    }
    if (!hasIt) {
        return name;
    }

    for (int i = 1; i < 100; i++) {
        std::ostringstream c;
        c << i;
        std::string currentName = name + "_" + c.str();

        hasIt                   = false;
        for (auto it = items_.begin(); it != items_.end(); ++it) {
            if ((*it)->name() == currentName) {
                hasIt = true;
                break;
            }
        }
        if (!hasIt) {
            return currentName;
        }
    }

    return name;
}

void ServerList::rescan() {
}

bool ServerList::checkItemToAdd(const std::string& name,
                                const std::string& host,
                                const std::string& port,
                                bool checkDuplicate,
                                std::string& errStr) {
    if (name.empty()) {
        errStr = "Empty server name";
        return false;
    }
    else if (host.empty()) {
        errStr = "Empty server host";
        return false;
    }
    else if (port.empty()) {
        errStr = "Empty server port";
        return false;
    }

    try {
        ecf::convert_to<int>(port);
    }
    catch (const ecf::bad_conversion&) {
        errStr = "Invalid port number: " + port;
        return false;
    }

    if (checkDuplicate && find(name)) {
        errStr = "Duplicated server name: " + name;
        return false;
    }

    return true;
}

//===========================================================
// Initialisation
//===========================================================

void ServerList::init() {
    localFile_ = DirectoryHandler::concatenate(DirectoryHandler::configDir(), "servers.txt");
    load();

    // Note: system files are loaded in a delayed mode
    // Note: we do not load the rc file any more. See ECFLOW-1825
}

bool ServerList::load() {
    UiLog().dbg() << UI_FN_INFO << "-->";

    std::ifstream in(localFile_.c_str());
    if (!in.good())
        return false;

    std::string errStr;
    std::string line;
    int lineCnt = 1;
    while (getline(in, line)) {
        // We ignore comment lines
        std::string buf = boost::trim_left_copy(line);
        if (buf.size() > 0 && buf.at(0) == '#') {
            lineCnt++;
            continue;
        }

        std::vector<std::string> sv;
        boost::split(sv, line, boost::is_any_of(","));

        bool favourite = false;
        if (sv.size() >= 4)
            favourite = (sv[3] == "1") ? true : false;

        bool sys = false;
        if (sv.size() >= 5)
            sys = (sv[4] == "1") ? true : false;

        bool ssl = false;
        if (sv.size() >= 6)
            ssl = (sv[5] == "1") ? true : false;

        std::string user;
        if (sv.size() >= 7)
            user = sv[6];

        if (sv.size() >= 3) {
            std::string name = sv[0], host = sv[1], port = sv[2];
            ServerItem* item = nullptr;
            try {
                item = add(name, host, port, user, favourite, ssl, false);
                UI_ASSERT(item != nullptr, "name=" << name << " host=" << host << " port=" << port << " user=" << user);
                item->setSystem(sys);
            }
            catch (std::exception& e) {
                std::string err = e.what();
                err += " [name=" + name + ",host=" + host + ",port=" + port + "]";
                errStr += err + " (in line " + UiLog::toString(lineCnt) + ")<br>";
                UiLog().err() << "  " << err << " (in line " << lineCnt << ")";
            }
        }

        lineCnt++;
    }

    in.close();

    if (!errStr.empty()) {
        errStr = "<b>Could not parse</b> the server list file <u>" + localFile_ + "</u>. The \
                    following errors occured:<br><br>" +
                 errStr + "<br>Please <b>correct the errors</b> in the server list file and restart ecFlowUI!";
        UserMessage::setEchoToCout(false);
        UserMessage::message(UserMessage::ERROR, true, errStr);
        UserMessage::setEchoToCout(true);
        exit(1);
    }

    if (count() == 0)
        return false;

    return true;
}

void ServerList::save() {
    std::ofstream out;
    out.open(localFile_.c_str());
    if (!out.good())
        return;

    out << "#Name Host Port Favourite System Ssl user" << std::endl;

    for (auto& item : items_) {
        std::string fav = (item->isFavourite()) ? "1" : "0";
        std::string ssl = (item->isSsl()) ? "1" : "0";
        std::string sys = (item->isSystem()) ? "1" : "0";
        out << item->name() << "," << item->host() << "," << item->port() << "," << fav << "," << sys << "," << ssl
            << "," << item->user() << std::endl;
    }
    out.close();
}

// called on startup (possibly in delayed mode)
void ServerList::syncSystemFiles() {
    sysFileManager_->sync();
}

void ServerList::loadSystemItems(const std::vector<ServerListTmpItem>& sysVec,
                                 std::vector<ServerListSyncChangeItem*>& changeVec) {
    // nothing to sync
    if (sysVec.empty()) {
        return;
    }

    bool changed      = false;
    bool needBrodcast = false;

#ifdef _UI_SERVERLIST_DEBUG
    UiLog().dbg() << UI_FN_INFO << "Load system server list:";
#endif

    // See what changed or was added
    for (auto& sysItem : sysVec) {
#ifdef _UI_SERVERLIST_DEBUG
        UiLog().dbg() << sysItem.name() << " " + sysItem.host() << " " + sysItem.port();
#endif
        ServerItem* item = nullptr;

        // There is a server with the same name, host and port as in the local list. We
        // mark it as system
        item = find(sysItem.name(), sysItem.host(), sysItem.port());
        if (item) {
            if (!item->isSystem()) {
#ifdef _UI_SERVERLIST_DEBUG
                UiLog().dbg() << " -> already in server-list (same name, host, port). Mark as system server";
#endif
                changed = true;
                changeVec.push_back(
                    new ServerListSyncChangeItem(sysItem, sysItem, ServerListSyncChangeItem::SetSysChange));
                item->setSystem(true);
            }
            continue;
        }

        // There is no server with the same name in the local list
        item = find(sysItem.name());
        if (!item) {
#ifdef _UI_SERVERLIST_DEBUG
            UiLog().dbg() << "  -> name is not in server-list. Import as system server";
#endif
            changed          = true;
            std::string name = sysItem.name(), host = sysItem.host(), port = sysItem.port();
            try {
                item = add(name, host, port, "", false, false, false);
                UI_ASSERT(item != nullptr, "name=" << name << " host=" << host << " port=" << port);
                item->setSystem(true);
                changeVec.push_back(
                    new ServerListSyncChangeItem(sysItem, sysItem, ServerListSyncChangeItem::AddedChange));
            }
            catch (std::exception& e) {
                std::string err = e.what();
                UiLog().err() << "  Could not sync server (name=" << name << ",host=" << host << "port=" << port
                              << "). " << err;
            }
            continue;
        }
        // There is a server with the same name but with different host or/and port
        else {
#ifdef _UI_SERVERLIST_DEBUG
            UiLog().dbg() << "  -> name exsist in server-list with different port or/and host: " << item->host() << "@"
                          << item->port() << " ! Reset host and port";
#endif
            changed      = true;
            needBrodcast = true;
            // assert(item->name() == sysVec[i].name());

            ServerListTmpItem localTmp(item);
            changeVec.push_back(new ServerListSyncChangeItem(sysItem, localTmp, ServerListSyncChangeItem::MatchChange));

            item = reset(item, sysItem.name(), sysItem.host(), sysItem.port(), "", false);
            if (item) {
                item->setSystem(true);
            }
            // item->reset(i.name(),i.host(),i.port(), "",false);
            // broadcastChanged();
            continue;
        }
    }

    std::vector<ServerItem*> itemsCopy = items_;

    // See what needs to be removed
    for (auto it = itemsCopy.begin(); it != itemsCopy.end(); ++it) {
        if ((*it)->isSystem()) {
            bool found = false;
            for (auto& i : sysVec) {
                if (i.name() == (*it)->name()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                changed = true;
                ServerListTmpItem localTmp(*it);
                changeVec.push_back(
                    new ServerListSyncChangeItem(localTmp, localTmp, ServerListSyncChangeItem::UnsetSysChange));
                // remove item
                remove(*it);
            }
        }
    }

    if (changed)
        save();

    if (needBrodcast)
        broadcastChanged();
}

//===========================================================
// Observers
//===========================================================

void ServerList::addObserver(ServerListObserver* o) {
    auto it = std::find(observers_.begin(), observers_.end(), o);
    if (it == observers_.end()) {
        observers_.push_back(o);
    }
}

void ServerList::removeObserver(ServerListObserver* o) {
    auto it = std::find(observers_.begin(), observers_.end(), o);
    if (it != observers_.end()) {
        observers_.erase(it);
    }
}

void ServerList::broadcastChanged() {
    for (auto it = observers_.begin(); it != observers_.end(); ++it)
        (*it)->notifyServerListChanged();
}
