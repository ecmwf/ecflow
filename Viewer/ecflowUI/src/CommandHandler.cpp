//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CommandHandler.hpp"

//#include "File.hpp"
//#include "NodeFwd.hpp"
//#include "ArgvCreator.hpp"
#include "Str.hpp"

#include "ServerHandler.hpp"
#include "ShellCommand.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"

#include <QRegExp>
#include <QString>

#include <map>

//Send the same command for a list of objects (nodes/servers) specified in a VInfo vector.
//The command is specified as a string.

void CommandHandler::run(std::vector<VInfo_ptr> info, const std::string& cmd)
{
    UI_FUNCTION_LOG

    std::string realCommand(cmd);
    std::vector<ServerHandler*> targetServers;

    if(realCommand.empty())
    {
        //UiLog().err() << " command is not recognised. Check the menu definition.";
        UserMessage::message(UserMessage::ERROR, true, "command " + cmd +
                             " is not recognised. Check the menu definition.");
        return;
    }

    UiLog().dbg() << "command=" << cmd;

    std::map<ServerHandler*,std::string> targetNodeNames;
    std::map<ServerHandler*,std::string> targetNodeFullNames;
    std::map<ServerHandler*,std::string> targetParentFullNames;

    //Figure out what objects (node/server) the command should be applied to
    for(auto & i : info)
    {
        std::string nodeFullName;
        std::string nodeName;
        std::string parentFullName;

        if(realCommand.find("<node_name>") != std::string::npos)
        {
            nodeName=i->name();
        }

        if(realCommand.find("<full_name>") != std::string::npos)
        {
            if(i->isNode())
                nodeFullName = i->node()->absNodePath();
            else if(i->isServer())
                i->server()->longName();
            else if(i->isAttribute())
                parentFullName = i->node()->absNodePath();
        }

        if(realCommand.find("<parent_name>") != std::string::npos)
        {
            if(i->isNode())
            {
                if(VNode *p=i->node()->parent())
                    parentFullName = p->absNodePath();
            }
            else if(i->isAttribute())
               parentFullName = i->node()->absNodePath();
        }

        //Store the names per target servers
        targetNodeNames[i->server()] += " " + nodeName;
        targetNodeFullNames[i->server()] += " " + nodeFullName;
        targetParentFullNames[i->server()] += " " + parentFullName;

        // add this to our list of target servers?
        if(std::find(targetServers.begin(), targetServers.end(), i->server()) == targetServers.end())
        {
            targetServers.push_back(i->server());
        }
    }

    // for each target server, construct and send its command
    for(auto serverHandler : targetServers)
    {
        // replace placeholders with real node names
        std::string placeholder("<full_name>");
        ecf::Str::replace_all(realCommand, placeholder, targetNodeFullNames[serverHandler]);

        placeholder = "<node_name>";
        ecf::Str::replace_all(realCommand, placeholder, targetNodeNames[serverHandler]);

        placeholder = "<parent_name>";
        ecf::Str::replace_all(realCommand, placeholder, targetParentFullNames[serverHandler]);

        //Shell command
        if(realCommand.find("sh ") == 0)
        {
            substituteVariables(realCommand,info);
            UiLog().dbg() << " final command: " << realCommand;
            ShellCommand::run(realCommand,cmd);
            return;
        }

        UiLog().dbg() << " final command: " << realCommand;

        // get the command into the right format by first splitting into tokens
        // and then converting to argc, argv format
        std::vector<std::string> strs;
        std::string delimiters(" ");
        ecf::Str::split(realCommand, strs, delimiters);

        // set up and run the thread for server communication
        serverHandler->runCommand(strs);
    }
}

//Send a command to a server. The command is specified as a string vector, while the node or server for that
//the command will be applied is specified in a VInfo object.
void CommandHandler::run(VInfo_ptr info,const std::vector<std::string>& cmd)
{
    UI_FUNCTION_LOG

    std::vector<std::string> realCommand=cmd;

    if(realCommand.empty())
    {
        //UiLog().err() << " command is not recognised!";
        UserMessage::message(UserMessage::ERROR, true, "command is not recognised.");
    }

    UiLog().dbg() << "command: " << commandToString(realCommand);

    //Get the name of the object for that the command will be applied
    std::string nodeFullName;
    std::string nodeName;
    ServerHandler* serverHandler = info->server();

    if(info->isNode() || info->isAttribute())
    {
        nodeFullName = info->node()->node()->absNodePath();
        nodeName = info->node()->node()->name();
        //UserMessage::message(UserMessage::DBG, false, std::string("  --> for node: ") + nodeFullName + " (server: " + info[i]->server()->longName() + ")");
    }
    else if(info->isServer())
    {
        nodeFullName = "/";
        nodeName = "/";
        //UserMessage::message(UserMessage::DBG, false, std::string("  --> for server: ") + nodeFullName);
    }

    //Replace placeholders with real node names
    for(std::size_t i=0; i < cmd.size(); i++)
    {
        if(realCommand[i]=="<full_name>")
            realCommand[i]=nodeFullName;
        else if(realCommand[i]=="<node_name>")
            realCommand[i]=nodeName;
    }

    UiLog().dbg() << " final command: " << commandToString(realCommand);

        // get the command into the right format by first splitting into tokens
        // and then converting to argc, argv format

        //std::vector<std::string> strs;
        //std::string delimiters(" ");
        //ecf::Str::split(realCommand, strs, delimiters);

    // set up and run the thread for server communication
    serverHandler->runCommand(realCommand);
}

void CommandHandler::run(VInfo_ptr info, const std::string& cmd)
{
    std::vector<std::string> commands;

    ecf::Str::split(cmd, commands);
    run(info, commands);
}

std::string CommandHandler::commandToString(const std::vector<std::string>& cmd)
{   
    std::string s;
    for(const auto & it : cmd)
    {
        if(!s.empty()) s+=" ";
        s+=it;
    }
    return s;
}

void CommandHandler::substituteVariables(std::string& cmd,const std::vector<VInfo_ptr>& info)
{
   if(info.size() > 0)
   {
       VNode *n=info[0]->node();
       if(!n || n->isAttribute())
            return;

       QString txt=QString::fromStdString(cmd);
       QString txtRes=txt;
       QRegExp rx("%(.*)%");
       rx.setMinimal(true);
       int pos=0;
       while ((pos = rx.indexIn(txt, pos)) != -1)
       {
            QString name=rx.cap(1);
            pos += rx.matchedLength();
            std::string value=n->findInheritedVariable(name.toStdString(),true);
            txtRes.replace("%" + name + "%",QString::fromStdString(value));
       }

       cmd = txtRes.toStdString();
   }
}
