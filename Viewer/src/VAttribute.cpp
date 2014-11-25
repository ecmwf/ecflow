//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VAttribute.hpp"

#include <QDebug>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>

#include "Node.hpp"
#include "UserMessage.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

std::map<std::string,VAttribute*> VAttribute::items_;


class VMeterAttribute : public VAttribute
{
public:
	VMeterAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VEventAttribute : public VAttribute
{
public:
	VEventAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};


class VRepeatAttribute : public VAttribute
{
public:
	VRepeatAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VTriggerAttribute : public VAttribute
{
public:
	VTriggerAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VLabelAttribute : public VAttribute
{
public:
	VLabelAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VDateAttribute : public VAttribute
{
public:
	VDateAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VTimeAttribute : public VAttribute
{
public:
	VTimeAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VLimitAttribute : public VAttribute
{
public:
	VLimitAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VLimiterAttribute : public VAttribute
{
public:
	VLimiterAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VLateAttribute : public VAttribute
{
public:
	VLateAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VVarAttribute : public VAttribute
{
public:
	VVarAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
};

class VGenvarAttribute : public VAttribute
{
public:
	VGenvarAttribute(const std::string& n) : VAttribute(n) {};
	int num(Node *node);
	bool getData(Node *node,int row,int& size,QStringList& data);
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


VAttribute::VAttribute(const std::string& name) :
		VParam(name)
{
	//items_.push_back(this);
	items_[name]=this;
}

std::vector<VParam*> VAttribute::filterItems()
{
	std::vector<VParam*> v;
	for(std::map<std::string,VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		v.push_back(it->second);
	}

	return v;
}

VAttribute* VAttribute::find(const std::string& name)
{
	std::map<std::string,VAttribute*>::const_iterator it=items_.find(name);
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

int VAttribute::totalNum(Node *node)
{
	if(!node)
		return 0;

	int total=0;
	for(std::map<std::string,VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		int n=it->second->num(node);
		total+=n;
	}

	return total;
}


bool VAttribute::getData(Node *node,int row,VAttribute **type,QStringList& data)
{
	*type=0;

	if(!node)
		return false;

	int totalRow=0;
	for(std::map<std::string,VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		int size=0;
		if(it->second->getData(node,row-totalRow,size,data))
		{
			*type=it->second;
			return true;
		}
		totalRow+=size;
	}

	return false;
}


void VAttribute::init(const std::string& parFile)
{
	//std::string parFile("/home/graphics/cgr/ecflowview_attribute.json");
	std::map<std::string,std::map<std::string,std::string> > vals;

	VParam::init(parFile,"attribute",vals);

	for(std::map<std::string,std::map<std::string,std::string> >::const_iterator it=vals.begin(); it != vals.end(); it++)
	{
		std::string name=it->first;
		//Assign the information we read to an existing VAttribute object
		if(VAttribute* obj=VAttribute::find(name))
				obj->addAttributes(it->second);

		//We are in trouble: the icon defined in the JSON file does not correspond to any of the VIcon objects!!!
		else
		{
			UserMessage::message(UserMessage::ERROR, true,
					std::string("Error, attribute defined in JSON file does not belong to any attribute objects : " + name));
		}
	}
}

//================================
// Meters
//================================

int VMeterAttribute::num(Node *node)
{
	return (node)?static_cast<int>(node->meters().size()):0;
}

bool VMeterAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const std::vector<Meter>&  v=node->meters();
	if(row >=0 && row < v.size())
	{
		data << qName_ <<
						QString::fromStdString(v.at(row).name()) <<
						QString::number(v.at(row).value()) << QString::number(v.at(row).min()) << QString::number(v.at(row).max()) <<
						QString::number(v.at(row).colorChange());
		return true;
	}
	size=v.size();
	return false;
}

//================================
// Labels
//================================

int VLabelAttribute::num(Node *node)
{
	return (node)?static_cast<int>(node->labels().size()):0;
}

bool VLabelAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const std::vector<Label>&  v=node->labels();
	if(row >=0 && row < v.size())
	{
		data << qName_ <<
					QString::fromStdString(v.at(row).name()) <<
					QString::fromStdString(v.at(row).value());
		return true;
	}
	size=v.size();
	return false;
}

//================================
// Events
//================================

int VEventAttribute::num(Node *node)
{
	return (node)? static_cast<int>(node->events().size()):0;
}

bool VEventAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const std::vector<Event>& v=node->events();
	if(row >=0 && row < v.size())
	{
		data << qName_ <<
						QString::fromStdString(v.at(row).name_or_number()) <<
						QString::number((v.at(row).value()==true)?1:0);
		return true;
	}
	size=v.size();
	return false;
}

//================================
//Generated Variables
//================================

int VGenvarAttribute::num(Node *node)
{
	if(node)
	{
		std::vector<Variable> genV;
		node->gen_variables(genV);
		return static_cast<int>(genV.size());
	}
	return 0;
}

bool VGenvarAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	std::vector<Variable> genV;
	node->gen_variables(genV);
	if(row >=0 && row < genV.size())
	{
		data << qName_ <<
				QString::fromStdString(genV.at(row).name()) <<
				QString::fromStdString(genV.at(row).theValue());
		return true;
	}
	size=genV.size();
	return false;
}

//================================
//Variables
//================================

int VVarAttribute::num(Node *node)
{
	return (node)?static_cast<int>(node->variables().size()):0;
}

bool VVarAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
		const std::vector<Variable>& v=node->variables();
		if(row >=0 && row < v.size())
		{
			data << qName_ <<
					QString::fromStdString(v.at(row).name()) <<
					QString::fromStdString(v.at(row).theValue());
			return true;
		}
		size=v.size();
		return false;
}

//================================
// Limits
//================================

int VLimitAttribute::num(Node *node)
{
	return (node)?static_cast<int>(node->limits().size()):0;
}

bool VLimitAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const std::vector<limit_ptr>& v=node->limits();
	if(row >=0 && row < v.size())
	{
		data << qName_ <<
					QString::fromStdString(v.at(row)->name()) <<
					QString::number(v.at(row)->value()) <<
					QString::number(v.at(row)->theLimit());
		return true;
	}
	size=v.size();
	return false;
}

//================================
//Limiters
//================================

int VLimiterAttribute::num(Node *node)
{
	return (node)?static_cast<int>(node->inlimits().size()):0;
}

bool VLimiterAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const std::vector<InLimit>& v=node->inlimits();
	if(row >=0 && row < v.size())
	{
			data << qName_ <<
						QString::fromStdString(v.at(row).name()) <<
						QString::fromStdString(v.at(row).pathToNode());
			return true;
	}
	size=v.size();
	return false;
}

//================================
//Triggers
//================================

int VTriggerAttribute::num(Node *node)
{
	return (node->get_trigger() != NULL || node->get_complete()!= NULL)?1:0;
}

bool VTriggerAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	Expression* eT=node->get_trigger();
	Expression* eC=node->get_complete();
	if(row ==0 && (eT || eC))
	{
		data << qName_;
		if(eT) data << "0" << QString::fromStdString(eT->expression());
		else if(eC) data << "1" << QString::fromStdString(eC->expression());
		return true;
	}

	size=(eT || eC)?1:0;
	return false;
}

//================================
//Times
//================================

int VTimeAttribute::num(Node *node)
{
	return (node)?static_cast<int>(node->timeVec().size() + node->todayVec().size()+ node->crons().size()):0;
}

bool VTimeAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const std::vector<ecf::TimeAttr>& tV=node->timeVec();
	const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
	const std::vector<ecf::CronAttr>& cV=node->crons();

	if(row >=0 && row < tV.size()+tdV.size()+ cV.size())
	{
			int n=row;
			data << qName_;
			if(row < tV.size())
				data << QString::fromStdString(tV.at(row).name());
			else if(row < tV.size() + tdV.size())
				data << QString::fromStdString(tdV.at(row-tV.size()).name());
			else
				data << QString::fromStdString(cV.at(row-tV.size()-tdV.size()).name());

			return true;
	}

	size=tV.size()+tdV.size()+ cV.size();
	return false;
}

//================================
//Date
//================================

int VDateAttribute::num(Node *node)
{
	return (node)?static_cast<int>(node->dates().size() + node->days().size()):0;
}

bool VDateAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const std::vector<DateAttr>& dV=node->dates();
	const std::vector<DayAttr>& dayV=node->days();

	if(row >=0 && row < dV.size()+dayV.size())
	{
		int n=row;
		data << qName_;
		if(row < dV.size())
			 data << QString::fromStdString(dV.at(row).name());
		else
			data << QString::fromStdString(dayV.at(row-dV.size()).name());

		return true;
	}
	size=dV.size()+dayV.size();

	return false;
}


//================================
//Repeat
//================================

int VRepeatAttribute::num(Node *node)
{
	return (node)?((node->repeat().empty())?0:1):0;
}

bool VRepeatAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	const Repeat& r=node->repeat();
	if(row >=0 && !r.empty())
	{
			data << qName_ << QString::fromStdString(r.name());
			return true;
	}
	size=(r.empty())?0:1;
	return false;
}

//================================
//Late
//================================

int VLateAttribute::num(Node *node)
{
	return (node)?((node->get_late())?1:0):0;
}

bool VLateAttribute::getData(Node *node,int row,int& size,QStringList& data)
{
	ecf::LateAttr *late=node->get_late();
	if(row >=0 && late)
	{
		data << qName_ << QString::fromStdString(late->name());
		return true;
	}
	size=(late)?1:0;
	return false;
}


