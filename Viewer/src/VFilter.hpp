//============================================================================
// Copyright 2016 ECMWF.
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

#include "VInfo.hpp"
#include "VParam.hpp"

#include "Node.hpp"

class NodeQuery;
class NodeFilterEngine;
class ServerFilter;
class ServerHandler;
class VAttribute;
class VAttributeType;
class VInfo;
class VNode;
class VSettings;
class VTree;

#include <boost/property_tree/ptree.hpp>

class VParamSet : public QObject
{
Q_OBJECT

public:
	VParamSet();
    virtual ~VParamSet() {}

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
    void virtual readSettings(VSettings* vs);

Q_SIGNALS:
	void changed();

protected:
	void init(const std::vector<VParam*>& items);

	std::set<VParam*> all_;
	std::set<VParam*> current_;
    std::string settingsId_;
    std::string settingsIdV0_;
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
    bool matchForceShowAttr(const VNode*,VAttributeType*) const;
    void setForceShowAttr(const VAttribute* a);
    void clearForceShowAttr();
    VAttribute* forceShowAttr() const;

private:
    VInfo_ptr forceShowAttr_;
};

class IconFilter : public VParamSet
{
public:
    IconFilter();
    void readSettings(VSettings* vs);
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
    friend class NodeFilterEngine;
    friend class VTreeServer;

public:
    enum MatchMode {NoneMatch,AllMatch,VectorMatch};

    NodeFilter(NodeFilterDef* def,ServerHandler*);
	virtual ~NodeFilter();

    virtual void clear();
    virtual bool isNull()=0;
    virtual bool isComplete()=0;
    virtual int  matchCount() const = 0;
    virtual bool update()=0;

    VNode* forceShowNode() const {return forceShowNode_;}
    void setForceShowNode(VNode*);
    void clearForceShowNode();

protected:
    NodeFilterDef* def_;
    NodeFilterEngine* queryEngine_;
    std::set<std::string> type_;
    MatchMode matchMode_;
    std::vector<VNode*> match_;
    ServerHandler * server_;
    VNode* forceShowNode_;
};

class TreeNodeFilter : public NodeFilter
{
public:
    explicit TreeNodeFilter(NodeFilterDef* def,ServerHandler*,VTree*);

    void clear();
    bool isNull();
    bool isComplete();
    int  matchCount() const {return 0;}
    bool update();
    bool update(const std::vector<VNode*>& topChange,
                std::vector<VNode*>& topFilterChange);

private:
	bool filterState(VNode* node,VParamSet* stateFilter);
    bool collectTopFilterChange(VNode* n,std::vector<VNode*>& topFilterChange);

    VTree* tree_;
};

class TableNodeFilter : public NodeFilter
{
public:
    explicit TableNodeFilter(NodeFilterDef* def,ServerHandler*);

	void clear();
    bool isNull();
    bool isComplete();
    int  matchCount() const {return matchCount_;}
    bool update();
    int indexOf(const VNode*) const;
    VNode* nodeAt(int index) const;

private:
    std::vector<int> index_;
    int matchCount_;
};

#endif
