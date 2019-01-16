//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_NODEQUERYHANDLER_HPP_
#define VIEWER_SRC_NODEQUERYHANDLER_HPP_

#include <string>
#include <vector>

class NodeQuery;

class NodeQueryHandler
{
public:
	NodeQueryHandler();

	NodeQuery* add(const std::string& name);
	NodeQuery* add(const std::string& name,const std::string& query);
	void add(NodeQuery* item,bool save);
	void remove(const std::string& name);
	void remove(NodeQuery*);
	NodeQuery* find(const std::string& name) const;

	void save();
	void save(NodeQuery*);
	void init(const std::string& dirPath);
	const std::vector<NodeQuery*>& items() const {return items_;}

	static NodeQueryHandler* instance();

protected:
	static NodeQueryHandler* instance_;

	std::string dirPath_;
	const std::string suffix_;
	std::vector<NodeQuery*> items_;
};


#endif /* VIEWER_SRC_NODEQUERYHANDLER_HPP_ */
