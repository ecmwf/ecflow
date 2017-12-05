//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VARIABLEMODELDATA_H
#define VARIABLEMODELDATA_H

#include <vector>
#include <set>

#include <QColor>
#include <QObject>

#include "NodeObserver.hpp"
#include "VInfo.hpp"

class Node;
class ServerHandler;
class VariableModelDataHandler;
class VariableModelDataObserver;

class VariableModelData
{
   friend class VariableModelDataHandler;

public:
	explicit VariableModelData(VInfo_ptr info);
	virtual ~VariableModelData();

	std::string fullPath();
    std::string name();
	std::string type();
	const std::string& name(int index) const;
    const std::string& value(int index) const;
    const std::string& value(const std::string name,bool&) const;
    VInfo_ptr info() const {return info_;}
    VInfo_ptr info(int index) const;
    int indexOf(const std::string& varName,bool genVar) const;
    bool isGenVar(const std::string& varName) const;
    bool isGenVar(int index) const;
	bool isReadOnly(int index) const;
    bool isReadOnly(const std::string& varName) const;
    bool isShadowed(int index) const;
    int varNum() const;
	bool hasName(const std::string& n) const;
	VNode* node() const;

#if 0
	void buildAlterCommand(std::vector<std::string>& cmd,
			           const std::string& action, const std::string& type,
			           const std::string& name,const std::string& value);
#endif
    void clear();  
	void setValue(int index,const std::string& val);
    void alter(const std::string& name,const std::string& val);
	void add(const std::string& name,const std::string& val);
    void remove(const std::string& val);

    //const std::vector<Variable>& vars() const {return vars_;}
    //const std::vector<Variable>& genVars() const {return genVars_;}

protected:
    void reload();
    void removeDuplicates(const std::vector<Variable>& vars,std::vector<Variable>& genVars);
    void latestVars(std::vector<Variable>& v,std::vector<Variable>& gv);
    int checkUpdateDiff(std::vector<Variable>& v,std::vector<Variable>& gv);
    bool checkUpdateNames(const std::vector<Variable>& v,const std::vector<Variable>& vg);
    void reset(const std::vector<Variable>& v,const std::vector<Variable>& gv);
    bool update(const std::vector<Variable>& v,const std::vector<Variable>& gv);
    bool updateShadowed(std::set<std::string>& names);

	std::vector<Variable> vars_;
	std::vector<Variable> genVars_;
    std::set<std::string> shadowed_;
	VInfo_ptr info_;
};

class VariableModelDataHandler : public QObject
{
Q_OBJECT

public:
	VariableModelDataHandler();
	~VariableModelDataHandler();

    void reload(VInfo_ptr info);
    void clear(bool emitSignal=true);
	int count() const {return static_cast<int>(data_.size());}
	int varNum(int index) const;
	VariableModelData* data(int index) const;
    void findVariable(const std::string& name,const std::string& nodePath,
                                                bool genVar,int& block,int& row) const;

    void findVariable(VInfo_ptr info,int& block,int& row) const;
    void findBlock(VInfo_ptr info,int& block) const;

    bool nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>&);
    bool defsChanged(const std::vector<ecf::Aspect::Type>&);
    const std::string& value(const std::string& node,const std::string& name,bool&) const;
    void addObserver(VariableModelDataObserver*);
    void removeObserver(VariableModelDataObserver*);


Q_SIGNALS:
	void reloadBegin();
	void reloadEnd();
    void clearBegin(int,int);
    void clearEnd(int,int);
    void loadBegin(int,int);
    void loadEnd(int,int);
	void addRemoveBegin(int,int);
    void addRemoveEnd(int);
    void dataChanged(int);
    void rerunFilter();

protected:
    //void reload();
    bool updateVariables(int);
    bool updateShadowed();
    void broadcastClear();
    void broadcastUpdate();

	std::vector<VariableModelData*> data_;
	ServerHandler* server_;
    std::set<std::string> names_;
    std::vector<VariableModelDataObserver*> observers_;
};

#endif
