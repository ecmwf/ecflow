//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <algorithm>

#include "CustomCommandHandler.hpp"

#include "DirectoryHandler.hpp"
#include "File.hpp"
#include "VSettings.hpp"


CustomCommand::CustomCommand(const std::string &name, const std::string &command, bool context)
{
    name_          = name;
    command_       = command;
    inContextMenu_ = context;
}


void CustomCommand::set(const std::string &name, const std::string &command, bool context)
{
	name_          = name;
	command_       = command;
	inContextMenu_ = context;
}


void CustomCommand::save(VSettings *vs)
{
    vs->put("name",    name());
    vs->put("command", command());
    vs->put("context", contextString());
}



CustomCommandHandler::CustomCommandHandler()
{
}


void CustomCommandHandler::init(const std::string& configFilePath)
{

    dirPath_ = configFilePath;
    readSettings();

    /*
    dirPath_=dirPath;
	DirectoryHandler::createDir(dirPath_);

	std::vector<std::string> res;
	std::string pattern=".*\\." + suffix_ + "$";
	DirectoryHandler::findFiles(dirPath_,pattern,res);

	for(std::vector<std::string>::const_iterator it=res.begin(); it != res.end(); ++it)
	{
		std::string fName=DirectoryHandler::concatenate(dirPath_,*it);
		VSettings vs(fName);
		vs.read();

		std::size_t pos=(*it).find("." + suffix_);
		assert(pos != std::string::npos);

		std::string name=(*it).substr(0,pos);
		NodeQuery* item=add(name);
		item->load(&vs);
	}
*/
}






CustomCommand* CustomCommandHandler::replace(int index, const std::string& name, const std::string& command, bool context)
{
    assert(index >= 0);
    assert(index < items_.size());

    CustomCommand *item;

    // already in the list - just update it
    item = items_[index];
    item->set(name, command, context);

    writeSettings();

    return item;
}

CustomCommand* CustomCommandHandler::replace(int index, const CustomCommand &cmd)
{
    replace(index, cmd.name(), cmd.command(), cmd.inContextMenu());
}



void CustomCommandHandler::remove(int index)
{
    assert(index >= 0);
    assert(index < items_.size());

    items_.erase(items_.begin()+index);

    writeSettings();
}

CustomCommand* CustomCommandHandler::duplicate(int index)
{
    assert(index >= 0);
    assert(index < items_.size());

    CustomCommand *item = items_[index];
    std::string postfix("_1");
    std::string newName = item->name() + postfix;

    // ensure we are creating a unique new name - if we find an existing item with the same name, add another postfix
    while(find(newName) != NULL)
        newName += postfix;

    CustomCommand*newCmd = add(newName, item->command(), item->inContextMenu(), false);

    writeSettings();
}

void CustomCommandHandler::swapCommandsByIndex(int i1, int i2)
{
    assert(i1 >= 0);
    assert(i1 < items_.size());
    assert(i2 >= 0);
    assert(i2 < items_.size());

    CustomCommand *temp = items_[i2];
    items_[i2] = items_[i1];
    items_[i1] = temp;
}


CustomCommand* CustomCommandHandler::find(const std::string& name) const
{
    for(std::deque<CustomCommand*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if((*it)->name() == name)
            return *it;
    }
    return NULL;
}


// find the index of the command which has the given name; -1 if not found
int CustomCommandHandler::findIndexFromName(const std::string& name) const
{
    int i = 0;
    for(std::deque<CustomCommand*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if((*it)->name() == name)
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
    VSettings vs(dirPath_);

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
    std::vector<VSettings> vsItems;
    std::string dummyFileName="dummy";
    std::string key="commands";
    VSettings vs(dirPath_);

    bool ok = vs.read(false);  // false means we don't abort if the file is not there

    if (ok)
    {
        std::vector<VSettings> commands;
        vs.get("commands", commands);

        for (int i = 0; i < commands.size(); i++)
        {
            VSettings *vsCommand = &commands[i];
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
    CustomCommand *item;
    //int index = findIndex(name);

    //if (index == -1)  // not already in the list
    {
        item=new CustomCommand(name, command, context);
        items_.push_back(item);
    }
    //else  // already in the list - just update it
    //{
    //    item = items_[index];
    //    item->set(name, command, context);
    //}

    if (saveSettings)
        writeSettings();

    return item;
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
    CustomCommand *item;
    int index = findIndexFromName(name);

    if (index == -1)  // not already in the list
    {
        item=new CustomCommand(name, command, context);
        items_.push_front(item);  // add it to the front

        if (items_.size() > maxCommands_)  // too many commands?
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



