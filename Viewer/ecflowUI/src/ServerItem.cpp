/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ServerItem.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ServerHandler.hpp"

ServerItem::ServerItem(const std::string& name) : name_(name) {
}

ServerItem::ServerItem(const std::string& name,
                       const std::string& host,
                       const std::string& port,
                       const std::string& user,
                       bool favourite,
                       bool ssl)
    : name_(name),
      host_(host),
      port_(port),
      user_(user),
      favourite_(favourite),
      ssl_(ssl) {
}

ServerItem::~ServerItem() {
    broadcastDeletion();

    if (handler_)
        ServerHandler::removeServer(handler_);
}

bool ServerItem::isUsed() const {
    return (handler_ != nullptr);
}

void ServerItem::reset(const std::string& name,
                       const std::string& host,
                       const std::string& port,
                       const std::string& user,
                       bool ssl) {
    if (name_ != name) {
        name_ = name;
        if (handler_) {
            handler_->rename(name_);
        }
    }

    if (host == host_ && port == port_) {
        // TODO: these should be called together
        setSsl(ssl);
        setUser(user);
    }

    // host or port changed: full reload needed and this situation cannot
    //  be handled here!
    else {
        assert(host_ == host);
        assert(port_ == port);
    }

    broadcastChanged();
}

void ServerItem::setFavourite(bool b) {
    favourite_ = b;
    broadcastChanged();
}

void ServerItem::setSystem(bool b) {
    system_ = b;
    if (isUsed()) {}
    // broadcastChanged();
}

void ServerItem::setSsl(bool b) {
    if (ssl_ != b) {
        ssl_ = b;
        if (handler_)
            handler_->setSsl(ssl_);
    }
    // broadcastChanged();
}

void ServerItem::setUser(const std::string& user) {
    if (user_ != user) {
        user_ = user;
        if (handler_)
            handler_->setUser(user_);
    }
    // broadcastChanged();
}

std::string ServerItem::longName() const {
    return host_ + "@" + port_;
}

//===========================================================
// Register the usage of the server. Create and destroys the
// the ServerHandler.
//===========================================================

void ServerItem::registerUsageBegin() {
    if (!handler_) {
        handler_ = ServerHandler::addServer(name_, host_, port_, user_, ssl_);
    }
    if (handler_)
        useCnt_++;
}

void ServerItem::registerUsageEnd() {
    useCnt_--;
    if (useCnt_ == 0 && handler_) {
        ServerHandler::removeServer(handler_);
        handler_ = nullptr;
    }
}

//===========================================================
// Observers
//===========================================================

void ServerItem::addObserver(ServerItemObserver* o) {
    auto it = std::find(observers_.begin(), observers_.end(), o);
    if (it == observers_.end()) {
        registerUsageBegin();
        // We might not be able to create the handle
        if (handler_)
            observers_.push_back(o);
    }
}

void ServerItem::removeObserver(ServerItemObserver* o) {
    auto it = std::find(observers_.begin(), observers_.end(), o);
    if (it != observers_.end()) {
        observers_.erase(it);
        registerUsageEnd();
    }
}

void ServerItem::broadcastChanged() {
    for (auto it = observers_.begin(); it != observers_.end(); ++it)
        (*it)->notifyServerItemChanged(this);
}

void ServerItem::broadcastDeletion() {
    std::vector<ServerItemObserver*> obsCopy = observers_;

    for (auto it = obsCopy.begin(); it != obsCopy.end(); ++it)
        (*it)->notifyServerItemDeletion(this);
}
