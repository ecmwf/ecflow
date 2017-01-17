//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VATTRIBUTETYPE_HPP_
#define VATTRIBUTETYPE_HPP_

#include <map>
#include <vector>
#include <string>

#include "VItemTmp.hpp"
#include "VParam.hpp"

class AttributeFilter;
class VNode;
class VAttribute;

class VAttributeType : public VParam
{
public:
    explicit VAttributeType(const std::string& name);
    virtual ~VAttributeType() {}

    static std::vector<VParam*> filterItems();

    static VAttributeType* getType(const VNode *vnode,int row,AttributeFilter *filter=0);
    static bool getData(VNode* vnode,int row,VAttributeType* &type,QStringList& data,AttributeFilter *filter=0);
    static bool getData(const std::string& type,VNode* vnode,int row,QStringList& data);
    static int totalNum(const VNode *vnode,AttributeFilter *filter=0);
    static void init(const std::string& parFile);
    static int getLineNum(const VNode *vnode,int row,AttributeFilter *filter=0);
#if 0
    static int getRow(const VNode *vnode,int row,AttributeFilter *filter=0);
#endif
    static VItemTmp_ptr itemForAbsIndex(const VNode *vnode,int absIndex,AttributeFilter *filter);

    static VAttributeType* find(const std::string& name);
    static VAttributeType* find(int id);
    static const std::vector<VAttributeType*>& types() {return types_;}
    
    //Called from VConfigLoader
    static void load(VProperty*);

    virtual QString toolTip(QStringList d) const {return QString();}

    static int absIndexOf(const VAttribute*,AttributeFilter *filter=0);
    int indexOf(const VAttribute*);
    virtual int indexOf(const VNode* vnode,QStringList data) const=0;
    bool exists(const VNode* vnode,QStringList data) { return (indexOf(vnode,data) != -1); }

    void items(const VNode* vnode,QList<VItemTmp_ptr>& lst);
    static void items(const std::string& type,const VNode* vnode,QList<VItemTmp_ptr>& lst);
    VItemTmp_ptr item(const VNode*,const std::string&);
    virtual bool itemData(const VNode*,int index,QStringList&)=0;

    int id() const {return id_;}
    int keyToDataIndex(const std::string& key) const;
    int searchKeyToDataIndex(const std::string& key) const;
    QStringList searchKeys() const;

protected:
    virtual void itemNames(const VNode* node,std::vector<std::string>&)=0;
    virtual bool getData(VNode *vnode,int row,int& totalRow,QStringList& data)=0;
    virtual int num(const VNode* vnode)=0;
    virtual int lineNum(const VNode* vnode,int row) {return 1;}

    std::map<std::string,int> keyToData_;
    std::map<std::string,int> searchKeyToData_;
    int dataCount_;
    int id_;

private:
    static std::map<std::string,VAttributeType*> typesMap_;
    static std::vector<VAttributeType*> types_;
};

#endif
