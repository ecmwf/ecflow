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

//#ifdef ECFLOW_QT5
//#include <QSoundEffect>
//#endif

#include <QDebug>

#include <map>

static std::map<std::string,ChangeNotify*> items;

static ChangeNotify abortedNotify("aborted");
static ChangeNotify restaredtNotify("restarted");
static ChangeNotify lateNotify("late");
static ChangeNotify zombieNotify("zombie");
static ChangeNotify aliasNotify("alias");

ChangeNotifyDialog* ChangeNotify::dialog_=0;

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


void ChangeNotify::add(const std::string& id,VNode *node,bool popup,bool sound)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->add(node,popup,sound);
	}
}

void ChangeNotify::add(VNode *node,bool popup,bool sound)
{
	//make();

	data_->add(node);

	if(popup)
	{
		dialog()->setCurrentTab(id_);
		dialog()->show();
		dialog()->raise();
	}
	if(sound)
	{
//#ifdef ECFLOW_QT5
//	QSoundEffect effect(dialog_);
//	effect.setSource(QUrl::fromLocalFile("file:/usr/share/xemacs/xemacs-packages/etc/sounds/boing.wav"));
//	effect.setLoopCount(1);
//	effect.setVolume(0.25f);
//	effect.play();
//#endif
	}
}

void ChangeNotify::make()
{
	if(!data_)
	{
		data_=new VNodeList();
		model_=new ChangeNotifyModel();
		model_->setData(data_);
		//dialog_=new ChangeNotifyDialog();
		//dialog_->init(prop_,model_);
	}
}

void ChangeNotify::setEnabled(const std::string& id,bool en)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->setEnabled(en);
	}
}

void ChangeNotify::setEnabled(bool en)
{
	enabled_=en;

	if(!enabled_)
	{
		data_->clear();
	}

	dialog()->setEnabledTab(id_,en);
	ChangeNotifyWidget::setEnabled(id_,en);
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
	for(int i=0; i < group->children().size(); i++)
    {
    	VProperty *p=group->children().at(i);

    	std::string id=p->strName();
    	if(ChangeNotify* obj=ChangeNotify::find(id))
    	{
    		//obj->make();
    		obj->prop_=p;
    	}
    }
}


ChangeNotifyDialog* ChangeNotify::dialog()
{
	if(!dialog_)
	{
		dialog_=new ChangeNotifyDialog();
		for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
		{
			dialog_->addTab(it->first,it->second->prop_,it->second->model_);
		}
	}

	return dialog_;
}

void  ChangeNotify::showDialog(const std::string& id)
{
	dialog()->setCurrentTab(id);
	dialog()->show();
	dialog()->raise();
}

void ChangeNotify::clearData(const std::string& id)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->data()->clear();
	}
}


void ChangeNotify::populate(ChangeNotifyWidget* w)
{
	for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
	{
		w->addTb(it->second);
	}
}

void ChangeNotify::init()
{
	for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
	{
		std::string v("server.notification." + it->first + ".enabled");

		UserMessage::message(UserMessage::DBG, false,"ChangeNotify:init() " + v);

		if(VProperty *p=VConfig::instance()->find(v))
		{
			it->second->setEnabled(p->value().toBool());
		}
		else
		{
			UserMessage::message(UserMessage::ERROR, false,
					std::string("Error! ChangeNotify::init() unable to find property: " + v));
		}
	}
}

static SimpleLoader<ChangeNotify> loaderTable("notification");

