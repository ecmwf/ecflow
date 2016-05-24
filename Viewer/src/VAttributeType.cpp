//============================================================================
// Copyright 2014 ECMWF.
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

class VMeterAttribute : public VAttributeType
{
public:
    explicit VMeterAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
    void getSearchData(const VNode* vnode,QList<VAttribute*>& lst);

private:
    void getData(const Meter& m,QStringList& data);

};

class VEventAttribute : public VAttributeType
{
public:
    explicit  VEventAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
};

class VRepeatAttribute : public VAttributeType
{
public:
    explicit VRepeatAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
};

class VTriggerAttribute : public VAttributeType
{
public:
    explicit VTriggerAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
};

class VLabelAttribute : public VAttributeType
{
public:
    explicit VLabelAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    int lineNum(const VNode* vnode,int row);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
};

class VDateAttribute : public VAttributeType
{
public:
    explicit VDateAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
};

class VTimeAttribute : public VAttributeType
{
public:
    explicit VTimeAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
};

class VLimitAttribute : public VAttributeType
{
public:
    explicit VLimitAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
    bool exists(const VNode* vnode,QStringList) const;
};

class VLimiterAttribute : public VAttributeType
{
public:
    explicit VLimiterAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
};

class VLateAttribute : public VAttributeType
{
public:
    explicit VLateAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    QString toolTip(QStringList d) const;
};

class VVarAttribute : public VAttributeType
{
public:
    explicit VVarAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    bool exists(const VNode* vnode,QStringList) const;
};

class VGenvarAttribute : public VAttributeType
{
public:
    explicit VGenvarAttribute(const std::string& n) : VAttributeType(n) {}
    int num(const VNode *node);
    bool getData(VNode *node,int row,int& size,QStringList& data);
    bool exists(const VNode* vnode,QStringList) const;
};

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


VAttributeType::VAttributeType(const std::string& name) :
        VParam(name)
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

//================================
// Meters
//================================

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
       /* data << qName_ <<
                        QString::fromStdString(v.at(row).name()) <<
                        QString::number(v.at(row).value()) << QString::number(v.at(row).min()) << QString::number(v.at(row).max()) <<
                        QString::number(v.at(row).colorChange());*/
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
    if(d.count() >=5)
    {
        t+="<b>Name:</b> " + d[1] + "<br>";
        t+="<b>Value:</b> " + d[2]+ "<br>";
        t+="<b>Minimum:</b> " + d[3] + "<br>";
        t+="<b>Maximum:</b> " + d[4];
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

    if(data.count() != 6 && data[0] != qName_)
        return false;

    const std::vector<Meter>&  v=node->meters();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[1].toStdString())
            return true;
    }

    return false;
}

void VMeterAttribute::getSearchData(const VNode* vnode,QList<VAttribute*>& lst)
{
    if(vnode->isServer())
        return;

    node_ptr node=vnode->node();
    if(!node)
        return;
    
    const std::vector<Meter>& v=node->meters();
    for(std::vector<Meter>::const_iterator it=v.begin(); it != v.end(); ++it)
    {    
        QStringList data;
        getData(*it,data);
        lst << new VAttribute(const_cast<VNode*>(vnode),this,data);
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
        std::string val=v.at(row).new_value();
        if(val.empty() || val == " ")
        {
            val=v.at(row).value();
        }

        data << qName_ <<
                    QString::fromStdString(v.at(row).name()) <<
                    QString::fromStdString(val);

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
        std::string val=v.at(row).new_value();
        if(val.empty() || val == " ")
        {
            val=v.at(row).value();
        }
        return std::count(val.begin(), val.end(), '\n')+1;
    }

    return 1;
}

QString VLabelAttribute::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Label<br>";
    if(d.count() >= 3)
    {
        t+="<b>Name:</b> " + d[1] + "<br>";
        t+="<b>Value:</b> " + d[2];
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

    if(data.count() != 3 && data[0] != qName_)
        return false;

    const std::vector<Label>&  v=node->labels();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[1].toStdString())
            return true;
    }

    return false;
}

//================================
// Events
//================================

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
        data << qName_ <<
                QString::fromStdString(v.at(row).name_or_number()) <<
                QString::number((v.at(row).value()==true)?1:0);
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
    if(d.count() >=3)
    {
        t+="<b>Name:</b> " + d[1] + "<br>";
        t+="<b>Status:</b> ";
        t+=(d[2] == "1")?"set (true)":"clear (false)";

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

    if(data.count() != 3 && data[0] != qName_)
        return false;

    const std::vector<Event>&  v=node->events();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name_or_number() == data[1].toStdString())
            return true;
    }

    return false;
}

//================================
//Generated Variables
//================================

int VGenvarAttribute::num(const VNode *vnode)
{
    return vnode->genVariablesNum();

    /*node_ptr node=vnode->node();
    if(node.get())
    {
        std::vector<Variable> genV;
        node->gen_variables(genV);
        return static_cast<int>(genV.size());
    }
    return 0;*/
}

bool VGenvarAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("VGenvarAttribute::getData -->");
#endif

    std::vector<Variable> genV;
    vnode->genVariables(genV);

    /*node_ptr node=vnode->node();
    if(!node.get())
            return false;

    std::vector<Variable> genV;
    node->gen_variables(genV);*/

    if(row >=0 && row < genV.size())
    {
        data << qName_ <<
                QString::fromStdString(genV.at(row).name()) <<
                QString::fromStdString(genV.at(row).theValue());
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
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    if(data.count() != 3 && data[0] != qName_)
        return false;

    std::vector<Variable> v;
    vnode->genVariables(v);

    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i].name() == data[1].toStdString())
           return true;
    }

    return false;
}

//================================
//Variables
//================================

int VVarAttribute::num(const VNode *vnode)
{
    return vnode->variablesNum();

    //node_ptr node=vnode->node();
    //return (node.get())?static_cast<int>(node->variables().size()):0;
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
        //node_ptr node=vnode->node();
        if(row >=0 && row < v.size())
        {
            data << qName_ <<
                    QString::fromStdString(v.at(row).name()) <<
                    QString::fromStdString(v.at(row).theValue());
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
        if(!node.get())
            return false;

        const std::vector<Variable>& v=node->variables();
        if(row >=0 && row < v.size())
        {
            data << qName_ <<
                    QString::fromStdString(v.at(row).name()) <<
                    QString::fromStdString(v.at(row).theValue());
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
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node)
        return false;

    if(data.count() != 3 && data[0] != qName_)
        return false;

    if(vnode->isServer())
    {
        std::vector<Variable> v;
        vnode->variables(v);
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == data[1].toStdString())
               return true;
        }
    }
    else
    {
        const std::vector<Variable>& v=node->variables();
        for(size_t i=0; i < v.size(); i++)
        {
            if(v[i].name() == data[1].toStdString())
                return true;
        }
    }

    return false;
}

//================================
// Limits
//================================

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
        data << qName_ <<
                    QString::fromStdString(v.at(row)->name()) <<
                    QString::number(v.at(row)->value()) <<
                    QString::number(v.at(row)->theLimit());
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
    if(d.count() >=4)
    {
        t+="<b>Name:</b> " + d[1] + "<br>";
        t+="<b>Value:</b> " + d[2] + "<br>";
        t+="<b>Maximum:</b> " + d[3];
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

    if(data.count() != 4 && data[0] != qName_)
        return false;

    const std::vector<limit_ptr>& v=node->limits();
    for(size_t i=0; i < v.size(); i++)
    {
        if(v[i]->name() == data[1].toStdString())
            return true;
    }

    return false;
}

//================================
//Limiters
//================================

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
        data << qName_ <<
                    QString::fromStdString(v.at(row).name()) <<
                    QString::fromStdString(v.at(row).pathToNode());
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
    if(d.count() >=3)
    {
        t+="<b>Limit:</b> " + d[1] + "<br>";
        t+="<b>Node:</b> " + d[2];

    }
    return t;
}

//================================
//Triggers
//================================

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
    {    if(eT)
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
    if(d.count() >=3)
    {
        if(d[1] == "0")
            t+="<b>Type:</b> Trigger<br>";
        else if(d[1] == "1")
            t+="<b>Type:</b> Complete<br>";
        else
            return t;

        t+="<b>Expression:</b> " + d[2];
    }
    return t;
}

//================================
//Times
//================================

int VTimeAttribute::num(const VNode *vnode)
{
    if(vnode->isServer())
        return 0;

    node_ptr node=vnode->node();
    return (node.get())?static_cast<int>(node->timeVec().size() + node->todayVec().size()+ node->crons().size()):0;
}

bool VTimeAttribute::getData(VNode *vnode,int row,int& size,QStringList& data)
{
    if(vnode->isServer())
        return false;

    node_ptr node=vnode->node();
    if(!node.get())
        return false;

#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("VTimeAttribute::getData -->");
#endif

    const std::vector<ecf::TimeAttr>& tV=node->timeVec();
    const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
    const std::vector<ecf::CronAttr>& cV=node->crons();

    if(row >=0 && row < tV.size()+tdV.size()+ cV.size())
    {
        data << qName_;
        if(row < tV.size())
            data << QString::fromStdString(tV.at(row).name());
        else if(row < tV.size() + tdV.size())
            data << QString::fromStdString(tdV.at(row-tV.size()).name());
        else
            data << QString::fromStdString(cV.at(row-tV.size()-tdV.size()).name());
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
    if(d.count() >=2)
    {
        t+="<b>Name:</b> " + d[1];
    }
    return t;
}



//================================
//Date
//================================

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
        data << qName_;
        if(row < dV.size())
             data << QString::fromStdString(dV.at(row).name());
        else
            data << QString::fromStdString(dayV.at(row-dV.size()).name());

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
    if(d.count() >=2)
    {
        t+="<b>Name:</b> " + d[1];
    }
    return t;
}

//================================
//Repeat
//================================

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
        //We try to avoid creating a VRepeat object everytime we are here
        std::string type=VRepeat::type(r);

        data << qName_ << QString::fromStdString(type) <<
             QString::fromStdString(r.name()) <<
             QString::fromStdString(r.valueAsString()) <<
             QString::fromStdString(r.value_as_string(r.start())) <<
             QString::fromStdString(r.value_as_string(r.end())) <<
             QString::number(r.step());

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
    if(d.count() == 7)
    {
        t+=" " + d[1] + "<br>";

        if(d[1] != "day")
        {
            t+="<b>Name:</b> " + d[2] + "<br>";
            t+="<b>Value:</b> " + d[3] + "<br>";
            t+="<b>Start:</b> " + d[4] + "<br>";
            t+="<b>End:</b> " + d[5] + "<br>";
            t+="<b>Step:</b> " + d[6];
        }
        else
        {
            t+="<b>Step:</b> " + d[6];
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

    if(data.count() != 7 && data[0] != qName_)
        return false;

    const Repeat& r=node->repeat();
    if(r.name() == data[2].toStdString())
    {
        return (VRepeat::type(r) == data[1].toStdString());
    }

    return false;
}

//================================
//Late
//================================

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
        data << qName_ << QString::fromStdString(late->name());
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
    if(d.count() >=2)
    {
        t+="<b>Name:</b> " + d[1];
    }
    return t;
}

static SimpleLoader<VAttributeType> loader("attribute");

