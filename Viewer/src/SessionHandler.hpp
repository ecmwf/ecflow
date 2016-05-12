//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SESSIONHANDLER_HPP_
#define SESSIONHANDLER_HPP_

#include <string>
#include <vector>

class SessionItem
{
public:
	explicit SessionItem(const std::string&);
    virtual ~SessionItem() {}

	void  name(const std::string& name) {name_ = name;}
	const std::string& name() const {return name_;}

	std::string sessionFile() const;
	std::string windowFile() const;
	std::string settingsFile() const ;
	std::string recentCustomCommandsFile() const ;
	std::string savedCustomCommandsFile() const ;
	std::string serverFile(const std::string& serverName) const;
    std::string qtDir() const;
    std::string qtSettingsFile(const std::string name) const;

protected:
	void checkDir();

	std::string name_;
    std::string dirPath_;
    std::string qtPath_;
};

class SessionHandler
{
public:
	SessionHandler();

	SessionItem* add(const std::string&);
	void remove(const std::string&);
	void remove(SessionItem*);
	void rename(SessionItem*, const std::string&);
	void current(SessionItem*);
	SessionItem* current();
	void save();
	void load();
	int numSessions() {return sessions_.size();};
	SessionItem *find(const std::string&);
	int          indexFromName(const std::string&);
	SessionItem *sessionFromIndex(int i) {return sessions_[i];};
	SessionItem *copySession(SessionItem* source, std::string &destName);
	SessionItem *copySession(std::string &source, std::string &destName);
	void         saveLastSessionName();
	void         removeLastSessionName();
	bool         loadLastSessionAtStartup();
	std::string  lastSessionName() {return lastSessionName_;};


	const std::vector<SessionItem*>& sessions() const {return sessions_;}

	static std::string sessionDirName(const std::string &sessionName);    // static because they are called from the constructor
	static std::string sessionQtDirName(const std::string &sessionName);  // static because they are called from the constructor
	static SessionHandler* instance();
	static bool requestStartupViaSessionManager();

protected:
	void readSessionListFromDisk();
	std::string defaultSessionName() {return "default";};
	void readLastSessionName();

	static SessionHandler* instance_;

	std::vector<SessionItem*> sessions_;
	SessionItem* current_;
	bool loadedLastSessionName_;
	std::string lastSessionName_;
};

#endif
