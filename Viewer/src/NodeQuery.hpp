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

#include <QStringList>
#include <QMap>

#include "VSettings.hpp"

class NodeQuery;

class NodeQueryStringOption
{
    friend class NodeQuery;

public:
	NodeQueryStringOption(QString name);
	void swap(const NodeQueryStringOption*);

	enum MatchMode {ContainsMatch=0,WildcardMatch=1,RegexpMatch=2};

	QString name() const {return name_;}
	QString value() const {return value_;}
	MatchMode matchMode() const {return matchMode_;}
	QString matchOperator() const;
	int matchModeAsInt() const {return static_cast<int>(matchMode_);}
	bool caseSensitive() const {return caseSensitive_;}

	void setValue(QString s) {value_=s;}
	void setMatchMode(MatchMode m) {matchMode_=m;}
	void setCaseSensitive(bool b) {caseSensitive_=b;}

	void load(VSettings*);
	void save(VSettings*);

protected:
	QString name_;
	QString value_;
	MatchMode matchMode_;
	bool caseSensitive_;
    
    static MatchMode defaultMatchMode_;
    static bool defaultCaseSensitive_;
	static QMap<MatchMode,QString> matchOper_;
};

class NodeQuerySelectOption
{
	friend class NodeQuery;

public:
	NodeQuerySelectOption(QString name) : name_(name) {}
	void swap(const NodeQuerySelectOption*);

	void load(VSettings*);
	void save(VSettings*);

	QString name() const {return name_;}
	QStringList selection() const {return selection_;}

protected:
	QString name_;
	QStringList selection_;
};


class NodeQuery
{
public:
	explicit NodeQuery(const std::string& name);
	~NodeQuery();
    NodeQuery* clone();
	NodeQuery* clone(const std::string& name);

	void swap(const NodeQuery*);

	void  setName(const std::string& name);
	const std::string& name() const {return name_;}

	void  setQuery(QString);
	const QString query() const {return query_;}

	void setRootNode(const std::string& rootNode) {rootNode_=rootNode;}
	const std::string& rootNode() const {return rootNode_;}

	void setServers(QStringList servers) {servers_=servers;}
	//const std::vector<std::string>& servers() const {return servers_;}
	QStringList servers() const {return servers_;}
	bool hasServer(const std::string& name) const;

	//void setOptions(const NodeQueryOptions& options) {options_=options;}
	//const NodeQueryOptions& options() const {return options_;}

	QString queryString(bool update=true);
	QString extQueryString() const {return extQuery_;}
	void buildQueryString();

	int maxNum() const {return maxNum_;}
	void setMaxNum(int m) {maxNum_=m;}

	void setCaseSensitive(bool b) {caseSensitive_=b;}
	bool caseSensitive() const {return caseSensitive_;}

	NodeQueryStringOption* stringOption(QString name) const;

	QStringList typeSelection() const;
	QStringList stateSelection() const;
	QStringList flagSelection() const;
	QStringList attrGroupSelection() const;

	void setTypeSelection(QStringList);
	void setStateSelection(QStringList);
	void setFlagSelection(QStringList);
	void setAttrGroupSelection(QStringList);

	static QStringList typeTerms() {return typeTerms_;}
	static QStringList stateTerms() {return stateTerms_;}
	static QStringList flagTerms() {return flagTerms_;}
	static QStringList attrGroupTerms() {return attrGroupTerms_;}
	static QStringList attrTerms(QString group) {return attrTerms_.value(group);}

	void load(VSettings*);
	void save(VSettings*);

protected:
	void checkDir();

	std::string name_;
	bool advanced_;	
	std::string rootNode_;
	QStringList servers_;
    QString query_;
    QString extQuery_;
    bool caseSensitive_;
    int maxNum_;

	QMap<QString,NodeQueryStringOption*> stringOptions_;
	QMap<QString,NodeQuerySelectOption*> selectOptions_;

	static QStringList nodeTerms_;
	static QStringList typeTerms_;
	static QStringList stateTerms_;
	static QStringList flagTerms_;
	static QStringList attrGroupTerms_;
	static QMap<QString,QStringList> attrTerms_;

	static bool defaultCaseSensitive_;
	static int defaultMaxNum_;
};

#endif /* VIEWER_SRC_NODEQUERY_HPP_ */
