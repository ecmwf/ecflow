//============================================================================
// Copyright 2014 ECMWF.
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
	VNodeListItem(VNode* n);
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
 	explicit VNodeList(QObject* parent=0);
 	~VNodeList();

 	int size() const {return data_.size();}
 	VNodeListItem* itemAt(int i);
 	void add(VNode*);
 	void remove(VNode*);
 	void clear();
 	bool contains(VNode*);

    //From ServerObserver
 	void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) {};
 	void notifyServerDelete(ServerHandler* server);
    void notifyBeginServerClear(ServerHandler* server);
 	void notifyEndServerClear(ServerHandler* server);
 	void notifyBeginServerScan(ServerHandler* server,const VServerChange&);
    void notifyEndServerScan(ServerHandler* server);

 	//From NodeObserver
    void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
 	void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);

Q_SIGNALS:
     void beginAppendRow();
     void endAppendRow();
     void beginRemoveRow(int);
     void endRemoveRow(int);
     void beginReset();
     void endReset();

protected:
     void clearData(bool hideOnly);
     void clear(ServerHandler*);
     void serverClear(ServerHandler*);
     void serverScan(ServerHandler*);
     void attach(ServerHandler*);
     void detach(ServerHandler*);

     std::vector<VNodeListItem*> data_;
     std::map<ServerHandler*,int> serverCnt_;
};

#endif /* VIEWER_SRC_VNODELIST_HPP_ */
