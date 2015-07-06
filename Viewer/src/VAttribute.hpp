//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VATTRIBUTE_HPP_
#define VATTRIBUTE_HPP_

#include <set>
#include <vector>
#include <string>

#include "VParam.hpp"

class VNode;

class VAttribute : public VParam
{
public:
	explicit VAttribute(const std::string& name);
	virtual ~VAttribute() {};
    
	static std::vector<VParam*> filterItems();
	
	static bool getType(VNode *vnode,int row,VAttribute **type);
	static bool getData(VNode* vnode,int row,VAttribute** type,QStringList& data);
	static int totalNum(const VNode *vnode);
	static void init(const std::string& parFile);
	
	static VAttribute* find(const std::string& name);
    
    //Called from VConfigLoader
    static void load(VProperty*);

protected:
	virtual bool getData(VNode *vnode,int row,int& totalRow,QStringList& data)=0;
	virtual int num(const VNode* vnode)=0;


private:
	static std::map<std::string,VAttribute*> items_;
};

#endif
