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

std::vector<VAttribute*> VAttribute::items_;

static VAttribute labelAttr("label",VParam::LabelAttribute);
static VAttribute meterAttr("meter",VParam::MeterAttribute);
static VAttribute eventAttr("event",VParam::EventAttribute);
static VAttribute repeatAttr("repeat",VParam::RepeatAttribute);
static VAttribute timeAttr("time",VParam::TimeAttribute);
static VAttribute dateAttr("date",VParam::DateAttribute);
static VAttribute triggerlAttr("trigger",VParam::TriggerAttribute);
static VAttribute varAttr("var",VParam::VarAttribute);
static VAttribute genvarAttr("genvar",VParam::GenVarAttribute);
static VAttribute lateAttr("late",VParam::LateAttribute);
static VAttribute limitAttr("limit",VParam::LimitAttribute);
static VAttribute limiterAttr("limiter",VParam::LimiterAttribute);


VAttribute::VAttribute(QString name,VParam::Type type) :
		VParam(name,type)
{
	items_.push_back(this);
}

std::vector<VParam*> VAttribute::filterItems()
{
	std::vector<VParam*> v;
	for(std::vector<VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		v.push_back(*it);
	}

	return v;
}

VAttribute* VAttribute::find(VParam::Type t)
{
	for(std::vector<VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		if((*it)->type() == t)
				return *it;
	}

	return NULL;
}

VAttribute* VAttribute::find(const std::string& name)
{
	for(std::vector<VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		if((*it)->stdName() == name)
				return *it;
	}

	return NULL;
}

int VAttribute::num(Node *node,Type type)
{
	if(!node || type == NoAttribute)
		return 0;

	switch(type)
	{
	case MeterAttribute:
		return static_cast<int>(node->meters().size());

	case LabelAttribute:
		return static_cast<int>(node->labels().size());

	case EventAttribute:
		return static_cast<int>(node->events().size());

	case GenVarAttribute:
	{
		std::vector<Variable> genV;
		node->gen_variables(genV);
		return static_cast<int>(genV.size());
	}
	case VarAttribute:
		return static_cast<int>(node->variables().size());

	case LimitAttribute:
		return static_cast<int>(node->limits().size());

	case LimiterAttribute:
		return static_cast<int>(node->inlimits().size());

	case TriggerAttribute:
		return (node->get_trigger() != NULL || node->get_complete()!= NULL)?1:0;

	case TimeAttribute:
		return static_cast<int>(node->timeVec().size() + node->todayVec().size()+ node->crons().size());

	case DateAttribute:
		return static_cast<int>(node->dates().size() + node->days().size());

	case RepeatAttribute:
		return (node->repeat().empty())?0:1;

	case LateAttribute:
		return (node->get_late())?1:0;

	default:
		return 0;
	}

	return 0;
}

int VAttribute::totalNum(Node *node)
{
	if(!node)
		return 0;

	int total=0;
	for(std::vector<VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		int n=num(node,(*it)->type());
		total+=n;
	}

	return total;
}


bool VAttribute::getData(Node *node,int row,VAttribute::Type &type,QStringList& data)
{
	type=NoAttribute;

	if(!node)
		return false;

	int totalRow=0;
	for(std::vector<VAttribute*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		if(getData(node,row,totalRow,(*it)->type(),data))
		{
			type=(*it)->type();
			return true;
		}
	}

	return false;
}

bool VAttribute::getData(Node *node,int row,int& totalRow,VAttribute::Type type,QStringList& data)
{
	if(!node || type == VAttribute::NoAttribute)
		return false;

	row-=totalRow;

	//Meters
	if(type == VAttribute::MeterAttribute)
	{
		const std::vector<Meter>&  v=node->meters();
		if(row >=0 && row < v.size())
		{
			data << QString::number(type) <<
							QString::fromStdString(v.at(row).name()) <<
							QString::number(v.at(row).value()) << QString::number(v.at(row).min()) << QString::number(v.at(row).max()) <<
							QString::number(v.at(row).colorChange());
			return true;
		}
		totalRow+=v.size();
	}

	//Labels
	else if(type == VAttribute::LabelAttribute)
	{
		const std::vector<Label>&  v=node->labels();
		if(row >=0 && row < v.size())
		{
			data << QString::number(type) <<
						QString::fromStdString(v.at(row).name()) <<
						QString::fromStdString(v.at(row).value());
			return true;
		}
		totalRow+=v.size();
	}

	//Event
	else if(type == VAttribute::EventAttribute)
	{
		const std::vector<Event>& v=node->events();
		if(row >=0 && row < v.size())
		{
			data << QString::number(type) <<
						QString::fromStdString(v.at(row).name_or_number()) <<
						QString::number((v.at(row).value()==true)?1:0);
			return true;
		}
		totalRow+=v.size();
	}

	//Generated Variables
	else if(type == VAttribute::GenVarAttribute)
	{
		std::vector<Variable> genV;
		node->gen_variables(genV);
		if(row >=0 && row < genV.size())
		{
			data << QString::number(type) <<
					QString::fromStdString(genV.at(row).name()) <<
					QString::fromStdString(genV.at(row).theValue());
			return true;
		}
		totalRow+=genV.size();
	}

	//Variables
	else if(type == VAttribute::VarAttribute)
	{
		const std::vector<Variable>& v=node->variables();
		if(row >=0 && row < v.size())
		{
			data << QString::number(type) <<
					QString::fromStdString(v.at(row).name()) <<
					QString::fromStdString(v.at(row).theValue());
			return true;
		}
		totalRow+=v.size();

	}

	//Limits
	else if(type == VAttribute::LimitAttribute)
	{
		const std::vector<limit_ptr>& v=node->limits();
		if(row >=0 && row < v.size())
		{
			data << QString::number(type) <<
						QString::fromStdString(v.at(row)->name()) <<
						QString::number(v.at(row)->value()) <<
						QString::number(v.at(row)->theLimit());
			return true;
		}
		totalRow+=v.size();

	}

	//Limiters
	else if(type == VAttribute::LimiterAttribute)
	{
			const std::vector<InLimit>& v=node->inlimits();
			if(row >=0 && row < v.size())
			{
				data << QString::number(type) <<
							QString::fromStdString(v.at(row).name());
				return true;
			}
			totalRow+=v.size();
	}

	//Triggers
	else if(type == VAttribute::TriggerAttribute)
	{
		Expression* eT=node->get_trigger();
		Expression* eC=node->get_complete();
		if(row ==0 && (eT || eC))
		{
			data << QString::number(type);
			if(eT) data << "0" << QString::fromStdString(eT->expression());
			else if(eC) data << "1" << QString::fromStdString(eC->expression());
			return true;
		}

		totalRow+=(eT || eC)?1:0;
	}

	//Times
	else if(type == VAttribute::TimeAttribute)
	{
		const std::vector<ecf::TimeAttr>& tV=node->timeVec();
		const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
		const std::vector<ecf::CronAttr>& cV=node->crons();

		if(row >=0 && row < tV.size()+tdV.size()+ cV.size())
		{
				int n=row;
				data << QString::number(type);
				if(row < tV.size())
					data << QString::fromStdString(tV.at(row).name());
				else if(row < tV.size() + tdV.size())
					data << QString::fromStdString(tdV.at(row-tV.size()).name());
				else
					data << QString::fromStdString(cV.at(row-tV.size()-tdV.size()).name());

				return true;
		}
		totalRow+=tV.size()+tdV.size()+ cV.size();
	}

	//Date
	else if(type == VAttribute::DateAttribute)
	{
		const std::vector<DateAttr>& dV=node->dates();
		const std::vector<DayAttr>& dayV=node->days();

		if(row >=0 && row < dV.size()+dayV.size())
		{
			int n=row;
			data << QString::number(type);
			if(row < dV.size())
				 data << QString::fromStdString(dV.at(row).name());
			else
				data << QString::fromStdString(dayV.at(row-dV.size()).name());

			return true;
		}
		totalRow+=dV.size()+dayV.size();
	}

	//Repeat
	else if(type == VAttribute::RepeatAttribute)
	{
		const Repeat& r=node->repeat();
		if(row >=0 && !r.empty())
		{
				data << QString::number(type) << QString::fromStdString(r.name());
				return true;
		}
		totalRow+=(r.empty())?0:1;
	}

	//Late
	else if(type == VAttribute::LateAttribute)
	{
		ecf::LateAttr *late=node->get_late();
		if(row >=0 && late)
		{
			data << QString::number(type) << QString::fromStdString(late->name());
			return true;
		}
		totalRow+=(late)?1:0;
	}

	return false;
}


void VAttribute::init()
{
	std::string parFile("/home/graphics/cgr/ecflowview_attribute.json");

	//Parse param file using the boost JSON property tree parser
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_json(parFile,pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		 std::string errorMessage = e.what();
		 UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON attributes file : " + errorMessage));
		 return;
	}

	ptree::const_iterator itTop = pt.begin();
	if(itTop == pt.end() || itTop->first != "attribute")
	{
		return;
	}

	// for each parameter
	for(ptree::const_iterator itPar = itTop->second.begin(); itPar != itTop->second.end(); ++itPar)
	{
		QString name=QString::fromStdString(itPar->first);
		ptree const &parPt = itPar->second;

		std::map<QString,QString> vals;
		for(ptree::const_iterator itV = parPt.begin(); itV != parPt.end(); ++itV)
		{
			vals[QString::fromStdString(itV->first)]=QString::fromStdString(itV->second.get_value<std::string>());
		}

		//Assign the information we read to an existing VAttribute object
		bool objFound=false;
		for(std::vector<VAttribute*>::const_iterator itObj=items_.begin(); itObj != items_.end(); itObj++)
		{
			if(name == (*itObj)->name())
			{
				objFound=true;
				(*itObj)->addAttributes(vals);
				break;
			}
		}

		//We are in trouble: the icon defined in the JSON file does not correspond to any of the VIcon objects!!!
		if(!objFound)
		{
			UserMessage::message(UserMessage::ERROR, true, std::string("Error, attribute defined in JSON file does not belong to any attribute objects : " + name.toStdString()));
		}

	}
}
