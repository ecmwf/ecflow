//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQuery.hpp"

#include "VSettings.hpp"

void NodeQueryOptions::load(VSettings* s)
{
	maxNum_=s->get<int>("maxNum",maxNum_);
	exactMatch_=s->getAsBool("exactMatch",exactMatch_);
	caseSensitive_=s->getAsBool("caseSensistive",caseSensitive_);
	regexp_=s->getAsBool("regexp",regexp_);
	wildcard_=s->getAsBool("wildcard",wildcard_);
}

void NodeQueryOptions::save(VSettings* s)
{
	s->put("maxNum",maxNum_);
	s->putAsBool("exactMatch",exactMatch_);
	s->putAsBool("caseSensistive",caseSensitive_);
	s->putAsBool("regexp",regexp_);
	s->putAsBool("wildcard",wildcard_);
}

NodeQuery::NodeQuery(const std::string& name) :
  name_(name)
{

}

NodeQuery::NodeQuery(const std::string& name,const std::string& query) :
  name_(name),
  query_(query)
{

}

NodeQuery::NodeQuery(const NodeQuery& q)
{
	name_=q.name_;
	query_=q.query_;
	rootNode_=q.rootNode_;
	servers_=q.servers_;
	options_=q.options_;
}

NodeQuery* NodeQuery::clone()
{
	return clone(name_);
}

NodeQuery* NodeQuery::clone(const std::string& name)
{
	NodeQuery *q=new NodeQuery(name);
	q->query_=query_;
	q->rootNode_=rootNode_;
	q->servers_=servers_;
	q->options_=options_;

	return q;
}

void  NodeQuery::setName(const std::string& name)
{
	name_=name;
}

void  NodeQuery::setQuery(const std::string& query)
{
	query_=query;
}

void NodeQuery::load(VSettings* s)
{
	options_.load(s);
	s->get("servers",servers_);
	rootNode_=s->get("rootNode",rootNode_);
	query_=s->get("query",query_);
}

void NodeQuery::save(VSettings* s)
{
	options_.save(s);
	s->put("servers",servers_);
	s->put("rootNode",rootNode_);
	s->put("query",query_);
}

