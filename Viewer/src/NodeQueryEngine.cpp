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
#include "VFilter.hpp"
#include "VNode.hpp"

static bool metaRegistered=false;

NodeQueryEngine::NodeQueryEngine(QObject* parent) :
	QThread(parent),
	query_(new NodeQuery("tmp")),
	parser_(NULL),
	stopIt_(false),
	cnt_(0),
	scanCnt_(0),
	maxNum_(250000),
	chunkSize_(100),
	rootNode_(0)
{
	//We will need to pass various non-Qt types via signals and slots
	//So we need to register these types.
	if(!metaRegistered)
	{
		qRegisterMetaType<NodeQueryResultTmp_ptr>("NodeQueryResultTmp_ptr");
		qRegisterMetaType<QList<NodeQueryResultTmp_ptr> >("QList<NodeQueryResultTmp_ptr>");
		metaRegistered=true;
	}

	connect(this,SIGNAL(finished()),
			this,SLOT(slotFinished()));
}

NodeQueryEngine::~NodeQueryEngine()
{
	delete query_;

	if(parser_)
		delete parser_;
}

void NodeQueryEngine::runQuery(NodeQuery* query,QStringList allServers)
{
	if(isRunning())
		wait();

	stopIt_=false;
	res_.clear();
	cnt_=0;
	scanCnt_=0;
	rootNode_=0;

	query_->swap(query);

	maxNum_=query_->maxNum();

	servers_.clear();

	if(parser_)
		delete parser_;

	UserMessage::message(UserMessage::DBG,false, std::string("Query: " + query_->query().toStdString()));

	parser_=NodeExpressionParser::parseWholeExpression(query_->query().toStdString(), query->caseSensitive());
	if(parser_ == NULL)
	{
		UserMessage::message(UserMessage::ERROR,true, std::string("Error, unable to parse enabled condition: " + query_->query().toStdString()));
		return;
	}

	QStringList serverNames=query_->servers();
	if(query_->servers().isEmpty())
		serverNames=allServers;

	Q_FOREACH(QString s,serverNames)
	{
		if(ServerHandler* server=ServerHandler::find(s.toStdString()))
		{
			servers_.push_back(server);
		}
	}

	if(!query_->rootNode().empty())
	{
		if(servers_.size() != 1)
			return;

		rootNode_=servers_.at(0)->vRoot()->find(query_->rootNode());
	}

	//Notify the servers that the search began
	Q_FOREACH(ServerHandler* s,servers_)
	{
		s->searchBegan();
	}

	//Start thread execution
	start();
}

void NodeQueryEngine::stopQuery()
{
	stopIt_=true;
}

void NodeQueryEngine::run()
{
	if(rootNode_)
	{
		run(servers_.at(0),rootNode_);
	}
	else
	{
		for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end(); ++it)
		{
			ServerHandler *server=*it;

			run(server,server->vRoot());

			if(stopIt_)
			{
				broadcastChunk(true);
				return;
			}
		}
	}

	broadcastChunk(true);
}

void NodeQueryEngine::run(ServerHandler *server,VNode* root)
{
	runRecursively(root);
}

void NodeQueryEngine::runRecursively(VNode *node)
{
	if(stopIt_)
		return;

	if(parser_->execute(node))
	{
		broadcastFind(node);
	}

	scanCnt_++;

	for(int i=0; i < node->numOfChildren(); i++)
	{
		runRecursively(node->childAt(i));
		if(stopIt_)
			return;
	}
}

void NodeQueryEngine::broadcastFind(VNode* node)
{
	NodeQueryResultTmp_ptr d(new NodeQueryResultTmp(node));

	//Add to res vector
	res_ << d;

	broadcastChunk(false);

	cnt_++;

	if(cnt_ >= maxNum_)
	{
		broadcastChunk(true);
		stopIt_=true;
	}
}

void NodeQueryEngine::broadcastChunk(bool force)
{
	bool doIt=false;
	if(!force)
	{
		if(res_.count() >= chunkSize_)
		{
			doIt=true;
		}
	}
	else if(!res_.isEmpty())
	{
		doIt=true;
	}

	if(doIt)
	{
		Q_EMIT found(res_);
		res_.clear();
	}
}

void NodeQueryEngine::slotFinished()
{
	//Notify the servers that the search finished
	Q_FOREACH(ServerHandler* s,servers_)
	{
		s->searchFinished();
	}
}

void NodeQueryEngine::slotFailed()
{

}

NodeFilterEngine::NodeFilterEngine(NodeFilter* owner) :
	query_(new NodeQuery("tmp")),
	parser_(NULL),
	server_(NULL),
	owner_(owner)
{
}

NodeFilterEngine::~NodeFilterEngine()
{
	delete query_;

	if(parser_)
		delete parser_;
}

void NodeFilterEngine::setQuery(NodeQuery* query)
{
	query_->swap(query);

	if(parser_)
		delete parser_;

	parser_=NodeExpressionParser::parseWholeExpression(query_->query().toStdString());
	if(parser_ == NULL)
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse enabled condition: " + query_->query().toStdString()));
	}
}


void NodeFilterEngine::runQuery(ServerHandler* server)
{
	if(!query_)
		return;

	server_=server;
	if(!server_)
		return;

	if(!parser_)
		return;

	runRecursively(server_->vRoot());
}

void NodeFilterEngine::runRecursively(VNode *node)
{
	if(!node->isServer() && parser_->execute(node))
	{
		//UserMessage::message(UserMessage::DBG,false,"FOUND: " + node->absNodePath());
		owner_->res_[node->index()]=1;
	}

	for(int i=0; i < node->numOfChildren(); i++)
	{
		runRecursively(node->childAt(i));
	}
}















