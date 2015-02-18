//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VARIABLEMODELDATA_H
#define VARIABLEMODELDATA_H

#include <vector>

#include <QColor>
#include <QObject>

#include "NodeObserver.hpp"
#include "VInfo.hpp"

class Node;
class ServerHandler;


class VariableModelData
{
public:
	virtual ~VariableModelData() {};

	virtual const std::string& name()=0;
	virtual QColor colour()=0;
	const std::string& name(int index) const;
	const std::string& value(int index) const;
	bool isGenVar(int index) const;
	int varNum() const;

	virtual void reload()=0;

	std::vector<std::pair<std::string,std::string> > vars_;
	std::vector<std::pair<std::string,std::string> > genVars_;
};

class VariableServerData : public VariableModelData
{
public:
	VariableServerData(ServerHandler*);
	const std::string& name();
	QColor colour();

protected:
	void reload();
	ServerHandler* server_;
};


class VariableNodeData : public VariableModelData
{
public:
	VariableNodeData(Node*);
	const std::string& name();
	QColor colour();

protected:
	void reload();
	Node* node_;
};


class VariableModelDataHandler : public QObject
{
Q_OBJECT

public:
	VariableModelDataHandler();
	~VariableModelDataHandler();

	void reload(VInfo_ptr info);
	void clear();
	int count() const {return static_cast<int>(data_.size());}
	int varNum(int index) const;
	VariableModelData* data(int index) const;

Q_SIGNALS:
	void reloadBegin();
	void reloadEnd();

protected:
	std::vector<VariableModelData*> data_;
	void nodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>&);

	ServerHandler* server_;

};


#endif
