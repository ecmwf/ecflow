//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEINFOQUERY_HPP_
#define NODEINFOQUERY_HPP_

#include <string>

#include <boost/shared_ptr.hpp>

#include <QMetaType>

#include "Node.hpp"

class NodeInfoAccessor;

class NodeInfoQuery
{
public:
	enum Type {NONE,SCRIPT,JOB,JOBOUT,MESSAGE,MANUAL};

	NodeInfoQuery(Node* n,Type t,NodeInfoAccessor* sender) : node_(n), type_(t), sender_(sender), done_(false) {};
	~NodeInfoQuery() {}

	bool readFile();
	void text(const std::vector<std::string>& msg);

	Node* node() const {return node_;}
	const std::string ecfVar() const {return ecfVar_;}
	const std::string ciPar() const {return ciPar_;}
	NodeInfoAccessor* sender() const {return sender_;}
	bool done() const {return done_;}
	const std::string fileName() const {return fileName_;}
	const std::string errorText() const {return errorText_;}
	const std::string text() const {return text_;}

	void ecfVar(const std::string& s) {ecfVar_=s;}
	void ciPar(const std::string& s) {ciPar_=s;}
	Type type() const {return type_;}
	void done(bool b) {done_=b;}
	void fileName(const std::string s) {fileName_=s;}
	void errorText(const std::string s) {errorText_=s;}
	void text(const std::string s) {text_=s;}

protected:
	Node* node_;
	Type type_;
	NodeInfoAccessor *sender_;
	std::string ecfVar_;
	std::string ciPar_;

	bool done_;
	std::string fileName_;
	std::string errorText_;
	std::string text_;
};

typedef boost::shared_ptr<NodeInfoQuery> NodeInfoQuery_ptr;


class NodeInfoAccessor
{
public:
	virtual ~NodeInfoAccessor() {};
	virtual void queryFinished(NodeInfoQuery_ptr) {};
};


#endif
