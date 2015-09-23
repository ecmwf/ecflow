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

#include "IconProvider.hpp"
#include "Submittable.hpp"
#include "UserMessage.hpp"
#include "VConfigLoader.hpp"
#include "VFilter.hpp"
#include "VNode.hpp"
#include "VProperty.hpp"

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
	explicit VWaitIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

class VRerunIcon : public VIcon
{
public:
	explicit VRerunIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

class VMessageIcon : public VIcon
{
public:
	explicit VMessageIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

class VCompleteIcon : public VIcon
{
public:
	explicit VCompleteIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

class VTimeIcon : public VIcon
{
public:
	explicit VTimeIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

class VDateIcon : public VIcon
{
public:
	explicit VDateIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

class VZombieIcon : public VIcon
{
public:
	explicit VZombieIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

class VLateIcon : public VIcon
{
public:
	explicit VLateIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
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
		pixId_(-1)
{
	items_[name]=this;
}

VIcon::~VIcon()
{
}

void VIcon::initPixmap()
{
	if(!prop_)
	{
		UserMessage::message(UserMessage::WARN, true,
				std::string("Warning! VIcon::initPixmap() unable to create icon image for: " + strName()));
		return;
	}

	//Add icon to iconprovider
	if(VProperty* ip=prop_->findChild("icon"))
	{
		pixId_=IconProvider::add(":/viewer/" + ip->value().toString(),name());
	}
}

QPixmap VIcon::pixmap(int size)
{
	return IconProvider::pixmap(name(),size);
}

//===============================================================
//
// Static methods
//
//===============================================================

std::vector<VParam*> VIcon::filterItems()
{
	std::vector<VParam*> v;
	for(std::map<std::string,VIcon*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
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
QVariantList VIcon::pixmapList(VNode *vnode,VParamSet *filter)
{
	QVariantList lst;
	if(!vnode)
		return lst;

	for(std::map<std::string,VIcon*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
            VIcon *v=it->second;
            if(!filter || filter->current().find(it->second) != filter->current().end())
            {
                if(v->show(vnode))
                {
                   lst << v->pixId_;
                }
            }
	}

	return lst;
}

void VIcon::load(VProperty* group)
{
    Q_FOREACH(VProperty* p,group->children())
    {
         if(VIcon* obj=VIcon::find(p->strName()))
         {
            obj->setProperty(p);
            obj->initPixmap();
         }
    }
}

static SimpleLoader<VIcon> loader("icon");


//==========================================================
// Wait
//==========================================================

bool  VWaitIcon::show(VNode *n)
{
	node_ptr node=n->node();
	if(!node.get()) return false;

	return node->flag().is_set(ecf::Flag::WAIT);
}

//==========================================================
// Rerun
//==========================================================

bool  VRerunIcon::show(VNode *n)
{
	node_ptr node=n->node();
	if(!node.get()) return false;

	if(Submittable* s = node->isSubmittable())
	{
		return (s->try_no() > 1);
	}

	return false;
}

//==========================================================
// Message
//==========================================================

bool  VMessageIcon::show(VNode *n)
{
	node_ptr node=n->node();
	if(!node.get()) return false;

	return node->flag().is_set(ecf::Flag::MESSAGE);
}

//==========================================================
// Complete
//==========================================================

bool  VCompleteIcon::show(VNode *n)
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

bool  VDateIcon::show(VNode *n)
{
	node_ptr node=n->node();

	if(!node.get()) return false;

	return (node->days().size() > 0 || node->dates().size() > 0);
}

//==========================================================
// Time
//==========================================================

bool  VTimeIcon::show(VNode *n)
{
	node_ptr node=n->node();

	//Check if time is held
	if(TimeDepAttrs *attr = node->get_time_dep_attrs())
	{
		return !attr->time_today_cron_is_free();
	}

	return (node->timeVec().size() > 0 ||
	        node->todayVec().size() > 0 ||
	        node->crons().size() > 0);
}

//==========================================================
// Zombie
//==========================================================

bool VZombieIcon::show(VNode *n)
{
	node_ptr node=n->node();
	if(!node.get()) return false;

	return node->flag().is_set(ecf::Flag::ZOMBIE);
}

//==========================================================
// Late
//==========================================================

bool VLateIcon::show(VNode *n)
{
	node_ptr node=n->node();
	if(!node.get()) return false;

	return node->flag().is_set(ecf::Flag::LATE);
}

