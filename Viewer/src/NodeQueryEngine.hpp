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

	void exec(NodeQuery* query);
	void exec(const NodeQuery& query,NodeFilter* filter);

Q_SIGNALS:
	void found(NodeQueryResultData);

protected:
	void run();

private:
	void run(ServerHandler*,VNode*);
	void runRecursively(VNode *node);
	void broadcastFind(VNode*);

	NodeQuery* query_;
	BaseNodeCondition* parser_;
	std::vector<ServerHandler*> servers_;
	std::vector<std::string> res_;
};

#endif /* VIEWER_SRC_NODEQUERYENGINE_HPP_ */
