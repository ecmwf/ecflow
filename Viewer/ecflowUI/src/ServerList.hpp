//============================================================================
// Copyright 2009-2019 ECMWF.
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

class ServerItem;
class ServerList;


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
    enum ChangeType {AddedChange,RemovedChange, MatchChange, SetSysChange,UnsetSysChange};

    ServerListSyncChangeItem(const ServerListTmpItem& sys,const ServerListTmpItem& local,ChangeType type) :
       sys_(sys), local_(local), type_(type) {}

    const ServerListTmpItem& sys() const {return sys_;}
    const ServerListTmpItem& local() const {return local_;}
    ChangeType type() const {return type_;}

    ServerListTmpItem sys_;
    ServerListTmpItem local_;
    ChangeType type_;
};

class ServerList
{
public:
    int count() const {return static_cast<int>(items_.size());}
	ServerItem* itemAt(int);
	ServerItem* find(const std::string& name);
	ServerItem* find(const std::string& name, const std::string& host, const std::string& port);

	//Can be added or changed only via these static methods
	ServerItem* add(const std::string&,const std::string&,const std::string&,bool,bool saveIt=true);
	void remove(ServerItem*);
	void reset(ServerItem*,const std::string& name,const std::string& host,const std::string& port);
	void setFavourite(ServerItem*,bool);

	std::string uniqueName(const std::string&);

    void init();
	void save();
	void rescan();
    void syncSystemFile();
    bool hasSystemFile() const;
    const std::vector<ServerListSyncChangeItem*>&  syncChange() const {return syncChange_;}
    bool hasSyncChange() const {return !syncChange_.empty();}
    QDateTime syncDate() const {return syncDate_;}

	void addObserver(ServerListObserver*);
	void removeObserver(ServerListObserver*);

	static ServerList* instance();

protected:
    ServerList() = default;
    ~ServerList() = default;

	static ServerList* instance_;

	bool load();
	bool readRcFile();
    //bool readSystemFile();
    void clearSyncChange();
    bool checkItemToAdd(const std::string& name,const std::string& host,const std::string& port,
                        bool checkDuplicate,std::string& errStr);

	void broadcastChanged();
	void broadcastChanged(ServerItem*);

	std::vector<ServerItem*> items_;
    std::string localFile_;
    std::string systemFile_;
	std::vector<ServerListObserver*> observers_;
    std::vector<ServerListSyncChangeItem*> syncChange_;
    QDateTime syncDate_;
};



#endif
