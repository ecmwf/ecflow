//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "Sound.hpp"
#include "UiLog.hpp"
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
#include <QSortFilterProxyModel>

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
	proxyModel_(0),
	prop_(0)
{
	data_=new VNodeList();
	model_=new ChangeNotifyModel();
	model_->setData(data_);

	proxyModel_=new QSortFilterProxyModel();
	proxyModel_->setSourceModel(model_);
	//proxyModel_->setFilterFixedString("1");
	//proxyModel_->setFilterKeyColumn(0);
	proxyModel_->setDynamicSortFilter(true);

	items[id] = this;
}

ChangeNotifyModel* ChangeNotify::model() const
{
	return model_; //proxyModel_;
}

void ChangeNotify::add(VNode *node,bool popup,bool sound)
{
	data_->add(node);
	//proxyModel_->invalidate();

	if(popup)
	{
		dialog()->setCurrentTab(this);
		dialog()->show();
		dialog()->raise();
	}
	else
	{
		if(!dialog()->isVisible())
			dialog()->setCurrentTab(this);
		else
			dialog()->raise();
	}

	if(sound)
	{
		bool sys=true;
		std::string fName;
		int loop=0;
		if(VProperty* p=prop_->findChild("sound_file_type"))
		{
			sys=(p->value() == "system");
		}

		if(sys)
		{
			if(VProperty* p=prop_->findChild("sound_system_file"))
			{
				fName=p->valueAsString();
			}
		}

		if(fName.empty())
			return;

		if(VProperty* p=prop_->findChild("sound_loop"))
		{
			loop=p->value().toInt();
		}

		if(sys)
		{
			Sound::instance()->playSystem(fName,loop);
		}

	}
}

void ChangeNotify::remove(VNode *node)
{
	data_->remove(node);
}

void ChangeNotify::setEnabled(bool en)
{
	enabled_=en;

	if(!enabled_)
	{
		data_->clear();
	}

	proxyModel_->invalidate();

	if(dialog_)
	{
		dialog()->setEnabledTab(this,en);
	}

	ChangeNotifyWidget::setEnabled(id_,en);
}

void ChangeNotify::setProperty(VProperty* prop)
{
	prop_=prop;

	if(VProperty* p=prop->findChild("fill_colour"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("text_colour"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("count_fill_colour"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("count_text_colour"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("sound_file_type"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("sound_system_file"))
	{
		p->addObserver(this);

		QStringList lst;
		const std::vector<std::string>& vals=Sound::instance()->sysSounds();
        for(std::vector<std::string>::const_iterator it=vals.begin(); it != vals.end(); ++it)
		{
			lst << QString::fromStdString(*it);
		}
		p->setParam("values",lst.join("/"));
		p->setParam("values_label",lst.join("/"));
		p->setParam("dir",QString::fromStdString(Sound::instance()->sysDir()));
		p->addObserver(this);
	}

	if(VProperty* p=prop->findChild("sound_user_file"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("sound_volume"))
		p->addObserver(this);

	if(VProperty* p=prop->findChild("sound_loop"))
		p->addObserver(this);
}

void ChangeNotify::notifyChange(VProperty* prop)
{
	if(prop->name().contains("sound",Qt::CaseInsensitive))
		return;

	if(prop->name() == "max_item_num")
	{
		data_->setMaxNum(prop->value().toInt());
	}

	dialog()->updateSettings(this);
	ChangeNotifyWidget::updateSettings(id_);
}

void ChangeNotify::clearData()
{
	data_->clear();
	//proxyModel_->invalidate();
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

void ChangeNotify::remove(const std::string& id,VNode *node)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->remove(node);
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
        UiLog().dbg() << "ChangeNotify:load() -- > notification";

		for(int i=0; i < group->children().size(); i++)
		{
			VProperty *p=group->children().at(i);
			if(ChangeNotify* obj=ChangeNotify::find(p->strName()))
			{
				obj->setProperty(p);
			}
		}

		//This should be observed by each notification object
		if(VProperty* p=group->find("notification.settings.max_item_num"))
		{
			for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
			{
				p->addObserver(it->second);
				it->second->data_->setMaxNum(p->value().toInt());
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
    UiLog().dbg() << "ChangeNotify::loadServerSettings() --> " << id_;

	std::string v("server.notification." + id_ + ".enabled");
    UiLog().dbg() << " property: " <<  v;

	if(VProperty *p=VConfig::instance()->find(v))
	{
		setEnabled(p->value().toBool());
	}
	else
	{
        UiLog().err() << "  Error!  Unable to find property: " << v;
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

		for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
		{
			dialog_->setEnabledTab(it->second,it->second->isEnabled());
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

		master=nsp->findChild("text_colour");
		local=prop_->findChild("text_colour");

		if(master && local)
		{
			local->setMaster(master,true);
		}
	}

}

static SimpleLoader<ChangeNotify> loaderNotify("notification");
static SimpleLoader<ChangeNotify> loaderServerNotify("server");
static SimpleLoader<ChangeNotify> loaderStateNotify("nstate");
