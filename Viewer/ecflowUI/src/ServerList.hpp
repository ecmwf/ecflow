//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERLIST_HPP_
#define SERVERLIST_HPP_

#include <string>
#include <vector>

#include <QDateTime>

#include "GenFileProvider.hpp"
#include "VProperty.hpp"
#include "PropertyMapper.hpp"

class ServerItem;
class ServerList;
class GenFileReceiver;


class ServerListObserver
{
public:
    ServerListObserver() = default;
    virtual ~ServerListObserver() = default;
	virtual void notifyServerListChanged()=0;
	virtual void notifyServerListFavouriteChanged(ServerItem*)=0;
};

class ServerListTmpItem
{
public:
    ServerListTmpItem() = default;
    ServerListTmpItem(const std::string& name,const std::string& host, const std::string& port) :
        name_(name), host_(host), port_(port) {}
    explicit ServerListTmpItem(ServerItem* item);
    ServerListTmpItem(const ServerListTmpItem& ) = default;

    const std::string& name() const {return name_;}
    const std::string& host() const {return host_;}
    const std::string& port() const {return port_;}

protected:
    std::string name_;
    std::string host_;
    std::string port_;
};

class ServerListSyncChangeItem
{
public:
    enum ChangeType {AddedChange,RemovedChange,MatchChange,SetSysChange,UnsetSysChange};

    ServerListSyncChangeItem(const ServerListTmpItem& sys,const ServerListTmpItem& local,ChangeType type) :
       sys_(sys), local_(local), type_(type) {}

    const ServerListTmpItem& sys() const {return sys_;}
    const ServerListTmpItem& local() const {return local_;}
    ChangeType type() const {return type_;}

    ServerListTmpItem sys_;
    ServerListTmpItem local_;
    ChangeType type_;
};

class ServerListSystemFileManager: public GenFileReceiver, public VPropertyObserver
{
public:
    ServerListSystemFileManager() = default;
    ~ServerListSystemFileManager();

    bool hasSyncChange() const {return !changedItems_.empty();}
    bool hasInfo() const {return !changedItems_.empty() || !unfetchedFiles_.empty();}
    QDateTime syncDate() const {return syncDate_;}
    const std::vector<std::string>& files() const {return files_;}
    const std::vector<std::string>& fetchedFiles() const {return fetchedFiles_;}
    const std::vector<std::string>& unfetchedFiles() const {return unfetchedFiles_;}
    const std::vector<ServerListSyncChangeItem*>&  changedItems() const {return changedItems_;}

    void sync();
    void clearChangeInfo();
    void fileFetchFinished(VReply*) override;
    void fileFetchFailed(VReply*) override;
    void notifyChange(VProperty*) override;

protected:
    void clear();
    std::vector<std::string> buildFileList();
    bool fileListSameAs(const std::vector<std::string>& v1, const std::vector<std::string>& v2) const;
    void syncInternal(const std::vector<std::string>& newFiles);
    void concludeSync();
    void delayedFetchFiles(const std::vector<std::string>& paths);
    void loadFiles(const std::vector<std::string>& paths);
    void loadFile(const std::string& fPath, std::vector<ServerListTmpItem>& sysVec,
                                            std::vector<std::string>& includedPaths);
    void loadPending();

    enum State {EmptyState, FetchState, SyncedState};
    std::vector<std::string> files_;
    std::vector<std::string> fetchedFiles_;
    std::vector<std::string> unfetchedFiles_;
    std::vector<ServerListSyncChangeItem*> changedItems_;
    std::vector<ServerListTmpItem> pendingItems_;
    QDateTime syncDate_;
    GenFileReceiver* fileProvider_{nullptr};
    PropertyMapper* prop_{nullptr};
    State state_{EmptyState};
};

class ServerList
{
    friend class ServerListSystemFileManager;

public:
    int count() const {return static_cast<int>(items_.size());}
	ServerItem* itemAt(int);
	ServerItem* find(const std::string& name);
	ServerItem* find(const std::string& name, const std::string& host, const std::string& port);

	//Can be added or changed only via these static methods
    ServerItem* add(const std::string& name,const std::string& host,const std::string& port, const std::string& user,
                    bool favorite, bool ssl, bool saveIt);
	void remove(ServerItem*);
    ServerItem* reset(ServerItem*,const std::string& name,const std::string& host,const std::string& port,
               const std::string& user, bool ssl);
	void setFavourite(ServerItem*,bool);

	std::string uniqueName(const std::string&);

    void init();
	void save();
	void rescan();
    void syncSystemFiles();
    ServerListSystemFileManager* systemFileManager() const {return sysFileManager_;}

	void addObserver(ServerListObserver*);
	void removeObserver(ServerListObserver*);

	static ServerList* instance();

protected:
    ServerList();
    ~ServerList();

	static ServerList* instance_;

    bool load();
    bool checkItemToAdd(const std::string& name,const std::string& host,const std::string& port,
                        bool checkDuplicate,std::string& errStr);
    void loadSystemItems(const std::vector<ServerListTmpItem>& sysVec,
                         std::vector<ServerListSyncChangeItem*>& changeVec);
	void broadcastChanged();
	void broadcastChanged(ServerItem*);

	std::vector<ServerItem*> items_;
    std::string localFile_;
	std::vector<ServerListObserver*> observers_;
    ServerListSystemFileManager* sysFileManager_{nullptr};
};

#endif
