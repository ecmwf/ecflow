/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ChangeNotify.hpp"

#include <cstdlib>
#include <map>

#include <QDebug>
#include <QSortFilterProxyModel>

#include "ChangeNotifyDialog.hpp"
#include "ChangeNotifyModel.hpp"
#include "ChangeNotifyWidget.hpp"
#include "ServerHandler.hpp"
#include "Sound.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VNode.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

static std::map<std::string, ChangeNotify*> items;

static AbortedNotify abortedNotify("aborted");
static ChangeNotify restaredtNotify("restarted");
static ChangeNotify lateNotify("late");
static ChangeNotify zombieNotify("zombie");
static ChangeNotify aliasNotify("alias");

ChangeNotifyDialog* ChangeNotify::dialog_ = nullptr;

//==============================================
//
// ChangeNotify
//
//==============================================

ChangeNotify::ChangeNotify(const std::string& id)
    : id_(id),
      data_(new VNodeList()),
      model_(new ChangeNotifyModel()),
      proxyModel_(new QSortFilterProxyModel())

{
    model_->resetData(data_);
    proxyModel_->setSourceModel(model_);
    proxyModel_->setDynamicSortFilter(true);

    items[id] = this;
}

ChangeNotify::~ChangeNotify() {
    delete data_;
    delete model_;
    delete proxyModel_;
}

ChangeNotifyModel* ChangeNotify::model() const {
    return model_; // proxyModel_;
}

void ChangeNotify::add(VNode* node, bool popup, bool sound) {
    data_->add(node);
    // proxyModel_->invalidate();

    if (popup) {
        dialog()->setCurrent(this);
        dialog()->show();
        dialog()->raise();
    }
    else {
        if (!dialog()->isVisible())
            dialog()->setCurrent(this);
        else
            dialog()->raise();
    }

    if (sound) {
        bool sys = true;
        std::string fName;
        int loop = 0;
        if (VProperty* p = prop_->findChild("sound_file_type")) {
            sys = (p->value() == "system");
        }

        if (sys) {
            if (VProperty* p = prop_->findChild("sound_system_file")) {
                fName = p->valueAsStdString();
            }
        }

        if (fName.empty())
            return;

        if (VProperty* p = prop_->findChild("sound_loop")) {
            loop = p->value().toInt();
        }

        if (sys) {
            Sound::instance()->playSystem(fName, loop);
        }
    }
}

void ChangeNotify::remove(VNode* node) {
    data_->remove(node);
}

void ChangeNotify::setEnabled(bool en) {
    enabled_ = en;

    if (!enabled_) {
        data_->clear();
    }

    proxyModel_->invalidate();

    if (dialog_) {
        dialog()->setEnabled(this, en);
    }

    ChangeNotifyWidget::setEnabled(id_, en);
}

void ChangeNotify::setProperty(VProperty* prop) {
    prop_ = prop;

    if (VProperty* p = prop->findChild("use_status_colour"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("fill_colour"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("text_colour"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("count_fill_colour"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("count_text_colour"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("sound_file_type"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("sound_system_file")) {
        p->addObserver(this);

        QStringList lst;
        const std::vector<std::string>& vals = Sound::instance()->sysSounds();
        for (const auto& val : vals) {
            lst << QString::fromStdString(val);
        }
        p->setParam("values", lst.join("/"));
        p->setParam("values_label", lst.join("/"));
        p->setParam("dir", QString::fromStdString(Sound::instance()->sysDir()));
        p->addObserver(this);
    }

    if (VProperty* p = prop->findChild("sound_user_file"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("sound_volume"))
        p->addObserver(this);

    if (VProperty* p = prop->findChild("sound_loop"))
        p->addObserver(this);
}

void ChangeNotify::notifyChange(VProperty* prop) {
    Q_ASSERT(prop);

    if (prop->name().contains("sound", Qt::CaseInsensitive))
        return;

    else if (prop->name() == "max_item_num") {
        data_->setMaxNum(prop->value().toInt());
    }
    // The central settings changed
    else if (prop == propEnabled_) {
        // Check if there is any loaded server with this
        // notification enabled
        bool hasEnabledServer = ServerHandler::checkNotificationState(id_);
        updateNotificationState(hasEnabledServer);
    }

    dialog()->updateSettings(this);
    ChangeNotifyWidget::updateSettings(id_);
}

void ChangeNotify::updateNotificationState(bool hasEnabledServer) {
    if (propEnabled_)
        setEnabled(propEnabled_->value().toBool() || hasEnabledServer);
    else
        setEnabled(hasEnabledServer);
}

void ChangeNotify::clearData() {
    data_->clear();
    // proxyModel_->invalidate();
}

void ChangeNotify::showDialog(ChangeNotify* notifier) {
    if (notifier)
        dialog()->setCurrent(notifier);

    dialog()->show();
    dialog()->raise();
}

QColor ChangeNotify::fillColour() const {
    if (prop_)
        if (VProperty* p = prop_->findChild("fill_colour"))
            return p->value().value<QColor>();

    return {};
}

QColor ChangeNotify::textColour() const {
    if (prop_)
        if (VProperty* p = prop_->findChild("text_colour"))
            return p->value().value<QColor>();

    return {};
}

QColor ChangeNotify::countFillColour() const {
    if (prop_)
        if (VProperty* p = prop_->findChild("count_fill_colour"))
            return p->value().value<QColor>();

    return {};
}

QColor ChangeNotify::countTextColour() const {
    if (prop_)
        if (VProperty* p = prop_->findChild("count_text_colour"))
            return p->value().value<QColor>();

    return {};
}

QString ChangeNotify::toolTip() const {
    return (prop_) ? (prop_->param("tooltip")) : QString();
}

QString ChangeNotify::widgetText() const {
    return (prop_) ? (prop_->param("widgetText")) : QString();
}

//-----------------------------------
//
// Static methods
//
//-----------------------------------

void ChangeNotify::add(const std::string& id, VNode* node, bool popup, bool sound) {
    if (ChangeNotify* obj = ChangeNotify::find(id)) {
        obj->add(node, popup, sound);
    }
}

void ChangeNotify::remove(const std::string& id, VNode* node) {
    if (ChangeNotify* obj = ChangeNotify::find(id)) {
        obj->remove(node);
    }
}

void ChangeNotify::updateNotificationStateFromServer(const std::string& id, bool en) {
    if (ChangeNotify* obj = ChangeNotify::find(id)) {
        obj->updateNotificationState(en);
    }
}

ChangeNotify* ChangeNotify::find(const std::string& id) {
    auto it = items.find(id);
    if (it != items.end())
        return it->second;

    return nullptr;
}

void ChangeNotify::load(VProperty* group) {
    UI_FUNCTION_LOG

    if (group->name() == "notification") {
        for (int i = 0; i < group->children().size(); i++) {
            VProperty* p = group->children().at(i);
            if (ChangeNotify* obj = ChangeNotify::find(p->strName())) {
                obj->setProperty(p);
            }
        }

        // This should be observed by each notification object
        if (VProperty* p = group->find("notification.settings.max_item_num")) {
            for (auto& item : items) {
                p->addObserver(item.second);
                item.second->data_->setMaxNum(p->value().toInt());
            }
        }
    }
    else if (group->name() == "server") {
        for (auto& item : items) {
            item.second->loadServerSettings();
        }
    }
#if 0
	else if(group->name() == "nstate")
	{
		for(std::map<std::string,ChangeNotify*>::iterator it=items.begin(); it != items.end(); ++it)
		{
			it->second->loadNodeState();
		}
	}
#endif
}

// Called only once during init
void ChangeNotify::loadServerSettings() {
    UI_FUNCTION_LOG

    UiLog().dbg() << " id=" << id_;

    std::string v("server.notification." + id_ + ".enabled");

    UiLog().dbg() << " property=" << v;

    if (VProperty* p = VConfig::instance()->find(v)) {
        propEnabled_ = p;
        p->addObserver(this);
        setEnabled(p->value().toBool());
    }
    else {
        UiLog().err() << "  Error!  Unable to find property: " << v;
    }
}

ChangeNotifyDialog* ChangeNotify::dialog() {
    if (!dialog_) {
        dialog_ = new ChangeNotifyDialog();
        for (auto& item : items) {
            dialog_->add(item.second);
        }

        for (auto& item : items) {
            dialog_->setEnabled(item.second, item.second->isEnabled());
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

void ChangeNotify::populate(ChangeNotifyWidget* w) {
    for (auto& item : items) {
        w->addTb(item.second);
    }
}

//==================================================
//
// AbortedNotify
//
//==================================================

QColor AbortedNotify::fillColour() const {
    bool useState = false;
    if (prop_)
        if (VProperty* p = prop_->findChild("use_status_colour"))
            useState = p->value().toBool();

    if (useState) {
        if (VProperty* nsp = VConfig::instance()->find("nstate.aborted")) {
            if (VProperty* master = nsp->findChild("fill_colour")) {
                return master->value().value<QColor>();
            }
        }
    }

    return ChangeNotify::fillColour();
}

QColor AbortedNotify::textColour() const {
    bool useState = false;
    if (prop_)
        if (VProperty* p = prop_->findChild("use_status_colour"))
            useState = p->value().toBool();

    if (useState) {
        if (VProperty* nsp = VConfig::instance()->find("nstate.aborted")) {
            if (VProperty* master = nsp->findChild("font_colour")) {
                return master->value().value<QColor>();
            }
        }
    }

    return ChangeNotify::textColour();
}

static SimpleLoader<ChangeNotify> loaderNotify("notification");
static SimpleLoader<ChangeNotify> loaderServerNotify("server");
