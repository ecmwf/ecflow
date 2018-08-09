//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_VNODELIST_HPP_
#define VIEWER_SRC_VNODELIST_HPP_

#include "ServerObserver.hpp"
#include "NodeObserver.hpp"

#include <QObject>

#include <map>
#include <vector>

class VNodeList;

class VNodeListItem
{
friend class VNodeList;

public:
	explicit VNodeListItem(VNode* n);
	VNode *node() const {return node_;}
	const std::string& server() const {return server_;}
	const std::string& path() const {return path_;}
	QString time() const {return time_;}
	bool sameAs(VNode *node) const;
	void invalidateNode();
	bool updateNode(ServerHandler*);

protected:
	VNode* node_;
	std::string server_;
	std::string path_;
	QString time_;
};

class VNodeList : public QObject, public ServerObserver, public NodeObserver
{
 Q_OBJECT

public:
 	explicit VNodeList(QObject* parent=nullptr);
 	~VNodeList() override;

 	int size() const {return data_.size();}
 	VNodeListItem* itemAt(int i);
 	void add(VNode*);
 	void remove(VNode*);
 	void clear();
 	bool contains(VNode*);
 	void setMaxNum(int);

    //From ServerObserver
    void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) override {}
 	void notifyServerDelete(ServerHandler* server) override;
    void notifyBeginServerClear(ServerHandler* server) override;
 	void notifyEndServerClear(ServerHandler* server) override;
 	void notifyBeginServerScan(ServerHandler* server,const VServerChange&) override;
    void notifyEndServerScan(ServerHandler* server) override;

 	//From NodeObserver
    void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&) override;
 	void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&) override;

Q_SIGNALS:
     void beginAppendRow();
     void endAppendRow();
     void beginRemoveRow(int);
     void endRemoveRow(int);
     void beginRemoveRows(int,int);
     void endRemoveRows(int,int);
     void beginReset();
     void endReset();

protected:
     void clearData(bool hideOnly);
     void clear(ServerHandler*);
     void serverClear(ServerHandler*);
     void serverScan(ServerHandler*);
     void attach(ServerHandler*);
     void detach(ServerHandler*);
     void detach(VNode*);
     void trim();

     std::vector<VNodeListItem*> data_;
     std::map<ServerHandler*,int> serverCnt_;
     int maxNum_{200};
};

#endif /* VIEWER_SRC_VNODELIST_HPP_ */
