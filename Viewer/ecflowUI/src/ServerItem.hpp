/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_ServerItem_HPP
#define ecflow_viewer_ServerItem_HPP

#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ecflow/base/ServerProtocol.hpp"

class ServerHandler;
class ServerItem;

class ServerItemObserver {
public:
    ServerItemObserver()                               = default;
    virtual ~ServerItemObserver()                      = default;
    virtual void notifyServerItemChanged(ServerItem*)  = 0;
    virtual void notifyServerItemDeletion(ServerItem*) = 0;
};

class ServerItem {

    friend class ServerList;

public:
    const std::string& name() const { return name_; }
    const std::string& host() const { return host_; }
    const std::string& port() const { return port_; }
    const std::string& user() const { return user_; }
    std::string longName() const;
    bool isFavourite() const { return favourite_; }
    bool isSystem() const { return system_; }
    ecf::Protocol protocol() const { return protocol_; }
    bool isSsl() const { return protocol_ == ecf::Protocol::Ssl; }
    bool isHttp() const { return protocol_ == ecf::Protocol::Http; }
    bool isHttps() const { return protocol_ == ecf::Protocol::Https; }

    bool isUsed() const;
    int useCnt() const { return useCnt_; }
    ServerHandler* serverHandler() const { return handler_; }

    void addObserver(ServerItemObserver*);
    void removeObserver(ServerItemObserver*);

protected:
    explicit ServerItem(const std::string&);
    ServerItem(const std::string& name,
               const std::string& host,
               const std::string& port,
               const std::string& user,
               bool favourite,
               ecf::Protocol protocol);
    ~ServerItem();

    void name(const std::string& name) { name_ = name; }
    void host(const std::string& host) { host_ = host; }
    void port(const std::string& port) { port_ = port; }
    void reset(const std::string& name,
               const std::string& host,
               const std::string& port,
               const std::string& user,
               ecf::Protocol protocol);
    void setFavourite(bool b);
    void setSystem(bool b);
    void setProtocol(ecf::Protocol protocol);
    void setUser(const std::string&);

    void registerUsageBegin();
    void registerUsageEnd();

    void broadcastChanged();
    void broadcastDeletion();

    std::string name_;
    std::string host_;
    std::string port_;
    std::string user_;
    bool favourite_{false};
    bool system_{false};
    ecf::Protocol protocol_{ecf::Protocol::Plain};
    int useCnt_{0};
    ServerHandler* handler_{nullptr};

    std::vector<ServerItemObserver*> observers_;
};

typedef std::shared_ptr<ServerItem> ServerItem_ptr;

#endif /* ecflow_viewer_ServerItem_HPP */
