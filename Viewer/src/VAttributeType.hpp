//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VATTRIBUTETYPE_HPP_
#define VATTRIBUTETYPE_HPP_

#include <set>
#include <vector>
#include <string>

#include "VParam.hpp"

class AttributeFilter;
class VNode;

class VAttributeType : public VParam
{
public:
    explicit VAttributeType(const std::string& name);
    virtual ~VAttributeType() {}
    
	static std::vector<VParam*> filterItems();
	
    static VAttributeType* getType(const VNode *vnode,int row,AttributeFilter *filter=0);
    static bool getData(VNode* vnode,int row,VAttributeType* &type,QStringList& data,AttributeFilter *filter=0);
    static bool getData(const std::string& type,VNode* vnode,int row,QStringList& data,AttributeFilter *filter=0);
    static int totalNum(const VNode *vnode,AttributeFilter *filter=0);
    static void init(const std::string& parFile);
    static int getLineNum(const VNode *vnode,int row,AttributeFilter *filter=0);
    static int getRow(const VNode *vnode,int row,AttributeFilter *filter=0);

    static VAttributeType* find(const std::string& name);

    //Called from VConfigLoader
    static void load(VProperty*);

protected:
	virtual bool getData(VNode *vnode,int row,int& totalRow,QStringList& data)=0;
	virtual int num(const VNode* vnode)=0;
	virtual int lineNum(const VNode* vnode,int row) {return 1;}

private:
    static std::map<std::string,VAttributeType*> items_;
};

#endif
