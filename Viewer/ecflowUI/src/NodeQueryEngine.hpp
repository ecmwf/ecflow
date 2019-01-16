//============================================================================
// Copyright 2009-2019 ECMWF.
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

#include <QMap>
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
	explicit NodeQueryEngine(QObject* parent=nullptr);
	~NodeQueryEngine() override;

    bool runQuery(NodeQuery* query,QStringList allServers);
	void stopQuery();
	int scannedCount() const {return scanCnt_;}
    bool wasStopped() const {return stopIt_;}
    bool wasMaxReached() const {return maxReached_;}

protected Q_SLOTS:
	void slotFinished();
	void slotFailed();

Q_SIGNALS:
	void found(NodeQueryResultTmp_ptr);
	void found(QList<NodeQueryResultTmp_ptr>);

protected:
	void run() override;

private:
	void run(ServerHandler*,VNode*);
	void runRecursively(VNode *node);
    void broadcastFind(VNode*);
    void broadcastFind(VNode*,QStringList);
	void broadcastChunk(bool);

	NodeQuery* query_;
    BaseNodeCondition* parser_{nullptr};
    QMap<VAttributeType*,BaseNodeCondition*> attrParser_;
    std::vector<ServerHandler*> servers_;
	int cnt_{0};
	int scanCnt_{0};
	int maxNum_{250000};
	int chunkSize_{100};
	QList<NodeQueryResultTmp_ptr> res_;
	bool stopIt_{false};
    bool maxReached_{false};
	VNode* rootNode_{nullptr};
};

class NodeFilterEngine
{
public:
	explicit NodeFilterEngine(NodeFilter*);
	~NodeFilterEngine();

    bool runQuery(ServerHandler*);
	void setQuery(NodeQuery*);

private:
	void runRecursively(VNode *node);

	NodeQuery* query_;
	BaseNodeCondition* parser_;
	ServerHandler* server_;
	NodeFilter *owner_;
    VNode* rootNode_;
};


#endif /* VIEWER_SRC_NODEQUERYENGINE_HPP_ */
