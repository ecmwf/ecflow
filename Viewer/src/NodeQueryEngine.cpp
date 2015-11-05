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

static bool metaRegistered=false;

NodeQueryEngine::NodeQueryEngine(QObject* parent) :
	QThread(parent),
	query_(NULL),
	parser_(NULL)
{
	//We will need to pass various non-Qt types via signals and slots
	//So we need to register these types.
	if(!metaRegistered)
	{
		qRegisterMetaType<NodeQueryResultData>("NodeQueryResultData");
		metaRegistered=true;
	}
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

		broadcastFind(node);
	}

	for(int i=0; i < node->numOfChildren(); i++)
	{
		runRecursively(node->childAt(i));
	}
}

void NodeQueryEngine::broadcastFind(VNode* node)
{
	NodeQueryResultData d;

	QStringList lst;
	if(node->server())
		d.server_=QString::fromStdString(node->server()->name());

	d.path_=QString::fromStdString(node->absNodePath());
	d.state_=node->stateName();
	//d.state_.truncate(2);
	d.stateCol_=node->stateColour();
	d.type_=QString::fromStdString(node->nodeType());
	//d.type_.truncate(1);

	Q_EMIT found(d);
}








