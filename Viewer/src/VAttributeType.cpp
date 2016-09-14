//============================================================================
// Copyright 2016 ECMWF.
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
#include "UserMessage.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VFilter.hpp"
#include "VRepeat.hpp"
#include "VAttribute.hpp"

std::map<std::string,VAttributeType*> VAttributeType::items_;
std::vector<VAttributeType*> VAttributeType::types_;

//#define _UI_ATTR_DEBUG

VAttributeType::VAttributeType(const std::string& name) :
        VParam(name),
        dataCount_(0),
        id_(types_.size())
{
    //items_.push_back(this);
    items_[name]=this;
    types_.push_back(this);
}

std::vector<VParam*> VAttributeType::filterItems()
{
    std::vector<VParam*> v;
    for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        v.push_back(it->second);
    }

    return v;
}

VAttributeType* VAttributeType::find(const std::string& name)
{
    std::map<std::string,VAttributeType*>::const_iterator it=items_.find(name);
    if(it != items_.end())
            return it->second;

    return 0;

    /*		for(std::vector<VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
    {
        if((*it)->stdName() == name)
                return *it;
    }

    return NULL;*/
}

VAttributeType* VAttributeType::find(int id)
{
    assert(id >=0  && id < types_.size());
    return types_[id];
}

int VAttributeType::totalNum(const VNode *vnode, AttributeFilter *filter)
{
    if(!vnode)
        return 0;

    int total=0;
    for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if(!filter || filter->isSet(it->second))
        {
            total+=it->second->num(vnode);
        }
    }

    return total;
}

VAttributeType* VAttributeType::getType(const VNode *vnode,int row,AttributeFilter *filter)
{
    if(!vnode)
        return NULL;

    int totalRow=0;
    for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if(!filter || filter->isSet(it->second))
        {
            int size=it->second->num(vnode);
            if(row-totalRow >=0 && row-totalRow < size)
            {
                return it->second;
            }
            totalRow+=size;
        }
    }

    return NULL;
}

bool VAttributeType::getData(VNode *vnode,int row,VAttributeType* &type,QStringList& data,AttributeFilter *filter)
{
    type=0;

    if(!vnode)
        return false;

    int totalRow=0;
    for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if(!filter || filter->isSet(it->second))
        {
            int size=0;
            if(it->second->getData(vnode,row-totalRow,size,data))
            {
                type=it->second;
                return true;
            }
            totalRow+=size;
        }
    }

    return false;
}

bool VAttributeType::getData(const std::string& type,VNode* vnode,int row,QStringList& data,AttributeFilter *filter)
{
    if(VAttributeType* va=find(type))
    {
        if(!filter || filter->isSet(va))
        {
            int size=0;
            return va->getData(vnode,row,size,data);
        }
    }
    return false;
}


int VAttributeType::getLineNum(const VNode *vnode,int row,AttributeFilter *filter)
{
    if(!vnode)
        return 1;

    int totalRow=0;
    for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
    {
        if(!filter || filter->isSet(it->second))
        {
            int size=it->second->num(vnode);
            if(row-totalRow >=0 && row-totalRow < size)
            {
                return it->second->lineNum(vnode,row-totalRow);
            }
            totalRow+=size;
        }
    }

    return 1;
}

int VAttributeType::getRow(const VNode *vnode,int row,AttributeFilter *filter)
{
    if(!vnode)
        return -1;

    if(!filter)
        return row;

    int totalRow=0;
    int realRow=0;
    for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
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

void VAttributeType::load(VProperty* group)
{
    Q_FOREACH(VProperty* p,group->children())
    {
         if(VAttributeType* obj=VAttributeType::find(p->strName()))
         {
            obj->setProperty(p);
         }
    }
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

void VAttributeType::getSearchData(const std::string& type,const VNode* vnode,QList<VAttribute*>& lst)
{
    VAttributeType *t=VAttributeType::find(type);
    Q_ASSERT(t);
    t->getSearchData(vnode,lst);
}


//================================
// Meters
//================================

class VMeterAttribute : public VAttributeType
{
public:
    explicit VMeterAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;

    VAttribute* getSearchData(const VNode*,const std::string&);
    VAttribute* getSearchData(const VNode*,int index);
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MinIndex=3, MaxIndex=4,ThresholdIndex=5};
    void getData(const Meter& m,QStringList& data);
};

VMeterAttribute::VMeterAttribute(const std::string& n) :
    VAttributeType(n)
{
    dataCount_=6;
    searchKeyToData_["meter_name"]=NameIndex;
    searchKeyToData_["meter_value"]=ValueIndex;
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
    UserMessage::debug("VMeterAttribute::getData -->");
#endif
    const std::vector<Meter>&  v=node->meters();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }

    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

bool VMeterAttribute::exists(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return false;

    const std::vector<Meter>&  v=node->meters();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[NameIndex].toStdString())
            return true;
    }

    return false;
}

VAttribute* VMeterAttribute::getSearchData(const VNode* vnode,const std::string& name)
{
    if(vnode->isServer())
        return NULL;

    node_ptr node=vnode->node();
    if(!node)
        return NULL;

    const std::vector<Meter>& v=node->meters();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == name)
        {
            QStringList data;
            getData(v[i],data);
            return new VAttribute(const_cast<VNode*>(vnode),this,data,i);
        }
    }

    return NULL;
}

VAttribute* VMeterAttribute::getSearchData(const VNode* vnode,int index)
{
    if(vnode->isServer())
        return NULL;

    node_ptr node=vnode->node();
    if(!node)
        return NULL;

    const std::vector<Meter>& v=node->meters();
    assert(index >=0 && index < v.size());

    QStringList data;
    getData(v[index],data);
    return new VAttribute(const_cast<VNode*>(vnode),this,data,index);
}

void VMeterAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;
    
    const std::vector<Meter>& v=node->meters();
    for(size_t i=0; i < v.size(); i++)
    {    
        QStringList data;
        getData(v[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data,i);
    } 
}    

void VMeterAttribute::getData(const Meter& m,QStringList& data)
{
    data << qName_ <<
                    QString::fromStdString(m.name()) <<
                    QString::number(m.value()) << QString::number(m.min()) << QString::number(m.max()) <<
                    QString::number(m.colorChange());
}

//================================
// Labels
//================================

class VLabelAttribute : public VAttributeType
{
public:
    explicit VLabelAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    int lineNum(const VNode* vnode,int row);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
    void getData(const Label& label,QStringList& data);
};


VLabelAttribute::VLabelAttribute(const std::string& n) :
    VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["label_name"]=NameIndex;
    searchKeyToData_["label_value"]=ValueIndex;
}

int VLabelAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())?static_cast<int>(node->labels().size()):0;
}

bool VLabelAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("VLabelAttribute::getData -->");
#endif
    const std::vector<Label>&  v=node->labels();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }
    size=v.size();

#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
#endif

    return false;
}

int VLabelAttribute::lineNum(const VNode* vnode,int row)
{
    if(vnode->isServer())
        return 1;

    node_ptr node=vnode->node();
    if(!node.get())
        return 1;

    const std::vector<Label>&  v=node->labels();
    if(row >=0 && row < v.size())
    {
        std::string val=v[row].new_value();
        if(val.empty() || val == " ")
        {
            val=v[row].value();
        }
        return std::count(val.begin(), val.end(), '\n')+1;
    }

    return 1;
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

bool VLabelAttribute::exists(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return false;

    const std::vector<Label>&  v=node->labels();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[NameIndex].toStdString())
            return true;
    }

    return false;
}

void VLabelAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<Label>& v=node->labels();
    for(std::vector<Label>::const_iterator it=v.begin(); it != v.end(); ++it)
    {
        QStringList data;
        getData(*it,data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
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


//================================
// Events
//================================


class VEventAttribute : public VAttributeType
{
public:
    explicit  VEventAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MinIndex=3, MaxIndex=4,ThresholdIndex=5};
    void getData(const Event& m,QStringList& data);

};

VEventAttribute::VEventAttribute(const std::string& n) :
    VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["event_name"]=NameIndex;
    searchKeyToData_["event_value"]=ValueIndex;
}


int VEventAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())? static_cast<int>(node->events().size()):0;
}

bool VEventAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
            return false;

#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("VEventAttribute::getData -->");
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
    UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }
    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

bool VEventAttribute::exists(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return false;

    const std::vector<Event>&  v=node->events();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name_or_number() == data[NameIndex].toStdString())
            return true;
    }

    return false;
}

void VEventAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<Event>& v=node->events();
    for(std::vector<Event>::const_iterator it=v.begin(); it != v.end(); ++it)
    {
        QStringList data;
        getData(*it,data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
}

void VEventAttribute::getData(const Event& e,QStringList& data)
{
    data << qName_ <<
              QString::fromStdString(e.name_or_number()) <<
              QString::number((e.value()==true)?1:0);
}


//================================
//Generated Variables
//================================

class VGenvarAttribute : public VAttributeType
{
public:
    explicit VGenvarAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    bool exists(const VNode* vnode,QStringList) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
    void getData(const Variable&,QStringList& data);
};

VGenvarAttribute::VGenvarAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["var_name"]=NameIndex;
    searchKeyToData_["var_value"]=ValueIndex;
    searchKeyToData_["var_type"]=TypeIndex;
}


int VGenvarAttribute::num(const VNode *vnode)
{
    return vnode->genVariablesNum();
}

bool VGenvarAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("VGenvarAttribute::getData -->");
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
    UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }
    size=genV.size();

#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
#endif
    return false;
}

bool VGenvarAttribute::exists(const VNode* vnode,QStringList data) const
{
    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return false;

    std::vector<Variable> v;
    vnode->genVariables(v);

    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[NameIndex].toStdString())
           return true;
    }

    return false;
}

void VGenvarAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    std::vector<Variable> v;
    vnode->genVariables(v);
    for(size_t i=0; i < v.size(); i++)
    {
        QStringList data;
        getData(v[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data,i);
    }
}

void VGenvarAttribute::getData(const Variable& v,QStringList& data)
{
    data << qName_ <<
            QString::fromStdString(v.name()) <<
            QString::fromStdString(v.theValue());
}

//================================
//Variables
//================================

class VVarAttribute : public VAttributeType
{
public:
    explicit VVarAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    bool exists(const VNode* vnode,QStringList) const;
    VAttribute* getSearchData(const VNode*,const std::string&);
    VAttribute* getSearchData(const VNode*,int index);
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
    void getData(const Variable&,QStringList& data);
};

VVarAttribute::VVarAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["var_name"]=NameIndex;
    searchKeyToData_["var_value"]=ValueIndex;
    searchKeyToData_["var_type"]=TypeIndex;
}

int VVarAttribute::num(const VNode *vnode)
{
    return vnode->variablesNum();
}

bool VVarAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("VVarAttribute::getData -->");
#endif

    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        if(row >=0 && row < v.size())
        {
            getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
            UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
            return true;
        }
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  size=" + QString::number(size).toStdString());
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
            UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
            return true;
        }
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  size=" + QString::number(size).toStdString());
#endif
        size=v.size();
    }

    return false;
}

bool VVarAttribute::exists(const VNode* vnode,QStringList data) const
{
    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return false;

    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == data[NameIndex].toStdString())
               return true;
        }
    }
    else
    {
        node_ptr node=vnode->node();
        if(!node)
            return false
            
            
            ;
        
        const std::vector<Variable>& v=node->variables();
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == data[NameIndex].toStdString())
                return true;
        }
    }

    return false;
}


VAttribute* VVarAttribute::getSearchData(const VNode* vnode,const std::string& name)
{
    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == name)
            {
                QStringList data;
                getData(v[i],data);
                return new VAttribute(const_cast<VNode*>(vnode),this,data,i);
            }
        }
    }
    else
    {
        node_ptr node=vnode->node();
        if(!node)
            return NULL;

        const std::vector<Variable>& v=node->variables();
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == name)
            {
                QStringList data;
                getData(v[i],data);
                return new VAttribute(const_cast<VNode*>(vnode),this,data,i);
            }
        }
    }
}

VAttribute* VVarAttribute::getSearchData(const VNode* vnode,int index)
{
    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        QStringList data;
        getData(v[index],data);
        return new VAttribute(const_cast<VNode*>(vnode),this,data,index);
    }
    else
    {
        node_ptr node=vnode->node();
        if(!node)
            return NULL;

        const std::vector<Variable>& v=node->variables();
        assert(index >=0 && index  < v.size());

        QStringList data;
        getData(v[index],data);
        return new VAttribute(const_cast<VNode*>(vnode),this,data,index);
    }
}


void VVarAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        for(size_t i=0; i < v.size(); i++)
        {
            QStringList data;
            getData(v[i],data);
            lst << new VAttribute(const_cast<VNode*>(vnode),this,data,i);
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
            QStringList data;
            getData(v[i],data);
            lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
        }
     }
}

void VVarAttribute::getData(const Variable& v,QStringList& data)
{
    data << qName_ <<
            QString::fromStdString(v.name()) <<
            QString::fromStdString(v.theValue());
}

//================================
// Limits
//================================

class VLimitAttribute : public VAttributeType
{
public:
    explicit VLimitAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
    VAttribute* getSearchData(const VNode*,const std::string&);
    VAttribute* getSearchData(const VNode*,int index);
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MaxIndex=3};
    void getData(limit_ptr lim,QStringList& data);
};

VLimitAttribute::VLimitAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=4;
    searchKeyToData_["limit_name"]=NameIndex;
    searchKeyToData_["limit_value"]=ValueIndex;
    searchKeyToData_["limit_max"]=MaxIndex;
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
    UserMessage::debug("VLimitAttribute::getData -->");
#endif

    const std::vector<limit_ptr>& v=node->limits();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif

        return true;
    }
    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

bool VLimitAttribute::exists(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return false;

    const std::vector<limit_ptr>& v=node->limits();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i]->name() == data[NameIndex].toStdString())
            return true;
    }

    return false;
}

VAttribute* VLimitAttribute::getSearchData(const VNode* vnode,const std::string& name)
{
    if(vnode->isServer())
        return NULL;

    node_ptr node=vnode->node();
    if(!node)
        return NULL;

    const std::vector<limit_ptr>& v=node->limits();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i]->name()== name)
        {
            QStringList data;
            getData(v[i],data);
            return new VAttribute(const_cast<VNode*>(vnode),this,data,i);
        }
    }

    return NULL;
}

VAttribute* VLimitAttribute::getSearchData(const VNode* vnode,int index)
{
    if(vnode->isServer())
        return NULL;

    node_ptr node=vnode->node();
    if(!node)
        return NULL;

    const std::vector<limit_ptr>& v=node->limits();
    assert(index >=0 && index < v.size());
    QStringList data;
    getData(v[index],data);
    return new VAttribute(const_cast<VNode*>(vnode),this,data,index);
}

void VLimitAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;
    
    const std::vector<limit_ptr>& v=node->limits();
    for(size_t i=0; i < v.size(); i++)
    {    
        QStringList data;
        getData(v[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data,i);
    } 
}    

void VLimitAttribute::getData(limit_ptr lim,QStringList& data)
{    
    data << qName_ <<
        QString::fromStdString(lim->name()) <<
        QString::number(lim->value()) <<
        QString::number(lim->theLimit());
}

//================================
//Limiters
//================================

class VLimiterAttribute : public VAttributeType
{
public:
    explicit VLimiterAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,PathIndex=2};
    void getData(const InLimit& lim,QStringList& data);
};

VLimiterAttribute::VLimiterAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["limiter_name"]=NameIndex;
    searchKeyToData_["limiter_path"]=PathIndex;
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
    UserMessage::debug("VLimiterAttribute::getData -->");
#endif
    const std::vector<InLimit>& v=node->inlimits();
    if(row >=0 && row < v.size())
    {
        getData(v[row],data);
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }
    size=v.size();
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

void VLimiterAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const std::vector<InLimit>& v=node->inlimits();
    for(size_t i=0; i < v.size(); i++)
    {
        QStringList data;
        getData(v[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
}

void VLimiterAttribute::getData(const InLimit& lim,QStringList& data)
{
    data << qName_ <<
           QString::fromStdString(lim.name()) <<
           QString::fromStdString(lim.pathToNode());
}


//================================
//Triggers
//================================

class VTriggerAttribute : public VAttributeType
{
public:
    explicit VTriggerAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,CompleteIndex=1,ExprIndex=2};
};

VTriggerAttribute::VTriggerAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=3;
    searchKeyToData_["trigger_type"]=CompleteIndex;
    searchKeyToData_["trigger_expression"]=ExprIndex;
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
    UserMessage::debug("VTriggerAttribute::getData -->");
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
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }
    else if(getComplete)
    {
        data << qName_;
        data << "1" << QString::fromStdString(eC->expression());
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }

    size=(eT)?1:0;
    size+=(eC)?1:0;

#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
#endif
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

void VTriggerAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    if(Expression* eT=node->get_trigger())
    {
        QStringList data;
        data << qName_ << "0" << QString::fromStdString(eT->expression());
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }

    if(Expression* eC=node->get_complete())
    {
        QStringList data;
        data << qName_ << "1" << QString::fromStdString(eC->expression());
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
}


//================================
//Times
//================================

class VTimeAttribute : public VAttributeType
{
public:
    explicit VTimeAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
    void getData(const ecf::TimeAttr& lim,QStringList& data);
    void getData(const ecf::TodayAttr& lim,QStringList& data);
    void getData(const ecf::CronAttr& lim,QStringList& data);
};

VTimeAttribute::VTimeAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=2;
    searchKeyToData_["time_name"]=NameIndex;
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
    UserMessage::debug("VTimeAttribute::getData -->");
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
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }

    size=tV.size()+tdV.size()+ cV.size();
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

void VTimeAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
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
        QStringList data;
        getData(tV[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }

    for(size_t i=0; i < tdV.size(); i++)
    {
        QStringList data;
        getData(tdV[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }

    for(size_t i=0; i < cV.size(); i++)
    {
        QStringList data;
        getData(cV[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
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
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
    void getData(const DateAttr& lim,QStringList& data);
    void getData(const DayAttr& lim,QStringList& data);
};

VDateAttribute::VDateAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=2;
    searchKeyToData_["date_name"]=NameIndex;
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
    UserMessage::debug("VDateAttribute::getData -->");
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
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif

        return true;
    }
    size=dV.size()+dayV.size();
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

void VDateAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
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
        QStringList data;
        getData(dV[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }

    for(size_t i=0; i < dayV.size(); i++)
    {
        QStringList data;
        getData(dayV[i],data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
}

void VDateAttribute::getData(const DateAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

void VDateAttribute::getData(const DayAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}


//================================
//Repeat
//================================

class VRepeatAttribute : public VAttributeType
{
public:
    explicit VRepeatAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,SubtypeIndex=1,NameIndex=2,ValueIndex=3,StartIndex=4,EndIndex=5,StepIndex=6};
    void getData(const Repeat& r,QStringList& data);
};


VRepeatAttribute::VRepeatAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=7;
    searchKeyToData_["repeat_name"]=NameIndex;
    searchKeyToData_["repeat_value"]=ValueIndex;
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
    UserMessage::debug("VRepeatAttribute::getData -->");
#endif

    const Repeat& r=node->repeat();
    if(row ==0 && !r.empty())
    {
        getData(r,data);
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }
    size=(r.empty())?0:1;
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

bool VRepeatAttribute::exists(const VNode* vnode,QStringList data) const
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    if(data.count() != dataCount_ && data[TypeIndex] != qName_)
        return false;

    const Repeat& r=node->repeat();
    if(r.name() == data[NameIndex].toStdString())
    {
        return (VRepeat::type(r) == data[SubtypeIndex].toStdString());
    }

    return false;
}

void VRepeatAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    const Repeat& r=node->repeat();
    if(!r.empty())
    {
        QStringList data;
        getData(r,data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
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


//================================
//Late
//================================

class VLateAttribute : public VAttributeType
{
public:
    explicit VLateAttribute(const std::string& n);
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
    void getData(ecf::LateAttr *late,QStringList& data);
};


VLateAttribute::VLateAttribute(const std::string& n) : VAttributeType(n)
{
    dataCount_=2;
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
    UserMessage::debug("VLateAttribute::getData -->");
#endif

    ecf::LateAttr *late=node->get_late();
    if(row ==0 && late)
    {
        getData(late,data);
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
    }
    size=(late)?1:0;
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
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

void VLateAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;

    ecf::LateAttr *late=node->get_late();
    if(late)
    {
        QStringList data;
        getData(late,data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
    }
}

void VLateAttribute::getData(ecf::LateAttr *late,QStringList& data)
{
    if(late)
        data << qName_ << QString::fromStdString(late->name());
}

static VMeterAttribute meterAttr("meter");
static VEventAttribute eventAttr("event");
static VRepeatAttribute repeatAttr("repeat");
static VTriggerAttribute triggerAttr("trigger");
static VLabelAttribute labelAttr("label");
static VTimeAttribute timeAttr("time");
static VDateAttribute dateAttr("date");
static VLimitAttribute limitAttr("limit");
static VLimiterAttribute limiterAttr("limiter");
static VLateAttribute lateAttr("late");
static VVarAttribute varAttr("var");
static VGenvarAttribute genvarAttr("genvar");

static SimpleLoader<VAttributeType> loader("attribute");
