//============================================================================
// Copyright 2015 ECMWF.
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
	virtual ~SessionItem() {};

	void  name(const std::string& name);
	const std::string& name() const {return name_;}

	std::string sessionFile() const;
	std::string windowFile() const;
	std::string settingsFile() const ;
	std::string recentCustomCommandsFile() const ;
	std::string savedCustomCommandsFile() const ;
	std::string serverFile(const std::string& serverName) const;

protected:
	void checkDir();

	std::string name_;
	std::string dirPath_;
};

class SessionHandler
{
public:
	SessionHandler();

	SessionItem* add(const std::string&);
	void remove(const std::string&);
	void remove(SessionItem*);
	void current(SessionItem*);
	SessionItem* current();
	void save();
	void load();

	const std::vector<SessionItem*>& sessios() const {return sessions_;}

	static SessionHandler* instance();

protected:
	static SessionHandler* instance_;

	std::vector<SessionItem*> sessions_;
	SessionItem* current_;
};

#endif
