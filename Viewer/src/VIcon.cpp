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

#include "ExprAst.hpp"

#include "IconProvider.hpp"
#include "Submittable.hpp"
#include "UserMessage.hpp"
#include "VConfigLoader.hpp"
#include "VFilter.hpp"
#include "VNode.hpp"
#include "VProperty.hpp"

std::map<std::string,VIcon*> VIcon::items_;
std::vector<VIcon*> VIcon::itemsVec_;

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

class VSlowIcon : public VIcon
{
public:
	explicit VSlowIcon(const std::string& name) : VIcon(name) {};
	bool show(VNode*);
};

//==========================================================
//
// Create VIcon instances
//
//==========================================================

//This also defines the order the icons will appear in the views
static VMessageIcon messageIcon("message");
static VRerunIcon rerunIcon("rerun");
static VCompleteIcon completeIcon("complete");
static VLateIcon lateIcon("late");
static VTimeIcon timeIcon("time");
static VDateIcon dateIcon("date");
static VWaitIcon waitIcon("wait");
static VZombieIcon zombieIcon("zombie");
static VSlowIcon slowIcon("slow");

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
	itemsVec_.push_back(this);
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

	for(std::vector<VIcon*>::const_iterator it=itemsVec_.begin(); it != itemsVec_.end(); ++it)
	{
		VIcon *v=*it;

        if(!filter || filter->current().find(v) != filter->current().end())
        {
       	   if(v->show(vnode))
           {
           	   lst << v->pixId_;
           }
        }
	}

	return lst;
}

QString VIcon::toolTip(VNode *vnode,VParamSet *filter)
{
	if(filter->isEmpty())
		return QString();

	int iconSize=12;
	QString txt;

	for(std::vector<VIcon*>::const_iterator it=itemsVec_.begin(); it != itemsVec_.end(); ++it)
	{
	   VIcon *v=*it;

       if(!filter || filter->current().find(v) != filter->current().end())
       {
      	   if(v->show(vnode))
      	   {
      		 if(txt.isEmpty())
      		 {
      			 txt+="<br><b>Icons:</b><table cellpadding=\'1\'>";
      		 }

      		 txt+="<tr><td><img src=\'" + IconProvider::path(v->pixId_) + "\' width=\'" +
      	           QString::number(iconSize) + "\' height=\'" + QString::number(iconSize) + "\'></td><td>" + v->name() + "</tr>";
      	   }
		}
	}

	if(!txt.isEmpty())
		txt+="</table>";

   	return txt;
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

//Task only
bool  VWaitIcon::show(VNode *n)
{
	if(!n || n->isServer())
		return false;

	return n->isFlagSet(ecf::Flag::WAIT);
}

//==========================================================
// Rerun
//==========================================================

//Task only
bool VRerunIcon::show(VNode *n)
{
	if(!n || n->isServer())
		return false;

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

//Node and server
bool VMessageIcon::show(VNode *n)
{
	if(!n)
		return false;

	return n->isFlagSet(ecf::Flag::MESSAGE);
}

//==========================================================
// Complete
//==========================================================

//Task only
bool VCompleteIcon::show(VNode *n)
{
	if(!n || n->isServer())
		return false;

	if(!n->node())
		return false;

	node_ptr node=n->node();
	if(!node.get()) return false;

	if(n->isDefaultStateComplete())
		return true;

	if(AstTop* t = node->completeAst())
	{
		if(t->evaluate())
			return true;
	}
	return false;
}

//==========================================================
// Date
//==========================================================

//Node only?
bool  VDateIcon::show(VNode *n)
{
	if(!n || n->isServer())
		return false;

	node_ptr node=n->node();

	if(!node.get()) return false;

	return (node->days().size() > 0 || node->dates().size() > 0);
}

//==========================================================
// Time
//==========================================================

//Node only?
bool  VTimeIcon::show(VNode *n)
{
	if(!n || n->isServer())
		return false;

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

//Node only?
bool VZombieIcon::show(VNode *n)
{
	if(!n)
		return false;

	return n->isFlagSet(ecf::Flag::ZOMBIE);
}

//==========================================================
// Late
//==========================================================

//Node and server
bool VLateIcon::show(VNode *n)
{
	if(!n || n->isServer())
		return false;

	return n->isFlagSet(ecf::Flag::LATE);
}

//==========================================================
// Slow
//==========================================================

//Server only
bool VSlowIcon::show(VNode *n)
{
	if(!n || !n->isServer())
		return false;

	return n->isFlagSet(ecf::Flag::LATE);
}
