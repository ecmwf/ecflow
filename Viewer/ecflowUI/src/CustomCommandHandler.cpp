//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <algorithm>

#include "CustomCommandHandler.hpp"
#include "SessionHandler.hpp"

#include "DirectoryHandler.hpp"
#include "File.hpp"
#include "VSettings.hpp"


CustomCommand::CustomCommand(const std::string &name, const std::string &command, bool context) :
    name_(name), command_(command), inContextMenu_(context)
{
}


void CustomCommand::set(const std::string &name, const std::string &command, bool context)
{
	name_          = name;
	command_       = command;
	inContextMenu_ = context;
}


void CustomCommand::save(VSettings *vs) const
{
    vs->put("name",    name());
    vs->put("command", command());
    vs->put("context", contextString());
}

CustomCommandHandler::CustomCommandHandler()
{
}

void CustomCommandHandler::init()
{
    readSettings();
}

CustomCommand* CustomCommandHandler::replace(int index, const std::string& name, const std::string& command, bool context)
{
    assert(index >= 0);
    assert(index < static_cast<int>(items_.size()));

    CustomCommand *item;

    // already in the list - just update it
    item = items_[index];
    item->set(name, command, context);

    writeSettings();

    return item;
}

CustomCommand* CustomCommandHandler::replace(int index, const CustomCommand &cmd)
{
    return replace(index, cmd.name(), cmd.command(), cmd.inContextMenu());
}



void CustomCommandHandler::remove(int index)
{
    assert(index >= 0);
    assert(index < static_cast<int>(items_.size()));

    items_.erase(items_.begin()+index);

    writeSettings();
}

CustomCommand* CustomCommandHandler::duplicate(int index)
{
    assert(index >= 0);
    assert(index < static_cast<int>(items_.size()));

    CustomCommand *item = items_[index];
    std::string postfix("_1");
    std::string newName = item->name() + postfix;

    // ensure we are creating a unique new name - if we find an existing item with the same name, add another postfix
    while(find(newName) != NULL)
        newName += postfix;

    CustomCommand*newCmd = add(newName, item->command(), item->inContextMenu(), false);

    writeSettings();

    return newCmd;
}

void CustomCommandHandler::swapCommandsByIndex(int i1, int i2)
{
    assert(i1 >= 0);
    assert(i1 < static_cast<int>(items_.size()));
    assert(i2 >= 0);
    assert(i2 < static_cast<int>(items_.size()));

    CustomCommand *temp = items_[i2];
    items_[i2] = items_[i1];
    items_[i1] = temp;
}


CustomCommand* CustomCommandHandler::find(const std::string& name) const
{
    for(auto item : items_)
    {
        if(item->name() == name)
            return item;
    }
    return NULL;
}


// find the index of the command which has the given name; -1 if not found
int CustomCommandHandler::findIndexFromName(const std::string& name) const
{
    int i = 0;
    for(auto item : items_)
    {
        if(item->name() == name)
            return i;
        i++;
    }
    return -1;  // it was not found
}


void CustomCommandHandler::writeSettings()
{
    std::vector<VSettings> vsItems;
    std::string dummyFileName="dummy";
    std::string key="commands";

    std::string settingsFilePath = settingsFile();
    VSettings vs(settingsFilePath);

    for(int i = 0; i < numCommands(); i++)
    {
        VSettings vsThisItem(dummyFileName);
        CustomCommand *cmd = commandFromIndex(i);
        cmd->save(&vsThisItem);
        vsItems.push_back(vsThisItem);
    }
    vs.put(key,vsItems);
    vs.write();
}

void CustomCommandHandler::readSettings()
{   
    std::string settingsFilePath = settingsFile();
    VSettings vs(settingsFilePath);

    bool ok = vs.read(false);  // false means we don't abort if the file is not there

    if(ok)
    {
        std::vector<VSettings> commands;
        vs.get("commands", commands);

        for (auto & i : commands)
        {
            VSettings *vsCommand = &i;
            std::string emptyDefault="";
            std::string name    = vsCommand->get("name",    emptyDefault);
            std::string command = vsCommand->get("command", emptyDefault);
            std::string context = vsCommand->get("context", emptyDefault);
            add(name, command, stringToBool(context), false);  // add it to our in-memory list
        }
    }
}

bool CustomCommandHandler::stringToBool(std::string &str)
{
    bool result = (!str.empty() && str == "yes");
    return result;
}


// -------------------------
// CustomSavedCommandHandler
// -------------------------

CustomSavedCommandHandler* CustomSavedCommandHandler::instance_=0;


CustomSavedCommandHandler* CustomSavedCommandHandler::instance()
{
    if(!instance_)
    {
        instance_=new CustomSavedCommandHandler();
    }

    return instance_;
}

CustomCommand* CustomSavedCommandHandler::add(const std::string& name, const std::string& command, bool context, bool saveSettings)
{
    CustomCommand *item=new CustomCommand(name, command, context);
    items_.push_back(item);

    if (saveSettings)
        writeSettings();

    return item;
}

std::string CustomSavedCommandHandler::settingsFile()
{
    SessionItem* cs=SessionHandler::instance()->current();
    return cs->savedCustomCommandsFile();
}


// ---------------------------
// CustomCommandHistoryHandler
// ---------------------------


CustomCommandHistoryHandler* CustomCommandHistoryHandler::instance_=0;

CustomCommandHistoryHandler::CustomCommandHistoryHandler()
{
    maxCommands_ = 10;
}


CustomCommandHistoryHandler* CustomCommandHistoryHandler::instance()
{
    if(!instance_)
    {
        instance_=new CustomCommandHistoryHandler();
    }

    return instance_;
}


CustomCommand* CustomCommandHistoryHandler::add(const std::string& name, const std::string& command, bool context, bool saveSettings)
{
    int index = findIndexFromName(name);

    if (index == -1)  // not already in the list
    {
        CustomCommand *item=new CustomCommand(name, command, context);
        items_.push_front(item);  // add it to the front

        if(static_cast<int>(items_.size()) > maxCommands_)  // too many commands?
        {
            items_.pop_back(); // remove the last item
        }

        if (saveSettings)
            writeSettings();

        return item;
    }
    else
    {
        return commandFromIndex(index);
    }

}

std::string CustomCommandHistoryHandler::settingsFile()
{
    SessionItem* cs=SessionHandler::instance()->current();
    return cs->recentCustomCommandsFile();
}


/*
void NodeQueryHandler::add(NodeQuery* item,bool saveToFile)
{
	items_.push_back(item);
	if(saveToFile)
		save(item);
}


void NodeQueryHandler::remove(const std::string&)
{
}

void NodeQueryHandler::remove(NodeQuery*)
{

}


void NodeQueryHandler::save()
{
}

void NodeQueryHandler::save(NodeQuery *item)
{
	std::string f=DirectoryHandler::concatenate(dirPath_,item->name() + "." + suffix_);
	VSettings vs(f);
	item->save(&vs);
	vs.write();

}

*/



