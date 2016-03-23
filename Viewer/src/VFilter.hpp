//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VIEWFILTER_HPP_
#define VIEWFILTER_HPP_

#include <set>
#include <vector>

#include <QObject>
#include <QStringList>

#include "VParam.hpp"

#include "Node.hpp"

class NodeQuery;
class NodeFilterEngine;
class ServerFilter;
class ServerHandler;
class VNode;
class VSettings;

#include <boost/property_tree/ptree.hpp>

class VParamSet : public QObject
{
Q_OBJECT

public:
	VParamSet();
	virtual ~VParamSet() {};

	const std::set<VParam*>& current() const {return current_;}
	void setCurrent(const std::set<VParam*>&);
	QStringList currentAsList() const;
	void current(const std::set<std::string>&);
	void setCurrent(QStringList);
	const std::set<VParam*>& all() const {return all_;}

	bool isEmpty() const {return current_.empty();}
	bool isComplete() const { return all_.size() == current_.size();}
	bool isSet(const std::string&) const;
	bool isSet(VParam*) const;

	void writeSettings(VSettings* vs);
	void readSettings(VSettings* vs);

Q_SIGNALS:
	void changed();

protected:
	void init(const std::vector<VParam*>& items);

	std::set<VParam*> all_;
	std::set<VParam*> current_;
	std::string settingsId_;
};

class NodeStateFilter : public VParamSet
{
public:
	NodeStateFilter();
};

class AttributeFilter : public VParamSet
{
public:
	AttributeFilter();
};

class IconFilter : public VParamSet
{
public:
	IconFilter();
};


class TreeNodeFilter;
class TableNodeFilter;

class NodeFilterDef : public QObject
{
Q_OBJECT

friend class  TreeNodeFilter;
friend class  TableNodeFilter;

public:
	enum Scope {NodeStateScope,GeneralScope};
	NodeFilterDef(ServerFilter*,Scope);
	~NodeFilterDef();

	NodeStateFilter* nodeState() const {return nodeState_;}

	const std::string& exprStr() const {return exprStr_;}
	NodeQuery* query() const;
	void setQuery(NodeQuery*);

	void writeSettings(VSettings *vs);
	void readSettings(VSettings *vs);

Q_SIGNALS:
	void changed();

protected:
	ServerFilter *serverFilter_;
	std::string exprStr_;
	NodeStateFilter *nodeState_;
	std::string nodePath_;
	std::string nodeType_;
	NodeQuery* query_;

	//AttributeFilter *attribute_;
	//std::string nodeType_;
	//std::string nodeName_;
};


class NodeFilter
{
public:
	enum ChangeAspect {AllChanged,StateChanged,AttributeChanged};
	enum ResultMode {StoreMatched,StoreNonMatched};
	enum MatchMode {NoneMatch,AllMatch,VectorMatch};

	NodeFilter(NodeFilterDef* def,ResultMode resultMode);
	virtual ~NodeFilter();

	virtual void clear()=0;
    //virtual void beginReset(ServerHandler* server)=0;
    //virtual void endReset()=0;
    //virtual bool update(const VNode*)=0;

	virtual bool isNull()=0;
    //virtual bool isFiltered(VNode* node)=0;
    virtual int  matchCount()=0;
    //virtual int  nonMatchCount()=0;
    //virtual VNode* matchAt(int i)=0;
    //virtual int matchPos(const VNode*)=0;

   // virtual int realMatchCount()=0;
    //virtual VNode* realMatchAt(int)=0;

    std::set<VNode*> match_;
    std::set<VNode*> nonMatch_;
    std::vector<bool> res_;
    std::vector<VNode*> nodes_;

protected:
    NodeFilterDef* def_;
    std::set<std::string> type_;
    ResultMode resultMode_;
	std::set<VNode*> result_;
	bool beingReset_;
	NodeFilterEngine* queryEngine_;
	MatchMode matchMode_;
};

class TreeNodeFilter : public NodeFilter
{
public:
	explicit TreeNodeFilter(NodeFilterDef* def);

    void clear();
	void beginReset(ServerHandler* server);
	void endReset();
	bool update(const VNode*);

	bool isNull();
	bool isFiltered(VNode* node);
	int  matchCount();
	//int  nonMatchCount();
	//VNode* matchAt(int i) {return NULL;}
	//int matchPos(const VNode*) {return -1;}

	//int realMatchCount();
	//VNode* realMatchAt(int);

private:
	bool filterState(VNode* node,VParamSet* stateFilter);
	//std::vector<VNode*> match_;
	//std::set<VNode*> nonMatch_;
};

class TableNodeFilter : public NodeFilter
{
public:
	explicit TableNodeFilter(NodeFilterDef* def);

	void clear();
	void beginReset(ServerHandler* server);
	void endReset();
	bool update(const VNode*);
    bool update(ServerHandler* server,const std::vector<VNode>&stateChangeSuites);

	bool isNull();
	bool isFiltered(VNode* node);
	int  matchCount();
	//int  nonMatchCount() {return -1;}
	//VNode* matchAt(int i);
	//int matchPos(const VNode*);

	//int realMatchCount();
	//VNode* realMatchAt(int);

private:
	//std::vector<VNode*> match_;
};

#endif
