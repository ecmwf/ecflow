//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <unistd.h>
 
#include "SessionHandler.hpp"
#include "DirectoryHandler.hpp"
#include "Str.hpp"
#include "UserMessage.hpp"
#include "ServerList.hpp"


SessionHandler* SessionHandler::instance_=0;


SessionItem::SessionItem(const std::string& name) :
  name_(name)
{
	checkDir();
	isTemporary_ = false;
	askToPreserveTemporarySession_ = true;
}

SessionItem::~SessionItem()
{
	// temporary session? if so, remove the directory on exit
	if (isTemporary_)
	{
		std::string msg;
        DirectoryHandler::removeDir(dirPath_, msg);
	}
}

void SessionItem::checkDir()
{
	dirPath_ = SessionHandler::sessionDirName(name_);
	DirectoryHandler::createDir(dirPath_);

    qtPath_= SessionHandler::sessionQtDirName(name_);
    DirectoryHandler::createDir(qtPath_);
}

std::string SessionItem::sessionFile() const
{
	return DirectoryHandler::concatenate(dirPath_, "session.json");
}

std::string SessionItem::windowFile() const
{
	return DirectoryHandler::concatenate(dirPath_, "window.conf");
}

std::string SessionItem::settingsFile() const
{
	return DirectoryHandler::concatenate(dirPath_, "settings.json");
}

std::string SessionItem::recentCustomCommandsFile() const
{
    return DirectoryHandler::concatenate(dirPath_, "recent_custom_commands.json");
}

std::string SessionItem::savedCustomCommandsFile() const
{
    return DirectoryHandler::concatenate(dirPath_, "saved_custom_commands.json");
}

std::string SessionItem::serverFile(const std::string& serverName) const
{
	return DirectoryHandler::concatenate(dirPath_, serverName + ".conf.json");
}

std::string SessionItem::qtDir() const
{
    return qtPath_;
}

std::string SessionItem::qtSettingsFile(const std::string name) const
{
    return DirectoryHandler::concatenate(qtPath_, name + ".conf");
}

//=================================================
//
// SessionHandler
//
//=================================================

SessionHandler::SessionHandler() :
	current_(0)
{
	//The default must always be exist!
	current_=add(defaultSessionName());
	loadedLastSessionName_ = false;

	readSessionListFromDisk();
	readLastSessionName();
}

SessionHandler::~SessionHandler()
{
    for(std::vector<SessionItem*>::const_iterator it=sessions_.begin(); it != sessions_.end(); ++it)
    {
        delete (*it);
    }
}


SessionHandler* SessionHandler::instance()
{
	if(!instance_)
	{
		instance_=new SessionHandler();
	}

	return instance_;
}

void SessionHandler::destroyInstance()
{
	if (instance_)
		delete instance_;

	instance_ = 0;
}

std::string SessionHandler::sessionDirName(const std::string &sessionName)
{
	return DirectoryHandler::concatenate(DirectoryHandler::configDir(), sessionName + ".session");
}

std::string SessionHandler::sessionQtDirName(const std::string &sessionName)
{
	std::string basedir = sessionDirName(sessionName);
	return DirectoryHandler::concatenate(basedir, "qt");
}

SessionItem* SessionHandler::find(const std::string& name)
{
    for(std::vector<SessionItem*>::const_iterator it=sessions_.begin(); it != sessions_.end(); ++it)
    {
        if((*it)->name() == name)
            return *it;
    }
    return NULL;

}


// returns -1 if the session name is not found
int SessionHandler::indexFromName(const std::string& name)
{
    int n = 0;
    for(std::vector<SessionItem*>::const_iterator it=sessions_.begin(); it != sessions_.end(); ++it)
    {
        if((*it)->name() == name)
            return n;
        n++;
    }
    return -1;
}

void SessionHandler::readSessionListFromDisk()
{
    // get the list of existing sessions (by listing the directories)
    std::string configDir = DirectoryHandler::configDir();
    std::string filter = ".*\\.session";
    std::vector<std::string> dirs;
    DirectoryHandler::findDirs(configDir, filter, dirs);

    // add each session to our list (but remove the .session first)
    for(std::vector<std::string>::const_iterator it=dirs.begin(); it != dirs.end(); ++it)
    {
        std::string dirName       = (*it);
        std::string toRemove      = ".session";
        std::string toReplaceWith = "";
        ecf::Str::replace(dirName, toRemove, toReplaceWith);
        add(dirName);
    }
}

bool SessionHandler::loadLastSessionAtStartup()
{
    // if there was a last session file, then it means the user wanted to load at startup
    return loadedLastSessionName_;
}



SessionItem* SessionHandler::add(const std::string& name)
{
	// only add if not already there
	if (find(name) == NULL)
	{
		SessionItem *item=new SessionItem(name);
		sessions_.push_back(item);
		return item;
	}
	else
		return NULL;
}

void SessionHandler::remove(const std::string& sessionName)
{
	SessionItem *session = find(sessionName);
	assert(session);

	remove(session);
}

void SessionHandler::remove(SessionItem* session)
{
    std::vector<SessionItem*>::iterator it = std::find(sessions_.begin(), sessions_.end(), session);
    assert(it != sessions_.end());  // session was not found - should not be possible


    // remove the session's directory
    std::string errorMessage;
    bool ok = DirectoryHandler::removeDir(sessionDirName(session->name()), errorMessage);

    if (ok)
    {
        // remove it from our list
        sessions_.erase(it);
    }
    else
    {
        UserMessage::message(UserMessage::ERROR, true, errorMessage);
    }
}


void SessionHandler::rename(SessionItem* item, const std::string& newName)
{
    std::string errorMessage;
    bool ok = DirectoryHandler::renameDir(sessionDirName(item->name()), sessionDirName(newName), errorMessage);

    if (ok)
    {
        item->name(newName);
    }
    else
    {
        UserMessage::message(UserMessage::ERROR, true, errorMessage);
    }
}


void SessionHandler::current(SessionItem* item)
{
	if(std::find(sessions_.begin(),sessions_.end(),item) != sessions_.end())
	{
		current_=item;
		load();
	}
}


SessionItem* SessionHandler::current()
{
	return current_;
}

void SessionHandler::save()
{
	if(current_)
	{
		//current_->save();
	}
}

void SessionHandler::load()
{

}


bool SessionHandler::requestStartupViaSessionManager()
{
	char *sm = getenv("ECFUI_SESSION_MANAGER");
	if (sm)
		return true;
	else
		return false;
}


void SessionHandler::setTemporarySessionIfReqested()
{
	char *sh = getenv("ECFUI_TEMP_SESSION_HOST");
	if (sh)
	{
		char *sp = getenv("ECFUI_TEMP_SESSION_PORT");
		if (sp)
		{
			// create a session name, likely to be unique - if it already exists in the list, then use that
			char sessName[1024];
			sprintf(sessName, "temporary_%s_%s_%d", sh, sp, getpid());
			std::string sname(sessName);
			SessionItem *si = instance()->add(sname);
			if (!si)
				si = instance()->find(sname);

			instance()->current(si);
			si->temporary(true);

			char *sask = getenv("ECFUI_TEMP_SESSION_PRESERVE_CONFIRM");
			if (sask && !strcmp(sask, "no"))
				si->askToPreserveTemporarySession(false);

			std::string templateName("ecflow_ui_test_session_template.json");
			instance()->createSessionDirWithTemplate(sname, templateName);

			// construct an alias for this server
			char calias[1024];
			sprintf(calias, "%s:%s", sh, sp);
			std::string alias(calias);
			si->temporaryServerAlias(alias);

			// does this exact server already exist in the user's list?
			std::string host(sh);
            std::string port(sp);
            if(ServerList::instance()->find(alias, host, port) == 0)
			{
				// no - add it, and make sure it's got a unique alias
				std::string uniqueName = ServerList::instance()->uniqueName(alias);
                ServerList::instance()->add(uniqueName, host, port, false, true);
			}
		}
	}
}


void SessionHandler::saveLastSessionName()
{
	std::string configDir = DirectoryHandler::configDir();
	std::string lastSessionFilename = DirectoryHandler::concatenate(configDir, "last_session.txt");

	// open the last_session.txt file and try to read it
	std::ofstream out(lastSessionFilename.c_str());

	if (out.good())
	{
		// the file is a one-line file containing the name of the current session
		std::string line = current()->name();
		out << line << std::endl;
	}
}

void SessionHandler::removeLastSessionName()
{
	std::string configDir = DirectoryHandler::configDir();
	std::string lastSessionFilename = DirectoryHandler::concatenate(configDir, "last_session.txt");

	std::string errorMessage;
	bool ok = DirectoryHandler::removeFile(lastSessionFilename, errorMessage);

	if (!ok)
	{
		UserMessage::message(UserMessage::ERROR, true, errorMessage);
	}
}


void SessionHandler::readLastSessionName()
{
	std::string configDir = DirectoryHandler::configDir();
	std::string lastSessionFilename = DirectoryHandler::concatenate(configDir, "last_session.txt");

	// open the last_session.txt file and try to read it
	std::ifstream in(lastSessionFilename.c_str());

	if (in.good())
	{
		// the file is a one-line file containing the name of the session we want
		std::string line;
		if (getline(in, line))
		{
			loadedLastSessionName_ = true;
			lastSessionName_ = line;
		}
		else
			lastSessionName_ = defaultSessionName();
	}
	else
	{
		// could not read the file, so just use the default
		lastSessionName_ = defaultSessionName();
	}


	// set this session as the current one if it exists
	SessionItem *item = find(lastSessionName_);
	if (item)
	{
		current(item);
	}
	else
	{
		lastSessionName_ = defaultSessionName();  // the last session file contained the name of a non-existant session
	}
}


bool SessionHandler::createSessionDirWithTemplate(const std::string &sessionName, const std::string &templateFile)
{
	// we want to:
	// 1) create a new directory for the session
	// 2) copy the session template JSON file across to the new dir

	std::string sessionDir = sessionDirName(sessionName);
	bool created = DirectoryHandler::createDir(sessionDir);

	if (created)
	{
		std::string errorMsg;
		std::string fullTemplatePath = DirectoryHandler::concatenate(DirectoryHandler::etcDir(), templateFile);
		std::string fullDestPath = DirectoryHandler::concatenate(sessionDir, "session.json");
		bool copied = DirectoryHandler::copyFile(fullTemplatePath, fullDestPath, errorMsg);
		if (!copied)
		{
			UserMessage::message(UserMessage::ERROR, true, errorMsg);
			return false;
		}
	}
	else
	{
		UserMessage::message(UserMessage::ERROR, true, "Could not create temporary session directory: " + sessionDir);
		return false;
	}

	return true;
}


SessionItem *SessionHandler::copySession(SessionItem* source, std::string &destName)
{
	// the main work is to make a copy of the source session's directory (recursively)
	std::string errorMessage;
	std::string sourceSessionDir = sessionDirName(source->name());
	std::string destSessionDir   = sessionDirName(destName);
	bool ok = DirectoryHandler::copyDir(sourceSessionDir, destSessionDir, errorMessage);
	if (ok)
	{
		// add it to our list
		SessionItem *newItem = add(destName);
		return newItem;
	}
	else
	{
		UserMessage::message(UserMessage::ERROR, true, errorMessage);
		return NULL;
	}
}

SessionItem *SessionHandler::copySession(std::string &source, std::string &destName)
{
	SessionItem *sourceSession = find(source);
	assert(sourceSession);

	return copySession(sourceSession, destName);
}

