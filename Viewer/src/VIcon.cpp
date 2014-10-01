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

std::map<std::string,VIcon*> VIcon::items_;

//==========================================================
//
// Define VIcon subclasses implementing the various icons.
// These do not need to be visible outside the class so we
// define them here in the cpp file.
//
//==========================================================

class VWaitIcon : public VIcon
{
public:
	VWaitIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

class VRerunIcon : public VIcon
{
public:
	VRerunIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

class VMessageIcon : public VIcon
{
public:
	VMessageIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

class VCompleteIcon : public VIcon
{
public:
	VCompleteIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

class VTimeIcon : public VIcon
{
public:
	VTimeIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

class VDateIcon : public VIcon
{
public:
	VDateIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

class VZombieIcon : public VIcon
{
public:
	VZombieIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

class VLateIcon : public VIcon
{
public:
	VLateIcon(const std::string& name) : VIcon(name) {};
	bool show(Node*);
};

//==========================================================
//
// Create VIcon instances
//
//==========================================================

static VWaitIcon waitIcon("wait");
static VRerunIcon rerunIcon("rerun");
static VMessageIcon messageIcon("message");
static VCompleteIcon completeIcon("complete");
static VTimeIcon timeIcon("time");
static VDateIcon dateIcon("date");
static VZombieIcon zombieIcon("zombie");
static VLateIcon lateIcon("late");

//==========================================================
//
// The VIcon baseclass
//
//==========================================================

VIcon::VIcon(const std::string& name) :
		VParam(name),
		pix_(0)
{
	items_[name]=this;
}


QPixmap* VIcon::pixmap(int size)
{
	if(!pix_)
	{
		QString path=QString::fromStdString(text("icon"));
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
	for(std::map<std::string,VIcon*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		v.push_back(it->second);
	}

	return v;
}

VIcon* VIcon::find(const std::string& name)
{
	std::map<std::string,VIcon*>::const_iterator it=items_.find(name);
	if(it != items_.end())
		return it->second;

	return NULL;
}


//Create the pixmap containing all the relevant icons for the given node according to the filter.
QVariantList VIcon::pixmapList(Node *node,VFilter *filter)
{
	QVariantList lst;
	if(!node)
		return lst;

	for(std::map<std::string,VIcon*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
			if(filter->current().find(it->second) != filter->current().end())
			{
				VIcon *v=it->second;
				if(v->show(node) )
				{
					lst << *(v->pixmap(16));
				}
			}
	}

	return lst;
}

void VIcon::init(const std::string& parFile)
{
	//std::string parFile("/home/graphics/cgr/ecflowview_icon.json");
	std::map<std::string,std::map<std::string,std::string> > vals;

	VParam::init(parFile,"icon",vals);

	for(std::map<std::string,std::map<std::string,std::string> >::const_iterator it=vals.begin(); it != vals.end(); it++)
	{
		std::string name=it->first;
		//Assign the information we read to an existing VAttribute object
		if(VIcon* obj=VIcon::find(name))
				obj->addAttributes(it->second);

		//We are in trouble: the icon defined in the JSON file does not correspond to any of the VIcon objects!!!
		else
		{
			UserMessage::message(UserMessage::ERROR, true,
					std::string("Error, icon defined in JSON file does not belong to any attribute objects : " + name));
		}
	}
}

//==========================================================
// Wait
//==========================================================

bool  VWaitIcon::show(Node *n)
{
	return n->flag().is_set(ecf::Flag::WAIT);
}

//==========================================================
// Rerun
//==========================================================

bool  VRerunIcon::show(Node *n)
{
	if(Submittable* s = n->isSubmittable())
	{
		return (s->try_no() > 1);
	}

	return false;
}

//==========================================================
// Message
//==========================================================

bool  VMessageIcon::show(Node *n)
{
	return n->flag().is_set(ecf::Flag::MESSAGE);
}

//==========================================================
// Complete
//==========================================================

bool  VCompleteIcon::show(Node *n)
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

//==========================================================
// Date
//==========================================================

bool  VDateIcon::show(Node *n)
{
	return (n->days().size() > 0 || n->dates().size() > 0);
}

//==========================================================
// Time
//==========================================================

bool  VTimeIcon::show(Node *n)
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

//==========================================================
// Zombie
//==========================================================

bool VZombieIcon::show(Node *n)
{
	return n->flag().is_set(ecf::Flag::ZOMBIE);
}

//==========================================================
// Late
//==========================================================

bool VLateIcon::show(Node *n)
{
	return n->flag().is_set(ecf::Flag::LATE);
}

