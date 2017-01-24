//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================


#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include "VNode.hpp"
#include "UIDebug.hpp"

#include <QDebug>

//#define  _UI_VATTRIBUTE_DEBUG

static unsigned int totalAttrNum=0;

VAttribute::VAttribute(VNode *parent,VAttributeType* type,int indexInType) :
    VItem(parent)
{
    UI_ASSERT(indexInType >=0, "Index = " << UIDebug::longToString(indexInType));
    assert(type);
    id_=indexToId(type,indexInType);
    totalAttrNum++;
}        

VAttribute::VAttribute(VNode *parent,int index) :
    VItem(parent),
    index_(index)
{
    totalAttrNum++;
}

VAttribute::~VAttribute()
{
    totalAttrNum--;
}

VAttribute* VAttribute::clone() const
{
    return new VAttribute(parent_,id_);
}

unsigned int VAttribute::totalNum()
{
    return totalAttrNum;
}

VServer* VAttribute::root() const
{
    return (parent_)?parent_->root():NULL;
}

QString VAttribute::toolTip() const
{
    VAttributeType* t=type();
    return (t)?(t->toolTip(data())):QString();
}

VAttributeType* VAttribute::type() const
{
    return idToType(id_);
}

const std::string& VAttribute::typeName() const
{
    VAttributeType* t=type();
    static std::string e;
    return (t)?(t->strName()):e;
}

std::string VAttribute::fullPath() const
{
    return (parent_)?(parent_->fullPath() + ":" + strName()):"";
}

bool VAttribute::sameContents(VItem* item) const
{
    if(!item)
        return false;

    if(VAttribute *a=item->isAttribute())
    {    return a->parent() == parent() && a->id_ == id_;
    }
    return false;
}

QStringList VAttribute::data() const
{
    QStringList d;
    if(id_ ==-1) return d;
    VAttributeType *t=idToType(id_);
    assert(t);
    int idx=idToTypeIndex(id_);
    t->itemData(parent(),idx,d);
    return d;
}

int VAttribute::absIndex(AttributeFilter *filter) const
{
    return VAttributeType::absIndexOf(this,filter);
}

void VAttribute::buildAlterCommand(std::vector<std::string>& cmd,
                                    const std::string& action, const std::string& type,
                                    const std::string& name,const std::string& value)
{
    cmd.push_back("ecflow_client");
    cmd.push_back("--alter");
    cmd.push_back(action);
    cmd.push_back(type);

    if(!name.empty())
    {
        cmd.push_back(name);
        cmd.push_back(value);
    }

    cmd.push_back("<full_name>");

}

void VAttribute::buildAlterCommand(std::vector<std::string>& cmd,
                                    const std::string& action, const std::string& type,
                                    const std::string& value)
{
    cmd.push_back("ecflow_client");
    cmd.push_back("--alter");
    cmd.push_back(action);
    cmd.push_back(type);
    cmd.push_back(value);

    cmd.push_back("<full_name>");
}

QString VAttribute::name() const
{
    std::string s;
    value("name",s);
    return QString::fromStdString(s);

#if 0
    QStringList d=data();
    if(d.count() >= 2)
       return d[1];

    return QString();
#endif
}

std::string VAttribute::strName() const
{
    return name().toStdString();
}

bool VAttribute::isValid(VNode* parent,QStringList data)
{
    if(VAttributeType* t=type())
    {
        return t->exists(parent,data);
    }
    return false;
}

bool VAttribute::value(const std::string& key,std::string& val) const
{
    int idx=type()->searchKeyToDataIndex(key);
    if(idx != -1)
    {
        QStringList d=data();
        val=d[idx].toStdString();
        return true;
    }
    return false;


#if 0

    QStringList d=data();
    VAttributeType* t=type();
    if(d.isEmpty() || !t)
        return false;

    int idx=t->searchKeyToDataIndex(key);

#ifdef _UI_VATTRIBUTE_DEBUG
    qDebug() << QString::fromStdString(key) << QString::fromStdString(val);
    qDebug() << "  data=" << d;
    qDebug() << "  idx=" << idx;
#endif

    if(idx != -1)
    {
        val=d[idx].toStdString();
        return true;
    }
    return false;
#endif

}

VAttribute* VAttribute::make(VNode* n,const std::string& type,const std::string& name)
{
    if(!n) return NULL;
    VAttributeType *t=VAttributeType::find(type);
    assert(t);
    VItemTmp_ptr item=t->item(n,name);
    return (item)?item->attribute()->clone():NULL;
}

VAttribute* VAttribute::makeFromId(VNode* n,int id)
{
    return NULL;
#if 0
    if(id ==-1) return NULL;
    VAttributeType *t=idToType(id);
    assert(t);
    QStringList d;
    int idx=idToTypeIndex(id);
    return t->getSearchData(n,idx,d);
#endif
}

VAttribute* VAttribute::make(VNode *parent,QStringList data)
{
    assert(parent);
    if(data.count() >=2)
    {
        std::string type=data[0].toStdString();
        VAttributeType *t=VAttributeType::find(type);
        assert(t);

        int idx=t->searchKeyToDataIndex("name");
        if(idx != -1 && idx < data.count())
        {
            std::string name=data[idx].toStdString();
            VItemTmp_ptr item=t->item(parent,name);
            return (item)?item->attribute()->clone():NULL;
        }
    }
    return 0;
}

int VAttribute::indexToId(VAttributeType* t,int idx)
{
    return (idx >=0)?(t->typeId()*10000+idx):-1;
}

VAttributeType* VAttribute::idToType(int id)
{
    if(id < 0) return NULL;
    return VAttributeType::find(id/10000);
}

int VAttribute::idToTypeIndex(int id)
{
    if(id < 0) return -1;
    return id-(id/10000)*10000;
}

//==============================================
//
// Label
//
//==============================================

#if 0
VAttributeType* VLabel::type_=0;
VAttributeType* VMeter::type_=0;
VAttributeType* VEvent::type_=0;

VLabel::VLabel(VNode *parent,const Label& label, int index) : VAttribute(parent,index)
{
    name_=label.name();
    if(type_ == 0)
        type_=VAttributeType::find("label");
}

int VLabel::lineNum() const
{
    return parent_->labelLineNum(index_);
}

VAttributeType* VLabel::type() const
{
    return VAttributeType::find("label");
}

QStringList VLabel::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<Label>& v=node->labels();
        encode(v[index_],s);
    }
    return s;
}

void VLabel::encode(const Label& label,QStringList& data)
{
    std::string val=label.new_value();
    if(val.empty() || val == " ")
    {
        val=label.value();
    }

    data << "label" <<
                QString::fromStdString(label.name()) <<
                QString::fromStdString(val);
}

void VLabel::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<Label>& v=node->labels();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VLabel(vnode,v[i],i));
        }
    }
}
#endif


#if 0


//==============================================
//
// Meter
//
//==============================================

VMeter::VMeter(VNode *parent,const Meter& meter, int index) : VAttribute(parent,index)
{
    name_=meter.name();
}

VAttributeType* VMeter::type() const
{
    return VAttributeType::find("meter");
}

QStringList VMeter::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<Meter>& v=node->meters();
        encode(v[index_],s);
    }
    return s;
}

void VMeter::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
     if(node_ptr node=vnode->node())
    {
        const std::vector<Meter>& v=node->meters();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VMeter(vnode,v[i],i));
        }
    }
}

//==============================================
//
// Event
//
//==============================================

VEvent::VEvent(VNode *parent,const Event& e, int index) : VAttribute(parent,index)
{
    name_=e.name_or_number();
}

VAttributeType* VEvent::type() const
{
    return VAttributeType::find("event");
}

QStringList VEvent::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<Event>& v=node->events();
        encode(v[index_],s);
    }
    return s;
}

void VEvent::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
     if(node_ptr node=vnode->node())
    {
        const std::vector<Event>& v=node->events();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VEvent(vnode,v[i],i));
        }
    }
}

#endif
