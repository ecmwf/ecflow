//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQueryEngine.hpp"

#include <QStandardItemModel>

#include "NodeExpression.hpp"
#include "NodeQuery.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"


NodeQueryEngine::NodeQueryEngine(QObject* parent) :
	QThread(parent),
	query_(NULL),
	parser_(NULL)
{
}

NodeQueryEngine::~NodeQueryEngine()
{
	if(query_)
		delete query_;
}

void NodeQueryEngine::exec(NodeQuery* query)
{
	if(isRunning())
		wait();

	if(query_)
	{
		delete query_;
		query_=NULL;
	}

	query_=query->clone();

	servers_.clear();

	if(parser_)
		delete parser_;

	parser_=NodeExpressionParser::parseWholeExpression(query_->query());
	if(parser_ == NULL)
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse enabled condition: " + query_->query()));
		return;
	}

	for(std::vector<std::string>::const_iterator it=query_->servers().begin(); it != query_->servers().end(); ++it)
	{
		if(ServerHandler* server=ServerHandler::find(*it))
			servers_.push_back(server);
	}

	//Start thread execution
	start();
}

void NodeQueryEngine::exec(const NodeQuery& query,NodeFilter* filter)
{
	/*query_=query;

	if(parser_)
		delete parser_;

	parser_=NodeExpressionParser::parseWholeExpression(query_.query());
	if(parser_ == NULL)
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse enabled condition: " + query_.query()));
		return;
	}*/
	//run(server,server->vRoot());
}


void NodeQueryEngine::run()
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end(); ++it)
	{
		ServerHandler *server=*it;

		//Set the mutex on the server defs. We do not allow sycn while the
		//search is running.
		//TODO: avoid blocking the main (gui) thread
		//ServerDefsAccess defs(server);

		run(server,server->vRoot());
	}
}

void NodeQueryEngine::run(ServerHandler *server,VNode* root)
{
	runRecursively(root);
}

void NodeQueryEngine::runRecursively(VNode *node)
{
	if(parser_->execute(node))
	{
		UserMessage::message(UserMessage::DBG,false,"FOUND: " + node->absNodePath());

		QStringList  lst;
		lst << QString::fromStdString(node->server()->name());
		lst << QString::fromStdString(node->absNodePath());
		Q_EMIT found(lst);
	}

	for(int i=0; i < node->numOfChildren(); i++)
	{
		runRecursively(node->childAt(i));
	}
}
