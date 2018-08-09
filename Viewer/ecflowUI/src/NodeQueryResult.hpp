//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_NODEQUERYRESULT_HPP_
#define VIEWER_SRC_NODEQUERYRESULT_HPP_

#include <QColor>
#include <QHash>
#include <QObject>
#include <QString>

#include "NodeObserver.hpp"
#include "NodeQueryResultTmp.hpp"
#include "ServerObserver.hpp"
#include "VInfo.hpp"

class ServerHandler;
class VNode;

class NodeQueryResultItem
{
	friend class NodeQueryEngine;
	friend class NodeQueryResult;
	friend class NodeQueryResultModel;
	friend class TriggerGraphModel;

public:
	NodeQueryResultItem() = default;
	NodeQueryResultItem(VNode* node);
	NodeQueryResultItem(NodeQueryResultTmp_ptr);

	void invalidateNode();
	bool updateNode();

	QString serverStr() const;
	QString pathStr() const;
	QString typeStr() const;
	QString stateStr() const;
	QColor stateColour() const;
    QString stateChangeTimeAsString() const;
    unsigned int stateChangeTime() const;
    QStringList attr() const;
    bool hasAttribute() const {return attr_.count() > 0;}

protected:
	VNode* node_{nullptr};
	ServerHandler* server_{nullptr};
	QStringList attr_;
	std::string path_;
};

struct Pos
{
	Pos()= default;
	int pos_{-1};
	int cnt_{0};
};

struct NodeQueryResultBlock : public Pos
{
	NodeQueryResultBlock()= default;
	void add(VNode*,int);
	void clear();
	bool find(const VNode* node,int &pos, int &cnt);

	ServerHandler* server_{nullptr};
	QHash<VNode*,Pos> nodes_;
};


class NodeQueryResult : public QObject, public ServerObserver, public NodeObserver
{
 Q_OBJECT

public:
 	explicit NodeQueryResult(QObject* parent=nullptr);
 	~NodeQueryResult() override;

 	int size() const {return data_.size();}
 	NodeQueryResultItem* itemAt(int i);
 	void clear();

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
    void add(std::vector<VInfo_ptr>);
    
    
public Q_SLOTS:
 	void add(NodeQueryResultTmp_ptr);
 	void add(QList<NodeQueryResultTmp_ptr> items);

Q_SIGNALS:
     void beginAppendRow();
     void endAppendRow();
     void beginAppendRows(int);
     void endAppendRows(int);
     void beginRemoveRow(int);
     void endRemoveRow(int);
     void beginRemoveRows(int,int);
     void endRemoveRows(int,int);
     void beginReset();
     void endReset();
     void stateChanged(const VNode*,int,int);

protected:
     void clearData(bool hideOnly);
     void clear(ServerHandler*);
     void serverClear(ServerHandler*);
     void serverScan(ServerHandler*);
     void attach(ServerHandler*);
     void detach(ServerHandler*);
     bool range(const VNode*,int&,int&);
     //void detach(VNode*);

     std::vector<NodeQueryResultItem*> data_;
     //std::map<ServerHandler*,int> serverCnt_;
     std::map<ServerHandler*,NodeQueryResultBlock> blocks_;
};



#endif /* VIEWER_SRC_NODEQUERYRESULT_HPP_ */
