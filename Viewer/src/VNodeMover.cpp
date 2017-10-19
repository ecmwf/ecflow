//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VNodeMover.hpp"

#include "CommandHandler.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"
#include "UserMessage.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"

VInfo_ptr VNodeMover::nodeMarkedForMove_=VInfo_ptr();

bool VNodeMover::hasMarkedForMove()
{
    return (nodeMarkedForMove_ && nodeMarkedForMove_->isNode());
}

void VNodeMover::markNodeForMove(VInfo_ptr markedNode)
{
    UI_FUNCTION_LOG

    nodeMarkedForMove_=markedNode;

    UI_ASSERT(markedNode->isNode(),"markedNode=" << markedNode->path());
    VNode* vNode=markedNode->node();
    UI_ASSERT(vNode,"markedNode->node() is NULL");

    if(!vNode->isSuspended())
    {
        CommandHandler::run(markedNode,"ecflow_client --suspend <full_name>");
        UiLog().info()  << " node=" << markedNode->path() << " suspended and marked for move.";
    }
}

void VNodeMover::moveMarkedNode(VInfo_ptr destNode)
{
    // if same server, then error
    // NO - ecflowview had this restriction, but it does not seem to be necessary

    if(!destNode)
    {
        UserMessage::message(UserMessage::ERROR, true, "Cannot move node! No target path is selected for move!");
        return;
    }

    if(!nodeMarkedForMove_ || !nodeMarkedForMove_->isNode())
    {
        UserMessage::message(UserMessage::ERROR, true, "Cannot move node! No node is marked for move!");
        return;
    }

    //get the stored path
    VItemPathParser srcPath(nodeMarkedForMove_->storedPath());

    //Regain the VInfo object's data
    nodeMarkedForMove_->regainData();

    //get the source (marked) vnode
    VNode* vnSrc=nodeMarkedForMove_->node();
    if(!vnSrc)
    {
        UserMessage::message(UserMessage::ERROR, true, "Cannot move node! The node " + srcPath.node() +
                             ", marked for move, no longer exists on server " + srcPath.server() );
        return;
    }

    //get the source (marked) server
    ServerHandler* serverSrc=nodeMarkedForMove_->server();
    if(!serverSrc)
    {
        UserMessage::message(UserMessage::ERROR, true, "Cannot move node! The source server " + serverSrc->name() +
                             " must be loaded into the UI");
        return;
    }

    // can only do this if the source (marked) node is suspended
    if(!vnSrc->isSuspended())
    {
        UserMessage::message(UserMessage::ERROR, true, "Cannot move node! Node " + nodeMarkedForMove_->path() +
                                                               " must be suspended first.");
        return;
    }

    //get the destination
    ServerHandler *serverDest = destNode->server();
    if(!serverDest)
    {
        return;
    }

    bool ok = UserMessage::confirm("About to move node <b>" + nodeMarkedForMove_->relativePath() +
                                   "</b> from server <b>" + serverSrc->name() +
                                   " (" + serverSrc->host() + ":" + serverSrc->port() + ")</b> to <b>" +
                                   serverDest->name() +
                                   " (" + serverDest->host() + ":" + serverDest->port() + ") " +
                                   destNode->relativePath() + "</b>. Ok?");

    // do it (?)
    if(ok)
    {
        std::string plugCommand;
        plugCommand = "ecflow_client --plug <full_name> " + serverDest->host() +
                ":" + serverDest->port() + destNode->relativePath();
        CommandHandler::run(nodeMarkedForMove_,plugCommand); //this will run it on the source server!!

        nodeMarkedForMove_.reset();

        //We should update the dest server only when the plug command has finished.
        //However the run command above is asynchronous, so we would need an observer to properly
        //time this update. As a temporary measure we call reset here, it takes some time and we
        //have more time to the plug command to finish.
        serverDest->reset(); //TODO: use VTask and update serverDest with refresh()
    }
}
