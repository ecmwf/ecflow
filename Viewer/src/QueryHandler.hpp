//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_QUERYHANDLER_HPP_
#define VIEWER_SRC_QUERYHANDLER_HPP_


#include <string>
#include <vector>

class QueryItem
{
public:
	explicit QueryItem(const std::string& name);
	QueryItem(const std::string&,const std::string& query);
	virtual ~QueryItem() {};

	void  setName(const std::string& name);
	const std::string& name() const {return name_;}

	void  setQuery(const std::string& query);
	const std::string& query() const {return query_;}

	//std::string sessionFile() const;
	//std::string windowFile() const;
	//std::string settingsFile() const ;
	//std::string serverFile(const std::string& serverName) const;

protected:
	void checkDir();

	std::string name_;
	std::string query_;
};

class QueryHandler
{
public:
	QueryHandler();

	QueryItem* add(const std::string& name);
	QueryItem* add(const std::string& name,const std::string& query);
	void remove(const std::string& name);
	void remove(QueryItem*);
	void save();
	void init(const std::string& dirPath);
	const std::vector<QueryItem*>& items() const {return items_;}

	static QueryHandler* instance();

protected:
	static QueryHandler* instance_;

	std::string dirPath_;
	std::vector<QueryItem*> items_;
};


#endif /* VIEWER_SRC_QUERYHANDLER_HPP_ */
