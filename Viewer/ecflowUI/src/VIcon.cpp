//============================================================================
// Copyright 2009-2017 ECMWF.
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

#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ExprAst.hpp"
#include "Submittable.hpp"

#include "DirectoryHandler.hpp"
#include "IconProvider.hpp"
#include "UserMessage.hpp"
#include "VConfigLoader.hpp"
#include "VFilter.hpp"
#include "VNode.hpp"
#include "VProperty.hpp"
#include "VConfig.hpp"
#include "VSettings.hpp"

std::map<std::string,VIcon*> VIcon::items_;
std::vector<VIcon*> VIcon::itemsVec_;
std::vector<std::string> VIcon::lastNames_;

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
    explicit VWaitIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VRerunIcon : public VIcon
{
public:
    explicit VRerunIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VNodeLogIcon : public VIcon
{
public:
    explicit VNodeLogIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VCompleteIcon : public VIcon
{
public:
    explicit VCompleteIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VTimeIcon : public VIcon
{
public:
    explicit VTimeIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VTimeFreeIcon : public VIcon
{
public:
    explicit VTimeFreeIcon(const std::string& name) : VIcon(name) {}
    bool show(VNode*);
};

class VDateIcon : public VIcon
{
public:
    explicit VDateIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VZombieIcon : public VIcon
{
public:
    explicit VZombieIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VLateIcon : public VIcon
{
public:
    explicit VLateIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VSlowIcon : public VIcon
{
public:
    explicit VSlowIcon(const std::string& name) : VIcon(name) {}
	bool show(VNode*) override;
};

class VKilledIcon : public VIcon
{
public:
    explicit VKilledIcon(const std::string& name) : VIcon(name) {}
    bool show(VNode*) override;
};

class VMigratedIcon : public VIcon
{
public:
    explicit VMigratedIcon(const std::string& name) : VIcon(name) {}
    bool show(VNode*) override;
};

//==========================================================
//
// Create VIcon instances
//
//==========================================================

//This also defines the order the icons will appear in the views
static VNodeLogIcon nodeLogIcon("message");
static VRerunIcon rerunIcon("rerun");
static VCompleteIcon completeIcon("complete");
static VLateIcon lateIcon("late");
static VTimeIcon timeIcon("time");
static VTimeFreeIcon timeFreeIcon("time_free");
static VDateIcon dateIcon("date");
static VWaitIcon waitIcon("wait");
static VZombieIcon zombieIcon("zombie");
static VKilledIcon killedIcon("killed");
static VSlowIcon slowIcon("slow");
static VMigratedIcon migratedIcon("migrated");

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
= default;

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

	return nullptr;
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

        if(!filter || filter->isSet(v))
        {
       	   if(v->show(vnode))
           {
           	   lst << v->pixId_;
           }
        }
	}

	return lst;
}

//Create the pixmap containing all the relevant icons for the given node according to the filter.
int VIcon::pixmapNum(VNode *vnode,VParamSet *filter)
{
    if(!vnode)
        return 0;

    int ret=0;

    for(std::vector<VIcon*>::const_iterator it=itemsVec_.begin(); it != itemsVec_.end(); ++it)
    {
        VIcon *v=*it;
        if(!filter || filter->isSet(v))
        {
           if(v->show(vnode))
           {
               ret++;
           }
        }
    }
    return ret;
}


QString VIcon::toolTip(VNode *vnode,VParamSet *filter)
{
    if(!filter || filter->isEmpty())
		return QString();

	int iconSize=16;
	QString txt;

	for(std::vector<VIcon*>::const_iterator it=itemsVec_.begin(); it != itemsVec_.end(); ++it)
	{
	   VIcon *v=*it;

       if(!filter || filter->isSet(v))
       {
      	   if(v->show(vnode))
      	   {
      		 if(txt.isEmpty())
      		 {
      			 txt+="<br><b>Icons:</b><table cellpadding=\'1\'>";
      		 }

      		 txt+="<tr><td><img src=\'" + IconProvider::path(v->pixId_) + "\' width=\'" +
      	           QString::number(iconSize) + "\' height=\'" + QString::number(iconSize) + "\'></td><td>" + v->shortDescription() + "</tr>";
      	   }
		}
	}

	if(!txt.isEmpty())
		txt+="</table>";

   	return txt;
}

QString VIcon::shortDescription() const
{
	QString v;
	if(prop_)
		v=prop_->param("shortDesc");

	if(v.isEmpty())
		v=name();

	return v;
}
void VIcon::names(std::vector<std::string>& v)
{
    for(std::map<std::string,VIcon*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
        v.push_back(it->first);
}

void VIcon::saveLastNames()
{
    lastNames_.clear();
    for(std::map<std::string,VIcon*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
        lastNames_.push_back(it->first);

    std::string iconFile = DirectoryHandler::concatenate(DirectoryHandler::configDir(), "last_icons.txt");
    VSettings vs(iconFile);
    vs.clear();
    vs.put("icons",lastNames_);
    vs.write();
}

void VIcon::initLastNames()
{
    //It has to be called only once
    assert(lastNames_.empty());
    std::string iconFile = DirectoryHandler::concatenate(DirectoryHandler::configDir(), "last_icons.txt");
    VSettings vs(iconFile);
    if(vs.read(false))
        vs.get("icons",lastNames_);
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
bool VNodeLogIcon::show(VNode *n)
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
// Time - hasTimeHolding in ecflowview
//==========================================================

//Node only?
bool VTimeIcon::show(VNode *n)
{
	if(!n || n->isServer())
		return false;

	node_ptr node=n->node();

    if(TimeDepAttrs *attr = node->get_time_dep_attrs())
        return !attr->time_today_cron_is_free();

    return false;
}


//==========================================================
// TimeFree - hasTime in ecflowview
//==========================================================

//Node only?
bool VTimeFreeIcon::show(VNode *n)
{
    if(!n || n->isServer())
        return false;

    if(timeIcon.show(n))
        return false;

    node_ptr node=n->node();
    if(node->timeVec().size() > 0 ||
       node->todayVec().size() > 0 ||
       node->crons().size() > 0)
    {
        return true;
    }
    return false;
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

//==========================================================
// Killed
//==========================================================

//Node only?
bool VKilledIcon::show(VNode *n)
{
    if(!n || n->isServer())
        return false;

    return n->isFlagSet(ecf::Flag::KILLED);
}

//==========================================================
// Migrated
//==========================================================

bool VMigratedIcon::show(VNode *n)
{
    if(n && (n->isSuite() || n->isFamily()))
    {
        return n->isFlagSet(ecf::Flag::ARCHIVED);
    }
    return false;
}
