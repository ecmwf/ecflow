//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ChangeNotify.hpp"

#include "ChangeNotifyDialog.hpp"
#include "ChangeNotifyModel.hpp"
#include "ChangeNotifyWidget.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VNode.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

#include <stdlib.h>

//#ifdef ECFLOW_QT5
//#include <QSoundEffect>
//#endif

#include <QDebug>

#include <map>

static std::map<std::string,ChangeNotify*> items;

static AbortedNotify abortedNotify("aborted");
static ChangeNotify restaredtNotify("restarted");
static ChangeNotify lateNotify("late");
static ChangeNotify zombieNotify("zombie");
static ChangeNotify aliasNotify("alias");

ChangeNotifyDialog* ChangeNotify::dialog_=0;

//==============================================
//
// ChangeNotify
//
//==============================================

ChangeNotify::ChangeNotify(const std::string& id) :
	id_(id),
	enabled_(false),
	data_(0),
	model_(0),
	prop_(0)
{
	data_=new VNodeList();
	model_=new ChangeNotifyModel();
	model_->setData(data_);

	items[id] = this;
}

void ChangeNotify::add(VNode *node,bool popup,bool sound)
{
	data_->add(node);

	if(popup)
	{
		dialog()->setCurrentTab(this);
		dialog()->show();
		dialog()->raise();
	}
	if(sound)
	{
		const char *soundCmd = "play -q /usr/share/xemacs/xemacs-packages/etc/sounds/boing.wav";
		if (system(soundCmd))
			UserMessage::message(UserMessage::DBG, false,"ChangeNotify:add() could not play sound alert");

//#ifdef ECFLOW_QT5
//	QSoundEffect effect(dialog_);
//	effect.setSource(QUrl::fromLocalFile("file:/usr/share/xemacs/xemacs-packages/etc/sounds/boing.wav"));
//	effect.setLoopCount(1);
//	effect.setVolume(0.25f);
//	effect.play();
//#endif
	}
}


void ChangeNotify::setEnabled(bool en)
{
	enabled_=en;

	if(!enabled_)
	{
		data_->clear();
	}

	dialog()->setEnabledTab(this,en);
	ChangeNotifyWidget::setEnabled(id_,en);
}

void ChangeNotify::setProperty(VProperty* prop)
{
	prop_=prop;

	if(VProperty* p=prop->findChild("fill_colour"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("font_colour"))
		p->addObserver(this);
}

void ChangeNotify::notifyChange(VProperty* prop)
{
	dialog()->updateSettings(this);
	ChangeNotifyWidget::updateSettings(id_);
}

void ChangeNotify::clearData()
{
	data_->clear();
}

void ChangeNotify::showDialog()
{
	dialog()->setCurrentTab(this);
	dialog()->show();
	dialog()->raise();
}


//-----------------------------------
//
// Static methods
//
//-----------------------------------

void ChangeNotify::add(const std::string& id,VNode *node,bool popup,bool sound)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->add(node,popup,sound);
	}
}

void ChangeNotify::setEnabled(const std::string& id,bool en)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->setEnabled(en);
	}
}

ChangeNotify*  ChangeNotify::find(const std::string& id)
{
	std::map<std::string,ChangeNotify*>::iterator it=items.find(id);
	if(it != items.end())
		return it->second;

	return 0;
}

void ChangeNotify::load(VProperty* group)
{
	if(group->name() == "notification")
	{
		UserMessage::message(UserMessage::DBG, false,"ChangeNotify:load() -- > notification");

		for(int i=0; i < group->children().size(); i++)
		{
			VProperty *p=group->children().at(i);
			if(ChangeNotify* obj=ChangeNotify::find(p->strName()))
			{
				obj->setProperty(p);
			}
		}
	}
	else if(group->name() == "server")
	{
		for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
		{
			it->second->loadServerSettings();
		}
	}
	else if(group->name() == "nstate")
	{
		for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
		{
			it->second->loadNodeState();
		}
	}
}

void ChangeNotify::loadServerSettings()
{
	UserMessage::message(UserMessage::DBG, false,"ChangeNotify::loadServerSettings() --> " + id_);

	std::string v("server.notification." + id_ + ".enabled");
	UserMessage::message(UserMessage::DBG, false," property: " +  v);

	if(VProperty *p=VConfig::instance()->find(v))
	{
		setEnabled(p->value().toBool());
	}
	else
	{
		UserMessage::message(UserMessage::ERROR, false,
			std::string("  Error!  Unable to find property: " + v));
	}
}

ChangeNotifyDialog* ChangeNotify::dialog()
{
	if(!dialog_)
	{
		dialog_=new ChangeNotifyDialog();
		for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
		{
			dialog_->addTab(it->second);
		}
	}

	return dialog_;
}

/*
void  ChangeNotify::showDialog(ChangeNotifyconst std::string& id)
{
	dialog()->setCurrentTab(id);
	dialog()->show();
	dialog()->raise();
}
*/
/*void ChangeNotify::clearData(const std::string& id)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->data()->clear();
	}
}*/

void ChangeNotify::populate(ChangeNotifyWidget* w)
{
	for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
	{
		w->addTb(it->second);
	}
}

//==================================================
//
// AbortedNotify
//
//==================================================

void AbortedNotify::loadNodeState()
{
	if(VProperty *nsp=VConfig::instance()->find("nstate.aborted"))
	{
		VProperty* master=nsp->findChild("fill_colour");
		VProperty* local=prop_->findChild("fill_colour");

		if(master && local)
		{
			local->setMaster(master,true);
		}

		master=nsp->findChild("font_colour");
		local=prop_->findChild("font_colour");

		if(master && local)
		{
			local->setMaster(master,true);
		}
	}

}

static SimpleLoader<ChangeNotify> loaderNotify("notification");
static SimpleLoader<ChangeNotify> loaderServerNotify("server");
static SimpleLoader<ChangeNotify> loaderStateNotify("nstate");
