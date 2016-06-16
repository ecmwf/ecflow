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

#include <QStringList>
#include <QMap>

#include "StringMatchMode.hpp"
#include "VSettings.hpp"

class NodeQuery;
class NodeQueryOption;
class VAttributeType;

class NodeQueryAttrGroup
{
friend class NodeQuery;

public:
    NodeQueryAttrGroup(QString name,QList<VAttributeType*> types,QList<NodeQueryOption*> options) :
        name_(name), types_(types), options_(options) {}

    QString name() const {return name_;}
    bool hasType(VAttributeType* t) const {return types_.contains(t);}
    QList<NodeQueryOption*> options() const {return options_;}
    QString query() const;

protected:
    QString name_;
    QList<VAttributeType*> types_;
    QList<NodeQueryOption*> options_;
};

class NodeQuery
{
friend class  NodeQueryOption;

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
    QStringList attrSelection() const;

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
    QMap<QString,NodeQueryAttrGroup*> attrGroup() {return attrGroup_;}

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
    QMap<QString,NodeQueryAttrGroup*> attrGroup_;
	static bool defaultCaseSensitive_;
	static int defaultMaxNum_;
};

#endif /* VIEWER_SRC_NODEQUERY_HPP_ */
