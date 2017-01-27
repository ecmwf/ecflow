//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VAttributeType.hpp"

#include <QDebug>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>

#include "VNode.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VFilter.hpp"
#include "VRepeatAttr.hpp"
#include "VAttribute.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"

std::map<std::string,VAttributeType*> VAttributeType::typesMap_;
std::vector<VAttributeType*> VAttributeType::types_;

//#define _UI_ATTR_DEBUG

VAttributeType::VAttributeType(const std::string& name) :
        VParam(name),
        dataCount_(0),
        typeId_(types_.size())
{
    typesMap_[name]=this;
    types_.push_back(this);
}

std::vector<VParam*> VAttributeType::filterItems()
{
    std::vector<VParam*> v;
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
        v.push_back(*it);

    return v;
}

VAttributeType* VAttributeType::find(const std::string& name)
{
    std::map<std::string,VAttributeType*>::const_iterator it=typesMap_.find(name);
    if(it != typesMap_.end())
            return it->second;

    return 0;
}

VAttributeType* VAttributeType::find(int id)
{
    assert(id >=0  && id < types_.size());
    return types_[id];
}

#if 0
void VAttributeType::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
    {
        (*it)->scan(vnode,vec);
    }
}
#endif


int VAttributeType::totalNum(const VNode *vnode, AttributeFilter *filter)
{
    if(!vnode)
        return 0;

    int total=0;
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
    {        
        if(!filter || filter->isComplete() || filter->isSet(*it) )
        {
            total+=(*it)->num(vnode);
        }
        //If the filter contains a forceShow item
        else if(filter->matchForceShowAttr(vnode,*it))
        {
            total+=1;
        }
    }

    return total;
}

VAttributeType* VAttributeType::getType(const VNode *vnode,int absRowInFilter,AttributeFilter *filter)
{
    if(!vnode)
        return NULL;

    int totalRow=0;
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
    {
        if(!filter || filter->isSet(*it))
        {
            int size=(*it)->num(vnode);
            if(absRowInFilter-totalRow >=0 && absRowInFilter-totalRow < size)
            {
                return *it;
            }
            totalRow+=size;
        }
        else if(filter && filter->matchForceShowAttr(vnode,*it))
        {
            int size=1;
            if(absRowInFilter-totalRow >=0 && absRowInFilter-totalRow < size)
            {
                return *it;
            }
            totalRow+=size;
        }
    }

    return NULL;
}

bool VAttributeType::getData(VNode *vnode,int absRowInFilter,VAttributeType* &type,QStringList& data,AttributeFilter *filter)
{
    type=0;

    if(!vnode)
        return false;

    int totalRow=0;
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
    {
        if(!filter || filter->isSet(*it))
        {
            int size=0;
            if((*it)->getData(vnode,absRowInFilter-totalRow,size,data))
            {
                type=*it;
                return true;
            }
            totalRow+=size;
        }
        else if(filter && filter->matchForceShowAttr(vnode,*it))
        {
            if(absRowInFilter == totalRow)
            {
                VAttribute* a=filter->forceShowAttr();
                Q_ASSERT(a);
                data=a->data();
                type=*it;
                return true;
            }
            totalRow+=1;
        }
    }

    return false;
}

bool VAttributeType::getData(const std::string& type,VNode* vnode,int rowInType,QStringList& data)
{
    if(VAttributeType* va=find(type))
    {       
        int size=0;
        return va->getData(vnode,rowInType,size,data);
    }
    return false;
}


//This has to be very fast so we had to optimise it.
//getLineNum() gives 1 for all attributes but the
//labels. So we must guarantee that labels come first in the attribute vector
//and this vector do not check the other attributes.
int VAttributeType::getLineNum(const VNode *vnode,int absRowInFilter,AttributeFilter *filter)
{
    if(!vnode)
        return 1;

    VAttributeType *t=types_[0];
    if(!filter || filter->isComplete() || filter->isSet(t))
    {
        if(absRowInFilter < t->num(vnode))
        {
            return t->lineNum(vnode,absRowInFilter);
        }
    }

    return 1;
    //TODO:: add forceShowAtt
}

#if 0
int VAttributeType::getRow(const VNode *vnode,int row,AttributeFilter *filter)
{
    if(!vnode)
        return -1;

    if(!filter)
        return row;

    int totalRow=0;
    int realRow=0;
    for(std::map<std::string,VAttributeType*>::const_iterator it=typesMap_.begin(); it != typesMap_.end(); ++it)
    {
        if(!filter || filter->isSet(it->second))
        {
            int size=it->second->num(vnode);
            if(row-totalRow >=0 && row-totalRow < size)
            {
                return realRow+row-totalRow;
            }
            totalRow+=size;
            realRow+=size;
        }
        else
        {
            realRow+=it->second->num(vnode);
        }
    }

    return -1;
}
#endif

VItemTmp_ptr VAttributeType::itemForAbsIndex(const VNode *vnode,int absIndex,AttributeFilter *filter) //,VAttributeType* &type,int& indexInType)
{
    if(!vnode)
        return VItemTmp_ptr();

    int totalNum=0;
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
    {
        if(!filter || filter->isSet(*it))
        {
            int size=(*it)->num(vnode);
            if(absIndex-totalNum >=0 && absIndex-totalNum < size)
            {
                int indexInType=absIndex-totalNum;
                return VItemTmp::create(new VAttribute(const_cast<VNode*>(vnode),*it,indexInType));
            }
            totalNum+=size;
        }
        else if(filter && filter->matchForceShowAttr(vnode,*it))
        {
            int size=1;
            if(absIndex-totalNum >=0 && absIndex-totalNum < size)
            {
                VAttribute* a=filter->forceShowAttr();
                Q_ASSERT(a);
                return VItemTmp::create(a->clone());
            }
            totalNum+=size;
        }
    }

    return VItemTmp_ptr();
}

//Returns the absolute index of the given attribute within the whole list of attributes of a given node.
int VAttributeType::absIndexOf(const VAttribute* a,AttributeFilter *filter)
{
    if(!a)
        return -1;

    VNode* vnode=a->parent();
    if(!vnode)
        return -1;

    if(filter && !filter->isSet(a->type()) &&
       a->sameContents(filter->forceShowAttr()) == false)
        return -1;

    int absIndex=-1;
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
    {
        if(a->type() == *it)
        {
            int idx=-1;
            if(filter && a->sameContents(filter->forceShowAttr()))
                idx=0;
            else
                idx=(*it)->indexOf(a);

            return (idx != -1)?(absIndex+idx+1):-1;
        }

        if(!filter || filter->isSet(*it))
        {
            int size=(*it)->num(vnode);
            if(size > 0)
                absIndex+=size;
        }
        else if(filter->matchForceShowAttr(vnode,*it))
        {
            absIndex+=1;
        }
    }

    return -1;
}

//Returns the index of the given attribute within the given type
int VAttributeType::indexOf(const VAttribute* a)
{
    if(a && a->parent())
        return indexOf(a->parent(),a->data());

    return -1;
}

//Load the attributes parameter file
void VAttributeType::load(VProperty* group)
{
    //We set some extra information on each type and also
    //try to reorder the types according to the order defined in the
    //parameter file. This order is very important:
    // -it defines the rendering order
    // -defines the order of the attribute items in the attribute filter
    std::vector<VAttributeType*> v;
    Q_FOREACH(VProperty* p,group->children())
    {
         if(VAttributeType* obj=VAttributeType::find(p->strName()))
         {
            obj->setProperty(p);
            v.push_back(obj);
         }
         else
         {
             UI_ASSERT(0,"Unknown attribute type is read from parameter file: " << p->strName());
         }
    }

    UI_ASSERT(v.size() == types_.size(),"types size=" << types_.size() << "loaded size=" << v.size());

    //non debug version
    if(v.size() == types_.size())
        types_=v;
}

int VAttributeType::keyToDataIndex(const std::string& key) const
{
    std::map<std::string,int>::const_iterator it=keyToData_.find(key);
    if(it != keyToData_.end())
        return it->second;

    return -1;
}

int VAttributeType::searchKeyToDataIndex(const std::string& key) const
{
    std::map<std::string,int>::const_iterator it=searchKeyToData_.find(key);
    if(it != searchKeyToData_.end())
        return it->second;

    return -1;
}

QStringList VAttributeType::searchKeys() const
{
    QStringList lst;
    for(std::map<std::string,int>::const_iterator it=searchKeyToData_.begin(); it != searchKeyToData_.end(); it++)
    {
        lst << QString::fromStdString(it->first);
    }
    return lst;
}

void VAttributeType::items(const VNode* vnode,QList<VItemTmp_ptr>& lst)
{
    int cnt=num(vnode);
    for(int i=0; i < cnt; i++)
        lst << VItemTmp::create(new VAttribute(const_cast<VNode*>(vnode),this,i));
}

void VAttributeType::items(const std::string& type,const VNode* vnode,QList<VItemTmp_ptr>& lst)
{
    VAttributeType *t=VAttributeType::find(type);
    Q_ASSERT(t);
    t->items(vnode,lst);
}

VItemTmp_ptr VAttributeType::item(const VNode* vnode,const std::string& name)
{
    std::vector<std::string> nameVec;
    itemNames(vnode,nameVec);
    for(size_t i=0; i < nameVec.size(); i++)
        if(nameVec[i] == name)
            return VItemTmp::create(new VAttribute(const_cast<VNode*>(vnode),this,i));

    return VItemTmp_ptr();
}

//================================
// Meters
//================================

#if 0
class VMeterAttribute : public VAttributeType
{
public:
    explicit VMeterAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MinIndex=3, MaxIndex=4,ThresholdIndex=5};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const Meter& m,QStringList& data) const;
};

VMeterAttribute::VMeterAttribute(const std::string& n) :
    VAttributeType(n)
{
    dataCount_=6;
    searchKeyToData_["meter_name"]=NameIndex;
    searchKeyToData_["meter_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VMeterAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())?static_cast<int>(node->meters().size()):0;
}

bool VMeterAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VMeterAttribute::getData -->";
#endif
    const std::vector<Meter>&  v=node->meters();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }

    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

QString VMeterAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Meter<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Value:</b> " + d[ValueIndex]+ "<br>";
        t+="<b>Minimum:</b> " + d[MinIndex] + "<br>";
        t+="<b>Maximum:</b> " + d[MaxIndex];
    }
    return t;
}

int VMeterAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const std::vector<Meter>&  v=node->meters();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[NameIndex].toStdString())
            return i;
    }

    return -1;
}

void VMeterAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<Meter>& v=node->meters();
    for(size_t i=0; i < v.size(); i++)
    {
        nameVec.push_back(v[i].name());
    }
}

bool VMeterAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const std::vector<Meter>& v=node->meters();
    assert(index >=0 && index < v.size());
    getData(v[index],data);
    return true;
}

void VMeterAttribute::getData(const Meter& m,QStringList& data) const
{
    data << qName_ <<
                    QString::fromStdString(m.name()) <<
                    QString::number(m.value()) << QString::number(m.min()) << QString::number(m.max()) <<
                    QString::number(m.colorChange());
}

#endif

//================================
// Labels
//================================

#if 0
class VLabelAttribute : public VAttributeType
{
public:
    explicit VLabelAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    int lineNum(const VNode* vnode,int row);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const Label& label,QStringList& data);
};


VLabelAttribute::VLabelAttribute(const std::string& n) :
    VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["label_name"]=NameIndex;
    searchKeyToData_["label_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;   
}

int VLabelAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    return vnode->labelNum();
}

bool VLabelAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VLabelAttribute::getData -->";
#endif
    const std::vector<Label>&  v=node->labels();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }
    size=v.size();

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif

    return false;
}

int VLabelAttribute::lineNum(const VNode* vnode,int row)
{
    if(vnode->isServer())
        return 1;

    return vnode->labelLineNum(row);
}

QString VLabelAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Label<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Value:</b> " + d[ValueIndex];
    }
    return t;
}

int VLabelAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const std::vector<Label>&  v=node->labels();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[NameIndex].toStdString())
            return i;
    }

    return -1;
}

void VLabelAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<Label>& v=node->labels();
    for(size_t i=0; i < v.size(); i++)
    {
        nameVec.push_back(v[i].name());
    }
}

bool VLabelAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const std::vector<Label>& v=node->labels();
    UI_ASSERT(index >=0 && index < v.size(), "Index: " << UIDebug::longToString(index) << " v.size: " << v.size());
    getData(v[index],data);
    return true;
}

void VLabelAttribute::getData(const Label& label,QStringList& data)
{
    std::string val=label.new_value();
    if(val.empty() || val == " ")
    {
        val=label.value();
    }

    data << qName_ <<
                QString::fromStdString(label.name()) <<
                QString::fromStdString(val);
}

#endif

//================================
// Events
//================================
#if 0

class VEventAttribute : public VAttributeType
{
public:
    explicit  VEventAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MinIndex=3, MaxIndex=4,ThresholdIndex=5};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const Event& m,QStringList& data);

};

VEventAttribute::VEventAttribute(const std::string& n) :
    VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["event_name"]=NameIndex;
    searchKeyToData_["event_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VEventAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node)? static_cast<int>(node->events().size()):0;
}

bool VEventAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VEventAttribute::getData -->";
#endif
    const std::vector<Event>& v=node->events();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);

#if 0
        data << qName_ <<
                QString::fromStdString(v.at(row).name_or_number()) <<
                QString::number((v.at(row).value()==true)?1:0);
#endif

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }
    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif

    return false;
}

QString VEventAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Event<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Status:</b> ";
        t+=(d[ValueIndex] == "1")?"set (true)":"clear (false)";

    }
    return t;
}

int VEventAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const std::vector<Event>&  v=node->events();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name_or_number() == data[NameIndex].toStdString())
            return i;
    }

    return -1;
}

void VEventAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<Event>& v=node->events();
    for(size_t i=0; i < v.size(); i++)
    {
        nameVec.push_back(v[i].name_or_number());
    }
}

bool VEventAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const std::vector<Event>& v=node->events();
    UI_ASSERT(index >=0 && index < v.size(), "Index: " << UIDebug::longToString(index) << " v.size: " << v.size());
    getData(v[index],data);
    return true;
}

void VEventAttribute::getData(const Event& e,QStringList& data)
{
    data << qName_ <<
              QString::fromStdString(e.name_or_number()) <<
              QString::number((e.value()==true)?1:0);
}

#endif
//================================
//Generated Variables
//================================

#if 0
class VGenvarAttribute : public VAttributeType
{
public:
    explicit VGenvarAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList& data);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const Variable&,QStringList& data);
};

VGenvarAttribute::VGenvarAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["var_name"]=NameIndex;
    searchKeyToData_["var_value"]=ValueIndex;
    searchKeyToData_["var_type"]=TypeIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VGenvarAttribute::num(const VNode *vnode)
{
    return vnode->genVariablesNum();
}

bool VGenvarAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VGenvarAttribute::getData -->";
#endif

    std::vector<Variable> genV;
    vnode->genVariables(genV);

    if(row >=0 && row < genV.size())
    {
        getData(genV[row],data);
        //data << qName_ <<
        //        QString::fromStdString(genV.at(row).name()) <<
        //        QString::fromStdString(genV.at(row).theValue());
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }
    size=genV.size();

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

int VGenvarAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    std::vector<Variable> v;
    vnode->genVariables(v);

    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[NameIndex].toStdString())
           return i;
    }

    return -1;
}

void VGenvarAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    std::vector<Variable> v;
    vnode->genVariables(v);
    for(size_t i=0; i < v.size(); i++)
    {
        nameVec.push_back(v[i].name());
    }
}

bool VGenvarAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    std::vector<Variable> v;
    vnode->genVariables(v);
    assert(index >=0 && index  < v.size());
    getData(v[index],data);
    return true;
}

void VGenvarAttribute::getData(const Variable& v,QStringList& data)
{
    data << qName_ <<
            QString::fromStdString(v.name()) <<
            QString::fromStdString(v.theValue());
}

#endif


//================================
//Variables
//================================

#if 0
class VVarAttribute : public VAttributeType
{
public:
    explicit VVarAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const Variable&,QStringList& data);
};

VVarAttribute::VVarAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["var_name"]=NameIndex;
    searchKeyToData_["var_value"]=ValueIndex;
    searchKeyToData_["var_type"]=TypeIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VVarAttribute::num(const VNode *vnode)
{
    return vnode->variablesNum();
}

bool VVarAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VVarAttribute::getData -->";
#endif

    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        if(row >=0 && row < v.size())
        {
            getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
            UiLog().dbg() << "  data=" << data.join(",");
#endif
            return true;
        }
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  size=" << size;
#endif
        size=v.size();
    }
    else
    {
        node_ptr node=vnode->node();
        if(!node)
            return false;

        const std::vector<Variable>& v=node->variables();
        if(row >=0 && row < v.size())
        {
            getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
            UiLog().dbg() << "  data=" << data.join(",");
#endif
            return true;
        }
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  size=" << size;
#endif
        size=v.size();
    }

    return false;
}

int VVarAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == data[NameIndex].toStdString())
               return i;
        }
    }
    else
    {
        node_ptr node=vnode->node();
        if(!node)
            return -1;

        const std::vector<Variable>& v=node->variables();
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == data[NameIndex].toStdString())
                return i;
        }
    }

    return -1;
}

void VVarAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        for(size_t i=0; i < v.size(); i++)
        {
            nameVec.push_back(v[i].name());
        }
    }
    else
    {
        node_ptr node=vnode->node();
        if(!node)
            return;

        const std::vector<Variable>& v=node->variables();
        for(size_t i=0; i < v.size(); i++)
        {
            nameVec.push_back(v[i].name());
        }
    }
}

bool VVarAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);        
        getData(v[index],data);

    }
    else
    {
        node_ptr node=vnode->node();
        if(!node)
            return false;

        const std::vector<Variable>& v=node->variables();
        assert(index >=0 && index  < v.size());
        getData(v[index],data);       
    }

    return true;
}

void VVarAttribute::getData(const Variable& v,QStringList& data)
{
    data << qName_ <<
            QString::fromStdString(v.name()) <<
            QString::fromStdString(v.theValue());
}


#endif

//================================
// Limits
//================================

#if 0

class VLimitAttribute : public VAttributeType
{
public:
    explicit VLimitAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MaxIndex=3};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(limit_ptr lim,QStringList& data);
};

VLimitAttribute::VLimitAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=4;
    searchKeyToData_["limit_name"]=NameIndex;
    searchKeyToData_["limit_value"]=ValueIndex;
    searchKeyToData_["limit_max"]=MaxIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VLimitAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())?static_cast<int>(node->limits().size()):0;
}

bool VLimitAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VLimitAttribute::getData -->";
#endif

    const std::vector<limit_ptr>& v=node->limits();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif

        return true;
    }
    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif

    return false;
}

QString VLimitAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Limit<br>";
    if(d.count()  == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Value:</b> " + d[ValueIndex] + "<br>";
        t+="<b>Maximum:</b> " + d[MaxIndex];
    }
    return t;
}

int VLimitAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const std::vector<limit_ptr>& v=node->limits();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i]->name() == data[NameIndex].toStdString())
            return i;
    }

    return -1;
}

void VLimitAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<limit_ptr>& v=node->limits();
    for(size_t i=0; i < v.size(); i++)
    {
        nameVec.push_back(v[i]->name());
    }
}

bool VLimitAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const std::vector<limit_ptr>& v=node->limits();
    UI_ASSERT(index >=0 && index < v.size(), "index = " << UIDebug::longToString(index) << " v.size: " << UIDebug::longToString(v.size()));
    getData(v[index],data);
    return true;
}

void VLimitAttribute::getData(limit_ptr lim,QStringList& data)
{    
    data << qName_ <<
        QString::fromStdString(lim->name()) <<
        QString::number(lim->value()) <<
        QString::number(lim->theLimit());
}

#endif

//================================
//Limiters
//================================

#if 0

class VLimiterAttribute : public VAttributeType
{
public:
    explicit VLimiterAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,PathIndex=2};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const InLimit& lim,QStringList& data);
};

VLimiterAttribute::VLimiterAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["limiter_name"]=NameIndex;
    searchKeyToData_["limiter_path"]=PathIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VLimiterAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node)?static_cast<int>(node->inlimits().size()):0;
}

bool VLimiterAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VLimiterAttribute::getData -->";
#endif
    const std::vector<InLimit>& v=node->inlimits();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }
    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

QString VLimiterAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Limiter<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Limit:</b> " + d[NameIndex] + "<br>";
        t+="<b>Node:</b> " + d[PathIndex];

    }
    return t;
}

int VLimiterAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const std::vector<InLimit>& v=node->inlimits();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[NameIndex].toStdString() &&
           v[i].pathToNode() == data[PathIndex].toStdString())
            return i;
    }

    return -1;
}

void VLimiterAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<InLimit>& v=node->inlimits();
    for(size_t i=0; i < v.size(); i++)
    {
        nameVec.push_back(v[i].name());
    }
}

bool VLimiterAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const std::vector<InLimit>& v=node->inlimits();
    assert(index >=0 && index < v.size());
    getData(v[index],data);
    return true;
}

void VLimiterAttribute::getData(const InLimit& lim,QStringList& data)
{
    data << qName_ <<
           QString::fromStdString(lim.name()) <<
           QString::fromStdString(lim.pathToNode());
}

#endif
//================================
//Triggers
//================================

#if 0
class VTriggerAttribute : public VAttributeType
{
public:
    explicit VTriggerAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList data) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,CompleteIndex=1,ExprIndex=2};
    void itemNames(const VNode*,std::vector<std::string>&) {}
};

VTriggerAttribute::VTriggerAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["trigger_type"]=CompleteIndex;
    searchKeyToData_["trigger_expression"]=ExprIndex;
    searchKeyToData_["name"]=CompleteIndex;
}

int VTriggerAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;
    int num=(node->get_trigger())?1:0;
    num+=(node->get_complete())?1:0;
    return num;
}

bool VTriggerAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VTriggerAttribute::getData -->";
#endif

    Expression* eT=node->get_trigger();
    Expression* eC=node->get_complete();

    bool getTrigger=false;
    bool getComplete=false;

    if(row == 0)
    {
        if(eT)
            getTrigger=true;
        else if(eC)
            getComplete=true;
    }
    else if(row==1 && eC)
        getComplete=true;

    if(getTrigger)
    {
        data << qName_;
        data << "0" << QString::fromStdString(eT->expression());
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }
    else if(getComplete)
    {
        data << qName_;
        data << "1" << QString::fromStdString(eC->expression());
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }

    size=(eT)?1:0;
    size+=(eC)?1:0;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

int VTriggerAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    Expression* eT=node->get_trigger();
    Expression* eC=node->get_complete();

    int index=-1;
    if(eT)
    {
        index=0;
        if(data[CompleteIndex] == "0")
            return index;
    }

    if(eC)
    {
        if(index==0)
            index=1;
        else
            index=0;

        if(data[CompleteIndex] == "1")
            return index;
    }

    //TODO: we should check the expressions as well

    return -1;
}

bool VTriggerAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    Expression* eT=node->get_trigger();
    Expression* eC=node->get_complete();

    bool getTrigger=false;
    bool getComplete=false;

    if(index == 0)
    {
        if(eT)
            getTrigger=true;
        else if(eC)
            getComplete=true;
    }
    else if(index==1 && eC)
        getComplete=true;

    if(getTrigger)
    {
        data << qName_ << "0" << QString::fromStdString(eT->expression());
        return true;
    }
    else if(getComplete)
    {        
        data << qName_ << "1" << QString::fromStdString(eC->expression());     
    }

    return false;
}

QString VTriggerAttribute::toolTip(QStringList d) const
{
    QString t;
    if(d.count() == dataCount_)
    {
        if(d[CompleteIndex] == "0")
            t+="<b>Type:</b> Trigger<br>";
        else if(d[CompleteIndex] == "1")
            t+="<b>Type:</b> Complete<br>";
        else
            return t;

        t+="<b>Expression:</b> " + d[ExprIndex];
    }
    return t;
}

#endif

//================================
//Times
//================================

#if 0
class VTimeAttribute : public VAttributeType
{
public:
    explicit VTimeAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const ecf::TimeAttr& lim,QStringList& data);
    void getData(const ecf::TodayAttr& lim,QStringList& data);
    void getData(const ecf::CronAttr& lim,QStringList& data);
};

VTimeAttribute::VTimeAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=2;
    searchKeyToData_["time_name"]=NameIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VTimeAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node)?static_cast<int>(node->timeVec().size() + node->todayVec().size()+ node->crons().size()):0;
}

bool VTimeAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VTimeAttribute::getData -->";
#endif

    const std::vector<ecf::TimeAttr>& tV=node->timeVec();
    const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
    const std::vector<ecf::CronAttr>& cV=node->crons();

    if(row >=0 && row < tV.size()+tdV.size()+ cV.size())
    {
        if(row < tV.size())
            getData(tV[row],data);
        else if(row < tV.size() + tdV.size())
            getData(tdV[row-tV.size()],data);
        else
            getData(cV[row-tV.size()-tdV.size()],data);

#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }

    size=tV.size()+tdV.size()+ cV.size();
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

QString VTimeAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Time<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex];
    }
    return t;
}

int VTimeAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const std::vector<ecf::TimeAttr>& tV=node->timeVec();
    const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
    const std::vector<ecf::CronAttr>& cV=node->crons();

    int cnt=0;
    for(size_t i=0; i < tV.size(); i++)
    {
        if(tV[i].name() == data[NameIndex].toStdString())
            return i;
    }

    cnt+=tV.size();
    for(size_t i=0; i < tdV.size(); i++)
    {
        if(tdV[i].name() == data[NameIndex].toStdString())
            return cnt+i;
    }

    cnt+=tdV.size();
    for(size_t i=0; i < cV.size(); i++)
    {
        if(cV[i].name() == data[NameIndex].toStdString())
            return cnt+i;
    }

    return -1;
}

void VTimeAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<ecf::TimeAttr>& tV=node->timeVec();
    const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
    const std::vector<ecf::CronAttr>& cV=node->crons();

    for(size_t i=0; i < tV.size(); i++)
    {
        nameVec.push_back(tV[i].name());
    }
    for(size_t i=0; i < tdV.size(); i++)
    {
        nameVec.push_back(tdV[i].name());
    }
    for(size_t i=0; i < cV.size(); i++)
    {
        nameVec.push_back(cV[i].name());
    }
}

bool VTimeAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const std::vector<ecf::TimeAttr>& tV=node->timeVec();
    const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
    const std::vector<ecf::CronAttr>& cV=node->crons();

    if(index >=0 && index < tV.size()+tdV.size()+ cV.size())
    {
        if(index < tV.size())
            getData(tV[index],data);
        else if(index < tV.size() + tdV.size())
            getData(tdV[index-tV.size()],data);
        else
            getData(cV[index-tV.size()-tdV.size()],data);

        assert(!data.isEmpty());
    }

    return false;
}

void VTimeAttribute::getData(const ecf::TimeAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

void VTimeAttribute::getData(const ecf::TodayAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

void VTimeAttribute::getData(const ecf::CronAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

//================================
//Date
//================================

class VDateAttribute : public VAttributeType
{
public:
    explicit VDateAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;    
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const DateAttr& lim,QStringList& data);
    void getData(const DayAttr& lim,QStringList& data);
};

VDateAttribute::VDateAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=2;
    searchKeyToData_["date_name"]=NameIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VDateAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())?static_cast<int>(node->dates().size() + node->days().size()):0;
}

bool VDateAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VDateAttribute::getData -->";
#endif

    const std::vector<DateAttr>& dV=node->dates();
    const std::vector<DayAttr>& dayV=node->days();

    if(row >=0 && row < dV.size()+dayV.size())
    {
        if(row < dV.size())
            getData(dV[row],data);
        else
            getData(dayV[row-dV.size()],data);

#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif

        return true;
    }
    size=dV.size()+dayV.size();
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

QString VDateAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Date<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex];
    }
    return t;
}

int VDateAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const std::vector<DateAttr>& dV=node->dates();
    const std::vector<DayAttr>& dayV=node->days();

    int cnt=0;
    for(size_t i=0; i < dV.size(); i++)
    {
        if(dV[i].name() == data[NameIndex].toStdString())
            return i;
    }

    cnt+=dV.size();
    for(size_t i=0; i < dayV.size(); i++)
    {
        if(dayV[i].name() == data[NameIndex].toStdString())
            return cnt+i;
    }

    return -1;
}

void VDateAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<DateAttr>& dV=node->dates();
    const std::vector<DayAttr>& dayV=node->days();

    for(size_t i=0; i < dV.size(); i++)
    {
        nameVec.push_back(dV[i].name());
    }
    for(size_t i=0; i < dayV.size(); i++)
    {
        nameVec.push_back(dayV[i].name());
    }
}

bool VDateAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const std::vector<DateAttr>& dV=node->dates();
    const std::vector<DayAttr>& dayV=node->days();

    if(index >=0 && index < dV.size()+dayV.size())
    {
        QStringList  data;
        if(index < dV.size())
            getData(dV[index],data);
        else
            getData(dayV[index-dV.size()],data);

        assert(!data.isEmpty());
    }

    return false;
}

void VDateAttribute::getData(const DateAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

void VDateAttribute::getData(const DayAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

#endif

//================================
//Repeat
//================================

#if 0
class VRepeatAttribute : public VAttributeType
{
public:
    explicit VRepeatAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList&);

private:
    enum DataIndex {TypeIndex=0,SubtypeIndex=1,NameIndex=2,ValueIndex=3,StartIndex=4,EndIndex=5,StepIndex=6};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(const Repeat& r,QStringList& data);
};


VRepeatAttribute::VRepeatAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=7;
    searchKeyToData_["repeat_name"]=NameIndex;
    searchKeyToData_["repeat_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VRepeatAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())?((node->repeat().empty())?0:1):0;
}

bool VRepeatAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VRepeatAttribute::getData -->";
#endif

    const Repeat& r=node->repeat();
    if(row ==0 && !r.empty())
    {
        getData(r,data);
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }
    size=(r.empty())?0:1;
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

QString VRepeatAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Repeat";
    if(d.count() == dataCount_)
    {
        t+=" " + d[SubtypeIndex] + "<br>";

        if(d[SubtypeIndex] != "day")
        {
            t+="<b>Name:</b> " + d[NameIndex] + "<br>";
            t+="<b>Value:</b> " + d[ValueIndex] + "<br>";
            t+="<b>Start:</b> " + d[StartIndex] + "<br>";
            t+="<b>End:</b> " + d[EndIndex] + "<br>";
            t+="<b>Step:</b> " + d[StepIndex];
        }
        else
        {
            t+="<b>Step:</b> " + d[StepIndex];
        }
    }

    return t;
}

int VRepeatAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    const Repeat& r=node->repeat();
    if(r.name() == data[NameIndex].toStdString())
    {
        if(VRepeat::type(r) == data[SubtypeIndex].toStdString())
            return 0;
    }

    return -1;
}

void VRepeatAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const Repeat& r=node->repeat();
    if(!r.empty())
        nameVec.push_back(r.name());
}

bool VRepeatAttribute::itemData(const VNode* vnode,int index,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    const Repeat& r=node->repeat();
    if(index == 0 && !r.empty())
    {
        getData(r,data);
        return true;
    }

    return false;
}

void VRepeatAttribute::getData(const Repeat& r,QStringList& data)
{
    //We try to avoid creating a VRepeat object everytime we are here
    std::string type=VRepeat::type(r);

    data << qName_ << QString::fromStdString(type) <<
         QString::fromStdString(r.name()) <<
         QString::fromStdString(r.valueAsString()) <<
         QString::fromStdString(r.value_as_string(r.start())) <<
         QString::fromStdString(r.value_as_string(r.end())) <<
         QString::number(r.step());
}

#endif
//================================
//Late
//================================

#if 0

class VLateAttribute : public VAttributeType
{
public:
    explicit VLateAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    int indexOf(const VNode* vnode,QStringList) const;
    bool itemData(const VNode*,int index,QStringList& data);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
    void itemNames(const VNode* vnode,std::vector<std::string>& nameVec);
    void getData(ecf::LateAttr *late,QStringList& data);
};


VLateAttribute::VLateAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=2;   
    searchKeyToData_["late_name"]=NameIndex;
    searchKeyToData_["late_type"]=TypeIndex;
    searchKeyToData_["name"]=NameIndex;
}

int VLateAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())?((node->get_late())?1:0):0;
}

bool VLateAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "VLateAttribute::getData -->";
#endif

    ecf::LateAttr *late=node->get_late();
    if(row ==0 && late)
    {
        getData(late,data);
#ifdef _UI_ATTR_DEBUG
        UiLog().dbg() << "  data=" << data.join(",");
#endif
        return true;
    }
    size=(late)?1:0;
#ifdef _UI_ATTR_DEBUG
    UiLog().dbg() << "  size=" << size;
#endif
    return false;
}

QString VLateAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Late<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex];
    }
    return t;
}

int VLateAttribute::indexOf(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return -1;

    node_ptr node=vnode->node();
    if(!node)
        return -1;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return -1;

    ecf::LateAttr *late=node->get_late();
    if(late && late->name() == data[NameIndex].toStdString())
    {
        return 0;
    }

    return -1;
}

void VLateAttribute::itemNames(const VNode* vnode,std::vector<std::string>& nameVec)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    ecf::LateAttr *late=node->get_late();
    if(late)
        nameVec.push_back(late->name());
}

bool VLateAttribute::itemData(const VNode* vnode,int /*index*/,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    ecf::LateAttr *late=node->get_late();
    if(late)
    {
        getData(late,data);
        return true;
    }

    return false;
}

void VLateAttribute::getData(ecf::LateAttr *late,QStringList& data)
{
    if(late)
        data << qName_ << QString::fromStdString(late->name());
}

#endif

//The order below must not be changed. LABEL has to come first: it is
//necessary for getLineNum(). Genvar should always come last: it is
//the slowest to access.

//static VConcreteAttributeType<VLabel> labelAttr("label");

//static VLabelAttribute labelAttr("label");
//static VMeterAttribute meterAttr("meter");
//static VEventAttribute eventAttr("event");
//static VRepeatAttribute repeatAttr("repeat");
//static VTriggerAttribute triggerAttr("trigger");
//static VTimeAttribute timeAttr("time");
//static VDateAttribute dateAttr("date");
//static VLimitAttribute limitAttr("limit");
//static VLimiterAttribute limiterAttr("limiter");
//static VLateAttribute lateAttr("late");
//static VVarAttribute varAttr("var");
//static VGenvarAttribute genvarAttr("genvar");

static SimpleLoader<VAttributeType> loader("attribute");
