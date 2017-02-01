//============================================================================
// Copyright 2009-2017 ECMWF.
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

class ServerItem;


class SessionItem
{
public:
	explicit SessionItem(const std::string&);
	virtual ~SessionItem();

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
	void temporary(bool t) {isTemporary_ = t;};
	bool temporary() {return isTemporary_;};
	void temporaryServerAlias(const std::string &alias) {temporaryServerAlias_ = alias;};
	std::string temporaryServerAlias() {return temporaryServerAlias_;};
	void askToPreserveTemporarySession(bool a) {askToPreserveTemporarySession_ = a;};
	bool askToPreserveTemporarySession() {return askToPreserveTemporarySession_;};

protected:
	void checkDir();

	std::string name_;
	std::string dirPath_;
	std::string qtPath_;
	std::string temporaryServerAlias_;
	bool isTemporary_;
	bool askToPreserveTemporarySession_;
};

class SessionHandler
{
public:
	SessionHandler();
	~SessionHandler();

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
	bool         createSessionDirWithTemplate(const std::string &sessionName, const std::string &templateFile);
	void         saveLastSessionName();
	void         removeLastSessionName();
	bool         loadLastSessionAtStartup();
	std::string  lastSessionName() {return lastSessionName_;};


	const std::vector<SessionItem*>& sessions() const {return sessions_;}

	static std::string sessionDirName(const std::string &sessionName);    // static because they are called from the constructor
	static std::string sessionQtDirName(const std::string &sessionName);  // static because they are called from the constructor
	static SessionHandler* instance();
	static void destroyInstance();
	static bool requestStartupViaSessionManager();
	static void setTemporarySessionIfReqested();

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
