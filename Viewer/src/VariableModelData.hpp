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
	explicit VariableModelData(VInfo_ptr info);
	virtual ~VariableModelData() {};

	std::string fullPath();
	std::string name();
	std::string type();
	const std::string& name(int index) const;
	const std::string& value(int index) const;
	bool isGenVar(int index) const;
	int varNum() const;
	bool hasName(const std::string& n) const;

	void buildAlterCommand(std::vector<std::string>& cmd,
			           const std::string& action, const std::string& type,
			           const std::string& name,const std::string& value);

    void clear();
	void reload();
	void setValue(int index,const std::string& val);
	void add(const std::string& name,const std::string& val);
	void remove(int index,const std::string& val);

	int checkUpdateDiff();
	bool update();

	std::vector<Variable> vars_;
	std::vector<Variable> genVars_;

	VInfo_ptr info_;
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
	void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>&);
	void defsChanged(const std::vector<ecf::Aspect::Type>&);

Q_SIGNALS:
	void reloadBegin();
	void reloadEnd();
	void addRemoveBegin(int,int);
	void addRemoveEnd(int);
	void dataChanged(int);

protected:
	void reload();

	std::vector<VariableModelData*> data_;
	ServerHandler* server_;
};

#endif
