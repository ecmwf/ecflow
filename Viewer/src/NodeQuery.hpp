//============================================================================
// Copyright 2016 ECMWF.
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

#include <QColor>
#include <QStringList>
#include <QMap>

#include "StringMatchMode.hpp"
#include "VSettings.hpp"

class NodeQuery;
class NodeQueryAttributeTerm;
class VAttributeType;
class VProperty;

//===============================================
//
// Definition terms
//
//===============================================

class NodeQueryDef
{
public:
    NodeQueryDef(VProperty* p);
    virtual void buildOption(NodeQuery*)=0;
    QString name() const {return name_;}
    QString label() const {label_;}
    QStringList values() const {return values_;}
    QStringList valueLabels() const {return valueLabels_;}

protected:
    QString name_;
    QString label_;
    QStringList values_;
    QStringList valueLabels_;
};

class NodeQueryStringDef : public NodeQueryDef
{
public:
    NodeQueryStringDef(VProperty* p) : NodeQueryDef(p) {}
    void buildOption(NodeQuery*);
};

class NodeQueryListDef : public NodeQueryDef
{
public:
    NodeQueryListDef(VProperty* p);
    QStringList values();
    void buildOption(NodeQuery*);
};

class NodeQueryComboDef : public NodeQueryDef
{
public:
    NodeQueryComboDef(VProperty* p);
    QStringList values();
    void buildOption(NodeQuery*);
};

class NodeQueryAttrDef : public NodeQueryDef
{
friend class NodeQuery;

public:
    NodeQueryAttrDef(VProperty* p);
    void buildOption(NodeQuery*);
    bool hasType(VAttributeType* t) const {return types_.contains(t);}
    QList<NodeQueryDef*> defs() const {return defs_;}

protected:
    QList<NodeQueryDef*> defs_;
    QList<VAttributeType*> types_;
};

//===============================================
//
// Query options
//
//===============================================

class NodeQueryOption
{
public:
    NodeQueryOption(NodeQueryDef *def) : def_(def) {}

    QString name() const {return def_->name();}
    QString label() const {return def_->label();}

    virtual void swap(const NodeQueryOption*)=0;
    virtual QString query() const {return QString();}
    virtual QString query(QString op) const {return QString();}
    virtual void load(VSettings*)=0;
    virtual void save(VSettings*)=0;

protected:
    NodeQueryDef *def_;
};

class NodeQueryStringOption : public NodeQueryOption
{
public:
    NodeQueryStringOption(NodeQueryDef *def);
    void swap(const NodeQueryOption*);

	QString value() const {return value_;}
	const StringMatchMode&  matchMode() const {return matchMode_;}
	QString matchOperator() const {return QString::fromStdString(matchMode_.matchOperator());}
	bool caseSensitive() const {return caseSensitive_;}

	void setValue(QString s) {value_=s;}
	void setMatchMode(StringMatchMode::Mode m) {matchMode_.setMode(m);}
	void setMatchMode(const StringMatchMode& m) {matchMode_=m;}
	void setCaseSensitive(bool b) {caseSensitive_=b;}

    QString query() const;
	void load(VSettings*);
	void save(VSettings*);

protected:
    QString value_;
	StringMatchMode matchMode_;
	bool caseSensitive_;
    
    static StringMatchMode::Mode defaultMatchMode_;
    static bool defaultCaseSensitive_;
};

class NodeQueryListOption : public NodeQueryOption
{
public:
    NodeQueryListOption(NodeQueryDef *def) : NodeQueryOption(def) {}

    void swap(const NodeQueryOption*);

    QString query(QString op) const;
    void load(VSettings*);
	void save(VSettings*);

    QStringList values() const {return def_->values();}
    QStringList valueLabels() const {return def_->valueLabels();}
    void setSelection(QStringList lst) {selection_=lst;}
	QStringList selection() const {return selection_;}

protected:
	QStringList selection_;
};

class NodeQueryComboOption : public NodeQueryOption
{
public:
    NodeQueryComboOption(NodeQueryDef *def) : NodeQueryOption(def) {}

    void swap(const NodeQueryOption*);

    QString query(QString op) const;
    void load(VSettings*);
    void save(VSettings*);

    QStringList values() const {return def_->values();}
    QStringList valueLabels() const {return def_->valueLabels();}
    void setValue() const;
    QString value() const {return value_;}

protected:
    QString value_;
};

//===============================================
//
// NodeQuery
//
//===============================================

class NodeQuery
{
friend class  NodeQueryStringDef;
friend class  NodeQueryListDef;
friend class  NodeQueryComboDef;

public:
	NodeQuery(const std::string& name,bool ignoreMaxNum=false);
	~NodeQuery();
    NodeQuery* clone();
	NodeQuery* clone(const std::string& name);

	void swap(const NodeQuery*);

	void  setName(const std::string& name);
	const std::string& name() const {return name_;}

	QString query() const;
    QString nodeQueryPart() const;
    QString attrQueryPart() const;
    QString attrQueryPart(VAttributeType*) const;
    bool hasAttribute(VAttributeType*) const;

	void setRootNode(const std::string& rootNode) {rootNode_=rootNode;}
	const std::string& rootNode() const {return rootNode_;}
	void setServers(QStringList servers) {servers_=servers;}	
	QStringList servers() const {return servers_;}
	bool hasServer(const std::string& name) const;

	QString extQueryHtml(bool multi,QColor bgCol,int firstColWidth) const;
	void buildQueryString();

	int maxNum() const {return maxNum_;}
	void setMaxNum(int m) {maxNum_=m;}
	bool ignoreMaxNum() const {return ignoreMaxNum_;}

	void setCaseSensitive(bool b) {caseSensitive_=b;}
	bool caseSensitive() const {return caseSensitive_;}

    NodeQueryOption* option(QString name) const;
    static NodeQueryDef* def(QString name);
    static QMap<QString,NodeQueryAttrDef*> attrDef() {return attrDef_;}

    //Called from VConfigLoader
    static void load(VProperty* group);

	void load(VSettings*);
	void save(VSettings*);

protected:
	void checkDir();

	std::string name_;
	bool advanced_;	
	std::string rootNode_;
	QStringList servers_;
	bool allServers_;
    QMap<QString,QString> extQuery_;
    bool caseSensitive_;
    int maxNum_;
    bool ignoreMaxNum_;

    QMap<QString,NodeQueryOption*> options_;

    static QMap<QString,NodeQueryDef*> def_;
    static QMap<QString,NodeQueryAttrDef*> attrDef_;
	static bool defaultCaseSensitive_;
	static int defaultMaxNum_;
};

#endif /* VIEWER_SRC_NODEQUERY_HPP_ */
