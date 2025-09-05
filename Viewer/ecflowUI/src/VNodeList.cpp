/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VNodeList.hpp"

#include <QDateTime>

#include "ServerHandler.hpp"
#include "VNode.hpp"

//========================================================
//
//  VNodeListItem
//
//========================================================

VNodeListItem::VNodeListItem(VNode* n) : node_(n) {
    if (n) {
        server_ = n->server();
        path_   = n->absNodePath();
        time_   = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    }
}

const std::string& VNodeListItem::serverName() const {
    if (server_) {
        return server_->name();
    }
    static std::string emptyStr;
    return emptyStr;
}

bool VNodeListItem::sameAs(VNode* node) const {
    if (!node) {
        return false;
    }

    if (node_) {
        return (node_ == node);
    }

    return (server_ == node->server() && path_ == node->absNodePath());
}

void VNodeListItem::invalidateNode() {
    node_ = nullptr;
}

bool VNodeListItem::updateNode(ServerHandler* s) {
    if (node_) {
        return (node_->server() == s && server_ == s);
    }
    else if (s == server_) {
        node_ = s->vRoot()->find(path_);
        return (node_ != nullptr);
    }

    return false;
}

//========================================================
//
//  VNodeList
//
//========================================================

VNodeList::VNodeList(QObject* parent) : QObject(parent) {
}

VNodeList::~VNodeList() {
    clear();
}

VNodeListItem* VNodeList::itemAt(int i) {
    if (i >= 0 && i < static_cast<int>(data_.size())) {
        return data_.at(i);
    }

    return nullptr;
}

void VNodeList::setMaxNum(int maxNum) {
    if (maxNum_ != maxNum) {
        assert(maxNum > 0);
        maxNum_ = maxNum;
        trim();
    }
}

void VNodeList::add(VNode* node) {
    if (contains(node)) {
        return;
    }

    ServerHandler* s = node->server();
    if (!s) {
        return;
    }

    attach(s);

    Q_EMIT beginAppendRow();
    data_.push_back(new VNodeListItem(node));
    serverCnt_[s]++;
    Q_EMIT endAppendRow();

    trim();
}

void VNodeList::remove(VNode* node) {
    if (!node) {
        return;
    }

    for (auto it = data_.begin(); it != data_.end(); it++) {
        if ((*it)->sameAs(node)) {
            int row = it - data_.begin();

            Q_EMIT beginRemoveRow(row);
            delete *it;
            data_.erase(it);

            detach(node);

            Q_EMIT endRemoveRow(row);
            return;
        }
    }
}

void VNodeList::trim() {
    int cnt     = data_.size();
    bool doTrim = (cnt > 0 && cnt > maxNum_);

    if (!doTrim) {
        return;
    }

    Q_EMIT beginRemoveRows(0, cnt - maxNum_ - 1);

    for (int row = cnt - 1; row >= maxNum_; row--) {
        VNode* node = data_.front()->node();

        delete data_.front();

        if (node) {
            detach(node);
        }

        data_.erase(data_.begin());
    }

    Q_EMIT endRemoveRows(0, cnt - maxNum_ - 1);
}

bool VNodeList::contains(VNode* node) {
    for (auto it = data_.begin(); it != data_.end(); ++it) {
        if ((*it)->sameAs(node)) {
            return true;
        }
    }

    return false;
}

void VNodeList::clear() {
    Q_EMIT beginReset();

    for (auto it = serverCnt_.begin(); it != serverCnt_.end(); ++it) {
        it->first->removeServerObserver(this);
        it->first->removeNodeObserver(this);
    }
    serverCnt_.clear();

    for (auto it = data_.begin(); it != data_.end(); ++it) {
        delete *it;
    }

    data_.clear();

    Q_EMIT endReset();
}

// remove all items related to server
void VNodeList::clear(ServerHandler* server) {
    std::vector<VNodeListItem*> prev = data_;
    data_.clear();

    detach(server);

    for (auto it = prev.begin(); it != prev.end(); ++it) {
        if ((*it)->server() == server) {
            delete *it;
        }
        else {
            data_.push_back(*it);
        }
    }
}

// temporarily disables all items related server
void VNodeList::serverClear(ServerHandler* server) {
    for (auto it = data_.begin(); it != data_.end(); ++it) {
        if (server == (*it)->server()) {
            (*it)->invalidateNode();
        }
    }
}

void VNodeList::serverScan(ServerHandler* server) {
    std::vector<VNodeListItem*> prev = data_;
    data_.clear();

    serverCnt_[server] = 0;

    for (auto it = prev.begin(); it != prev.end(); ++it) {
        if (server == (*it)->server()) {
            if ((*it)->updateNode(server)) {
                data_.push_back(*it);
                serverCnt_[server]++;
            }
            else {
                delete *it;
            }
        }
        else {
            data_.push_back(*it);
        }
    }

    if (serverCnt_[server] == 0) {
        detach(server);
    }
}

void VNodeList::attach(ServerHandler* s) {
    if (serverCnt_.find(s) == serverCnt_.end()) {
        s->addServerObserver(this);
        s->addNodeObserver(this);
        serverCnt_[s] = 0;
    }
}

void VNodeList::detach(ServerHandler* s) {
    auto it = serverCnt_.find(s);
    if (it != serverCnt_.end()) {
        serverCnt_.erase(it);
        s->removeServerObserver(this);
        s->removeNodeObserver(this);
    }
}

void VNodeList::detach(VNode* node) {
    auto it = serverCnt_.find(node->server());
    if (it != serverCnt_.end()) {
        it->second--;
        if (it->second == 0) {
            detach(node->server());
        }
    }
}

void VNodeList::notifyServerDelete(ServerHandler* server) {
    Q_EMIT beginReset();
    clear(server);
    Q_EMIT endReset();
}

void VNodeList::notifyBeginServerClear(ServerHandler* server) {
    serverClear(server);
}

void VNodeList::notifyEndServerClear(ServerHandler*) {
}

void VNodeList::notifyBeginServerScan(ServerHandler*, const VServerChange&) {
    Q_EMIT beginReset();
}

void VNodeList::notifyEndServerScan(ServerHandler* server) {
    serverScan(server);
    Q_EMIT endReset();
}

void VNodeList::notifyServerRenamed(ServerHandler*, const std::string& /*oldName*/) {
    Q_EMIT beginReset();
    Q_EMIT endReset();
}

void VNodeList::notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&, const VNodeChange&) {
}

void VNodeList::notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&, const VNodeChange&) {
}
