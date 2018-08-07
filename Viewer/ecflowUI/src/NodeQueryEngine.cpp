//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQueryEngine.hpp"

#include <QtAlgorithms>
#include <QStandardItemModel>

#include "NodeExpression.hpp"
#include "NodeQuery.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VAttributeType.hpp"
#include "VFilter.hpp"
#include "VNode.hpp"

static bool metaRegistered=false;

NodeQueryEngine::NodeQueryEngine(QObject* parent) :
    QThread(parent),
    query_(new NodeQuery("tmp")),
    parser_(NULL),
    cnt_(0),
    scanCnt_(0),
    maxNum_(250000),
    chunkSize_(100),
    stopIt_(false),
    maxReached_(false),   
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

bool NodeQueryEngine::runQuery(NodeQuery* query,QStringList allServers)
{
    UiLog().dbg() << "NodeQueryEngine::runQuery -->";

    if(isRunning())
        wait();

    stopIt_=false;
    maxReached_=false;
    res_.clear();
    cnt_=0;
    scanCnt_=0;
    rootNode_=0;

    query_->swap(query);

    maxNum_=query_->maxNum();

    servers_.clear();

    //Init the parsers
    if(parser_)
    {
        delete parser_;
        parser_=NULL;
    }

    qDeleteAll(attrParser_);
    attrParser_.clear();

    //The nodequery parser
    UiLog().dbg() << " node part: " << query_->nodeQueryPart().toStdString();

    parser_=NodeExpressionParser::instance()->parseWholeExpression(query_->nodeQueryPart().toStdString(), query->caseSensitive());
    if(parser_ == NULL)
    {
        UiLog().err() << " unable to parse node query: " << query_->nodeQueryPart().toStdString();
        UserMessage::message(UserMessage::ERROR,true,"Error, unable to parse node query: " + query_->nodeQueryPart().toStdString());
        return false;
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
            return false;

        rootNode_=servers_.at(0)->vRoot()->find(query_->rootNode());
        if(!rootNode_)
        {
            UiLog().err() << " the specified root node does not exist: " << query_->rootNode();
            UserMessage::message(UserMessage::ERROR,true,
                         "Error, the specified root node <u>does not</u> exist: " + query_->rootNode());
            return false;
        }
    }

    //The attribute parser
    UiLog().dbg() << " full attr part: " << query_->attrQueryPart().toStdString();

    for(auto it : VAttributeType::types())
    {
        if(query_->hasAttribute(it))
        {
            QString attrPart=(query_->attrQueryPart(it));
            UiLog().dbg() << "  " << it->strName() << ": " << attrPart.toStdString();
            BaseNodeCondition* ac=NodeExpressionParser::instance()->parseWholeExpression(attrPart.toStdString(), query->caseSensitive());
            if(!ac)
            {
                UiLog().err() << "  unable to parse attribute query: " << attrPart.toStdString();
                UserMessage::message(UserMessage::ERROR,true, "Error, unable to parse attribute query: " + attrPart.toStdString());
                return false;
            }
            attrParser_[it]=ac;
         }
    }

    //Notify the servers that the search began
    Q_FOREACH(ServerHandler* s,servers_)
    {
        s->searchBegan();
    }

    //Start thread execution
    start();

    UiLog().dbg() << "<-- runQuery";

    return true;
}

void NodeQueryEngine::stopQuery()
{
    stopIt_=true;
    wait();
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

    //Execute the node part
    if(parser_->execute(node))
    {
        //Then execute the attribute part
        if(!attrParser_.isEmpty())
        {
            QMap<VAttributeType*,BaseNodeCondition*>::const_iterator it = attrParser_.constBegin();
            while (it != attrParser_.constEnd())
            {
                //Process a given attribute type
                const std::vector<VAttribute*>& av=node->attr();
                bool hasType=false;
                for(auto i : av)
                {
                    if(i->type() == it.key())
                    {
                        hasType=true;
                        if(it.value()->execute(i))
                        {
                            broadcastFind(node,i->data(true));
                            scanCnt_++;
                         }
                    }
                    //The the attribute vector elements are grouped by type.
                    //So we leave the loop when we reach the next type group
                    else if(hasType)
                    {
                        break;
                    }
                }
                ++it;
            }


        }
        else
        {
            broadcastFind(node);
            scanCnt_++;
        }
    }

    for(int i=0; i < node->numOfChildren(); i++)
    {
        runRecursively(node->childAt(i));
        if(stopIt_)
            return;
    }
}

void NodeQueryEngine::broadcastFind(VNode* node,QStringList attr)
{
    Q_ASSERT(node);

    if(!attr.isEmpty())
    {
        NodeQueryResultTmp_ptr d(new NodeQueryResultTmp(node,attr));
        res_ << d;       
    }
    else
    {
        NodeQueryResultTmp_ptr d(new NodeQueryResultTmp(node));
        res_ << d;
    }

    broadcastChunk(false);

    cnt_++;

    if(cnt_ >= maxNum_)
    {
        broadcastChunk(true);
        stopIt_=true;
        maxReached_=true;
    }
}

void NodeQueryEngine::broadcastFind(VNode* node)
{
    Q_ASSERT(node);

    NodeQueryResultTmp_ptr d(new NodeQueryResultTmp(node));
    res_ << d;

    broadcastChunk(false);

    cnt_++;

    if(cnt_ >= maxNum_)
    {
        broadcastChunk(true);
        stopIt_=true;
        maxReached_=true;
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
    owner_(owner),
    rootNode_(0)
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

    parser_=NodeExpressionParser::instance()->parseWholeExpression(query_->query().toStdString());
    if(parser_ == NULL)
    {
        UiLog().err() << "Error, unable to parse enabled condition: " << query_->query().toStdString();
        UserMessage::message(UserMessage::ERROR, true,"Error, unable to parse enabled condition: " + query_->query().toStdString());
    }
}


bool NodeFilterEngine::runQuery(ServerHandler* server)
{
    rootNode_=0;

    if(!query_)
        return false;

    server_=server;
    if(!server_)
        return false;

    if(!parser_)
        return false;

    if(!query_->rootNode().empty() &&
      (query_->servers().count() == 1 && !query_->servers()[0].simplified().isEmpty()))
    {
        rootNode_=server_->vRoot()->find(query_->rootNode());
        if(!rootNode_)
        {
            UiLog().err() << " the specified root node does not exist: " << query_->rootNode();
#if 0
            UserMessage::message(UserMessage::ERROR,true,
                         "Node filter failed! The specified root node <u>does not</u> exist: <b>" + query_->rootNode() +
                         "</b><br> Please redefine your filter!");
#endif
            return false;
        }
    }

    if(rootNode_)
        runRecursively(rootNode_);
    else
        runRecursively(server_->vRoot());

    return true;
}

void NodeFilterEngine::runRecursively(VNode *node)
{
    if(!node->isServer() &&
       (node == owner_->forceShowNode() ||  parser_->execute(node)))
    {
        owner_->match_.push_back(node);
    }

    for(int i=0; i < node->numOfChildren(); i++)
    {
        runRecursively(node->childAt(i));
    }
}
