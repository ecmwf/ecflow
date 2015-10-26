//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_NODEQUERY_HPP_
#define VIEWER_SRC_NODEQUERY_HPP_

#include <string>
#include <vector>

#include "VSettings.hpp"

struct NodeQueryOptions
{
	NodeQueryOptions() : maxNum_(-1), exactMatch_(false),
			caseSensitive_(false), regexp_(false), wildcard_(false) {}

	void load(VSettings*);
	void save(VSettings*);

	int  maxNum_;
	bool exactMatch_;
	bool caseSensitive_;
	bool regexp_;
	bool wildcard_;
};

class NodeQuery
{
public:
	explicit NodeQuery(const std::string& name);
	NodeQuery(const std::string&,const std::string& query);
	NodeQuery(const NodeQuery&);
	NodeQuery* clone();
	NodeQuery* clone(const std::string& name);

	void  setName(const std::string& name);
	const std::string& name() const {return name_;}

	void  setQuery(const std::string& query);
	const std::string& query() const {return query_;}

	void setRootNode(const std::string& rootNode) {rootNode_=rootNode;}
	const std::string& rootNode() const {return rootNode_;}

	void setServers(const std::vector<std::string>& servers) {servers_=servers;}
	const std::vector<std::string>& servers() const {return servers_;}

	void setOptions(const NodeQueryOptions& options) {options_=options;}
	const NodeQueryOptions& options() const {return options_;}

	void load(VSettings*);
	void save(VSettings*);

protected:
	void checkDir();

	std::string name_;
	std::string query_;
	NodeQueryOptions options_;
	std::string rootNode_;
	std::vector<std::string> servers_;
};

#endif /* VIEWER_SRC_NODEQUERY_HPP_ */
