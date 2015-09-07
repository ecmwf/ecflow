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
#include "VConfigLoader.hpp"
#include "VNode.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

#ifdef ECFLOW_QT5
#include <QSoundEffect>
#endif

#include <QDebug>

#include <map>

static std::map<std::string,ChangeNotify*> items;

static AbortedNotify abortedNotify("aborted");
static RestartedNotify restaredtNotify("restarted");

ChangeNotify::ChangeNotify(const std::string& id) :
	id_(id),
	data_(0),
	model_(0),
	dialog_(0),
	prop_(0)
{
	items[id] = this;
}


void ChangeNotify::add(const std::string& id,VNode *node)
{
	if(ChangeNotify* obj=ChangeNotify::find(id))
	{
		obj->add(node);
	}
}

void ChangeNotify::add(VNode *node)
{
	make();

	data_->add(node);

	dialog()->show();

#ifdef ECFLOW_QT5
	QSoundEffect effect(dialog_);
	effect.setSource(QUrl::fromLocalFile("/usr/share/xemacs/xemacs-packages/etc/sounds/boing.wav"));
	effect.setLoopCount(1);
	effect.setVolume(0.25f);
	effect.play();
	qDebug() << effect.status();
#endif

}

void ChangeNotify::make()
{
	if(!data_)
	{
		data_=new VNodeList();
		model_=new ChangeNotifyModel();
		model_->setData(data_);
		dialog_=new ChangeNotifyDialog();
		dialog_->init(prop_,model_);
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
	for(int i=0; i < group->children().size(); i++)
    {
    	VProperty *p=group->children().at(i);

    	std::string id=p->strName();
    	if(ChangeNotify* obj=ChangeNotify::find(id))
    	{
    		obj->prop_=p;
    	}
    }
}

static SimpleLoader<ChangeNotify> loaderTable("notification");

