//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VIcon.hpp"

#include <QDebug>
#include <QImage>
#include <QImageReader>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>

#include "Node.hpp"
#include "Submittable.hpp"
#include "UserMessage.hpp"
#include "VFilter.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

std::vector<VIcon*> VIcon::items_;

static VIcon waitIcon("wait",VParam::WaitIcon,&VIcon::testWait);
static VIcon rerunIcon("rerun",VParam::RerunIcon,&VIcon::testRerun);
static VIcon messageIcon("message",VParam::MessageIcon,&VIcon::testMessage);
static VIcon completeIcon("complete",VParam::CompleteIcon,&VIcon::testComplete);
static VIcon timeIcon("time",VParam::TimeIcon,&VIcon::testTime);
static VIcon dateIcon("date",VParam::DateIcon,&VIcon::testDate);
static VIcon zombieIcon("zombie",VParam::ZombieIcon,&VIcon::testZombie);
static VIcon lateIcon("late",VParam::LateIcon,&VIcon::testLate);

VIcon::VIcon(QString name,VParam::Type type,TestProc testProc) :
		VParam(name,type),
		pix_(0),
        testProc_(testProc)
{
	items_.push_back(this);
}


QPixmap* VIcon::pixmap(int size)
{
	if(!pix_)
	{
		QString path=text("icon");
		QImageReader imgR(":/viewer/" + path);
		if(imgR.canRead())
		{
			imgR.setScaledSize(QSize(size,size));
			QImage img=imgR.read();
			pix_=new QPixmap(QPixmap::fromImage(img));
		}
		else
			pix_=new QPixmap();

	}

	return pix_;
}

VIcon::~VIcon()
{
	delete pix_;
}

//===============================================================
//
// Static methods
//
//===============================================================

std::vector<VParam*> VIcon::filterItems()
{
	std::vector<VParam*> v;
	for(std::vector<VIcon*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		v.push_back(*it);
	}

	return v;
}

VIcon* VIcon::find(VParam::Type t)
{
	for(std::vector<VIcon*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		if((*it)->type() == t)
				return *it;
	}

	return NULL;
}

VIcon* VIcon::find(const std::string& name)
{
	for(std::vector<VIcon*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		if((*it)->stdName() == name)
				return *it;
	}

	return NULL;
}

/*VParam* VIcon::item(VParam::Type type)
{
	for(std::vector<VIcon*>::const_iterator it=items_.begin(); it != items_.end(); it++)
		if( (*it)->type() == type)
			return this;

	return NULL;
}*/

//Create the pixmap containing all the relevant icons for the given node according to the filter.

QVariantList VIcon::pixmapList(Node *node,VFilter *filter)
{
	QVariantList lst;
	if(!node)
		return lst;

	for(std::vector<VIcon*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
			if(filter->current().find((*it)->type()) != filter->current().end())
			{
				VIcon *v=*it;
				if((*(v->testProc_))(node) )
				{
					lst << *((*it)->pixmap(16));
				}
			}
	}

	return lst;
}

bool  VIcon::testWait(Node *n)
{
	return n->flag().is_set(ecf::Flag::WAIT);
}

bool  VIcon::testRerun(Node *n)
{
	if(Submittable* s = n->isSubmittable())
	{
		return (s->try_no() > 1);
	}

	return false;
}

bool  VIcon::testMessage(Node *n)
{
	return n->flag().is_set(ecf::Flag::MESSAGE);
}

bool  VIcon::testComplete(Node *n)
{
	 /*if (!owner_) return False;
	  else if (!owner_)
	    return False;
	  else if (owner_->defstatus() == STATUS_COMPLETE)
	    return True;
	  Node* ecf = __node__() ? __node__()->get_node() : 0;
	  if (ecf) {
	    AstTop* t = ecf->completeAst();
	    if (t)
	      if (t->evaluate())
	        return True;
	  }
	  return False;*/

	  return false;
}

bool  VIcon::testDate(Node *n)
{
	return (n->days().size() > 0 || n->dates().size() > 0);
}

bool  VIcon::testTime(Node *n)
{
	//Check if time is held
	if(TimeDepAttrs *attr = n->get_time_dep_attrs())
	{
		return !attr->time_today_cron_is_free();
	}

	return (n->timeVec().size() > 0 ||
	        n->todayVec().size() > 0 ||
	        n->crons().size() > 0);
}

bool VIcon::testZombie(Node *n)
{
	return n->flag().is_set(ecf::Flag::ZOMBIE);
}

bool VIcon::testLate(Node *n)
{
	return n->flag().is_set(ecf::Flag::LATE);
}


void VIcon::init()
{
	std::string parFile("/home/graphics/cgr/ecflowview_icon.json");

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
		 UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON icons file : " + errorMessage));
		 return;
	}

	//Check if the categorry to read in is aleblled as "icon"
	ptree::const_iterator itTop = pt.begin();
	if(itTop == pt.end() || itTop->first != "icon")
	{
		return;
	}

	//Loop for each parameter
	for(ptree::const_iterator itPar = itTop->second.begin(); itPar != itTop->second.end(); ++itPar)
	{
		QString name=QString::fromStdString(itPar->first);
		ptree const &parPt = itPar->second;

		std::map<QString,QString> vals;
		for(ptree::const_iterator itV = parPt.begin(); itV != parPt.end(); ++itV)
		{
			vals[QString::fromStdString(itV->first)]=QString::fromStdString(itV->second.get_value<std::string>());
		}

		//Assign the information we read to an existing VIcon object
		bool objFound=false;
		for(std::vector<VIcon*>::const_iterator itObj=items_.begin(); itObj != items_.end(); itObj++)
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
			 UserMessage::message(UserMessage::ERROR, true, std::string("Error, icon defined in JSON file does not belong to any icon objects : " + name.toStdString()));
		}
	}
}
