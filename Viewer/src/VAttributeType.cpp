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

std::map<std::string,VAttributeType*> VAttributeType::items_;

//#define _UI_ATTR_DEBUG

class VMeterAttribute : public VAttributeType
{
public:
    explicit VMeterAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VEventAttribute : public VAttributeType
{
public:
    explicit  VEventAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};


class VRepeatAttribute : public VAttributeType
{
public:
    explicit VRepeatAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VTriggerAttribute : public VAttributeType
{
public:
    explicit VTriggerAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VLabelAttribute : public VAttributeType
{
public:
    explicit VLabelAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
	int lineNum(const VNode* vnode,int row);
};

class VDateAttribute : public VAttributeType
{
public:
    explicit VDateAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VTimeAttribute : public VAttributeType
{
public:
    explicit VTimeAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VLimitAttribute : public VAttributeType
{
public:
    explicit VLimitAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VLimiterAttribute : public VAttributeType
{
public:
    explicit VLimiterAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VLateAttribute : public VAttributeType
{
public:
    explicit VLateAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VVarAttribute : public VAttributeType
{
public:
    explicit VVarAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
};

class VGenvarAttribute : public VAttributeType
{
public:
    explicit VGenvarAttribute(const std::string& n) : VAttributeType(n) {}
	int num(const VNode *node);
	bool getData(VNode *node,int row,int& size,QStringList& data);
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

int VAttributeType::totalNum(const VNode *vnode)
{
	if(!vnode)
		return 0;

	int total=0;
	for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		int n=it->second->num(vnode);
		total+=n;
	}

	return total;
}

VAttributeType* VAttributeType::getType(const VNode *vnode,int row)
{
	if(!vnode)
		return NULL;

	int totalRow=0;
	for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		int size=it->second->num(vnode);
		if(row-totalRow >=0 && row-totalRow < size)
		{
			return it->second;
		}
		totalRow+=size;
	}

	return NULL;
}


bool VAttributeType::getData(VNode *vnode,int row,VAttributeType* &type,QStringList& data)
{
	type=0;

	if(!vnode)
		return false;

    if(vnode->name() == "main")
        qDebug() << "main";

	int totalRow=0;
	for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		int size=0;
		if(it->second->getData(vnode,row-totalRow,size,data))
		{
			type=it->second;        
            return true;
		}
		totalRow+=size;
	}

	return false;
}

bool VAttributeType::getData(const std::string& type,VNode* vnode,int row,QStringList& data)
{
	if(VAttributeType* va=find(type))
	{
		int size=0;
		return va->getData(vnode,row,size,data);
	}
	return false;
}

int VAttributeType::getLineNum(const VNode *vnode,int row)
{
	if(!vnode)
		return 1;

	int totalRow=0;
	for(std::map<std::string,VAttributeType*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		int size=it->second->num(vnode);
		if(row-totalRow >=0 && row-totalRow < size)
		{
			return it->second->lineNum(vnode,row-totalRow);
		}
		totalRow+=size;
	}

	return 1;
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
		data << qName_ <<
						QString::fromStdString(v.at(row).name()) <<
						QString::number(v.at(row).value()) << QString::number(v.at(row).min()) << QString::number(v.at(row).max()) <<
						QString::number(v.at(row).colorChange());
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
		node_ptr node=vnode->node();
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

	return (node->get_trigger() != NULL || node->get_complete()!= NULL)?1:0;
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
	if(row ==0 && (eT || eC))
	{
		data << qName_;
		if(eT) data << "0" << QString::fromStdString(eT->expression());
		else if(eC) data << "1" << QString::fromStdString(eC->expression());
#ifdef _UI_ATTR_DEBUG
        UserMessage::debug("  data=" + data.join(",").toStdString());
#endif
        return true;
	}

	size=(eT || eC)?1:0;
#ifdef _UI_ATTR_DEBUG
    UserMessage::debug("  size=" + QString::number(size).toStdString());
#endif
    return false;
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
		data << qName_ << QString::fromStdString(r.name()) <<
		QString::fromStdString(r.valueAsString());
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

static SimpleLoader<VAttributeType> loader("attribute");

