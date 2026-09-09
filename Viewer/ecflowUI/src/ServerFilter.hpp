/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ServerFilter_HPP
#define ecflow_viewer_ServerFilter_HPP

#include <vector>

#include "ServerItem.hpp"
#include "ecflow/node/Node.hpp"

class VSettings;

class ServerFilterObserver {
public:
    virtual ~ServerFilterObserver()                     = default;
    virtual void notifyServerFilterAdded(ServerItem*)   = 0;
    virtual void notifyServerFilterRemoved(ServerItem*) = 0;
    virtual void notifyServerFilterChanged(ServerItem*) = 0;
    virtual void notifyServerFilterDelete()             = 0;
};

class ServerFilter : public ServerItemObserver {
public:
    ServerFilter();
    ~ServerFilter() override;

    enum ChangeAspect { Reset, Added, Removed };

    const std::vector<ServerItem*>& items() const { return items_; }
    int itemCount() const { return static_cast<int>(items_.size()); }
    void serverNames(std::vector<std::string>&) const;

    void addServer(ServerItem*, bool broadcast = true);
    void removeServer(ServerItem*);
    bool isFiltered(ServerItem*) const;
    bool isFiltered(ServerHandler*) const;
    bool isFiltered(const std::string& serverName) const;

    void writeSettings(VSettings*) const;
    void readSettings(VSettings*);

    void addObserver(ServerFilterObserver*);
    void removeObserver(ServerFilterObserver*);

    // From ServerItemObserver
    void notifyServerItemChanged(ServerItem*) override;
    void notifyServerItemDeletion(ServerItem*) override;

protected:
    void broadcastAdd(ServerItem*);
    void broadcastRemove(ServerItem*);
    void broadcastChange(ServerItem*);

private:
    std::vector<ServerItem*> items_;
    std::vector<ServerFilterObserver*> observers_;
};

#endif /* ecflow_viewer_ServerFilter_HPP */
