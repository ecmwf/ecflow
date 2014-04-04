//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERITEM_HPP_
#define SERVERITEM_HPP_

#include <string>
#include <vector>

class ServerItem
{
public:
	ServerItem(const std::string&);
	ServerItem(const std::string&,const std::string&,const std::string&);
	virtual ~ServerItem() {};

	ServerItem* clone() const;
	void  name(const std::string& name) {name_=name;}
	void  host(const std::string& name) {host_=name;}
	void  port(const std::string& name) {port_=name;}

	const std::string& name() const {return name_;}
	const std::string& host() const {return host_;}
	const std::string& port() const {return port_;}

	virtual bool match(ServerItem*);

protected:
	std::string name_;
	std::string host_;
	std::string port_;
};

/*class ServerViewItem: public ServerItem
{
public:
	ServerViewItem(const std::string&,const std::string&,const std::string&);
	bool hasSuiteFilter();
	const std::vector<node_ptr>&  suiteFilter() const {return suiteFilter_;}
	void addToSuiteFilter(node_ptr);
	void removeFromSuiteFilter(node_ptr);

protected:
	std::vector<node_ptr> suiteFilter_;

};*/

#endif
