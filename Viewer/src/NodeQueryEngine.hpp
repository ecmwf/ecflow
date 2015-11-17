//============================================================================
// Copyright 2014 ECMWF.
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

#include "NodeQueryResultData.hpp"

class BaseNodeCondition;
class ServerHandler;
class VNode;
class NodeFilter;
class NodeQuery;
class NodeQueryOptions;

class NodeQueryEngine : public QThread
{

Q_OBJECT

public:
	explicit NodeQueryEngine(QObject* parent=0);
	~NodeQueryEngine();

	void runQuery(NodeQuery* query);
	void stopQuery();
	int scannedCount() const {return scanCnt_;}

Q_SIGNALS:
	void found(NodeQueryResultData);
	void found(QList<NodeQueryResultData>);

protected:
	void run();

private:
	void run(ServerHandler*,VNode*);
	void runRecursively(VNode *node);
	void broadcastFind(VNode*);
	void broadcastChunk(bool);

	NodeQuery* query_;
	BaseNodeCondition* parser_;
	std::vector<ServerHandler*> servers_;
	int cnt_;
	int scanCnt_;
	int maxNum_;
	int chunkSize_;
	QList<NodeQueryResultData> res_;
	bool stopIt_;

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
