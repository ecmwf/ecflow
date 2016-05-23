//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_NODEQUERYENGINE_HPP_
#define VIEWER_SRC_NODEQUERYENGINE_HPP_

#include <string>
#include <vector>

#include <QStringList>
#include <QThread>

#include "NodeQueryResultTmp.hpp"

class BaseNodeCondition;
class ServerHandler;
class VNode;
class NodeFilter;
class NodeQuery;
class NodeQueryOptions;
class VAttribute;
class VAttributeType;

class NodeQueryEngine : public QThread
{

Q_OBJECT

public:
	explicit NodeQueryEngine(QObject* parent=0);
	~NodeQueryEngine();

    bool runQuery(NodeQuery* query,QStringList allServers);
	void stopQuery();
	int scannedCount() const {return scanCnt_;}

protected Q_SLOTS:
	void slotFinished();
	void slotFailed();

Q_SIGNALS:
	void found(NodeQueryResultTmp_ptr);
	void found(QList<NodeQueryResultTmp_ptr>);

protected:
	void run();

private:
	void run(ServerHandler*,VNode*);
	void runRecursively(VNode *node);
    void broadcastFind(VNode*);
    void broadcastFind(VNode*,QStringList);
	void broadcastChunk(bool);

	NodeQuery* query_;
    BaseNodeCondition* parser_;
    BaseNodeCondition* attrParser_;
    std::vector<ServerHandler*> servers_;
    QList<VAttributeType*> attrTypes_;
	int cnt_;
	int scanCnt_;
	int maxNum_;
	int chunkSize_;
	QList<NodeQueryResultTmp_ptr> res_;
	bool stopIt_;
	VNode* rootNode_;

};

class NodeFilterEngine
{
public:
	explicit NodeFilterEngine(NodeFilter*);
	~NodeFilterEngine();

	void runQuery(ServerHandler*);
	void setQuery(NodeQuery*);

private:
	void runRecursively(VNode *node);

	NodeQuery* query_;
	BaseNodeCondition* parser_;
	ServerHandler* server_;
	NodeFilter *owner_;
};


#endif /* VIEWER_SRC_NODEQUERYENGINE_HPP_ */
