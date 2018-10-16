//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ModelColumn.hpp"

#include <map>

#include <QDebug>

#include "DiagData.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VSettingsLoader.hpp"

static std::map<std::string,ModelColumn*> defs;

ModelColumn::ModelColumn(const std::string& id) : id_(id), diagStart_(-1), diagEnd_(-1)
{
	defs[id_]=this;
}

ModelColumn* ModelColumn::def(const std::string& id)
{
	std::map<std::string,ModelColumn*>::const_iterator it=defs.find(id);
	if(it != defs.end())
		return it->second;
	return nullptr;
}

ModelColumn* ModelColumn::tableModelColumn()
{
    return ModelColumn::def("table_columns");
}

//int ModelColumn::indexOf(const std::string& id) const
//{
//    return indexOf(QString::fromStdString(id));
//}

int ModelColumn::indexOf(QString id) const
{
    for(int i=0; i< items_.count(); i++)
    {
        if(items_.at(i)->id_ == id)
            return i;
    }
    return -1;
}

void ModelColumn::loadItem(VProperty *p)
{
    ModelColumnItem* obj=new ModelColumnItem(p->strName());
    obj->label_=p->param("label");
    obj->tooltip_=p->param("tooltip");
    obj->icon_=p->param("icon");
    obj->index_=items_.count();
    items_ << obj;
}

void ModelColumn::loadExtraItem(QString name,QString label)
{
    ModelColumnItem* obj=new ModelColumnItem(name.toStdString(),true);
    obj->label_=label;
    obj->tooltip_=obj->label_;
    //obj->icon_=p->param("icon");
    obj->index_=items_.count();
    obj->editable_=true;

    if(hasDiag())
    {
        items_.insert(diagStart_,obj);
        diagStart_++;
        diagEnd_++;
    }
    else
    {
       items_ << obj;
    }
}

void ModelColumn::loadDiagItem(QString name,QString label)
{
    ModelColumnItem* obj=new ModelColumnItem(name.toStdString(),true);
    obj->label_=label;
    obj->tooltip_=obj->label_;
    //obj->icon_=p->param("icon");
    obj->index_=items_.count();
    obj->editable_=false;
    items_ << obj;
}

void ModelColumn::addExtraItem(QString name,QString label)
{
    if(indexOf(name) !=  -1)
        return;

    //Editable extra items are always inserted in front of the diag items
    int pos=items_.count();
    if(hasDiag())
    {
        pos=diagStart_;
        Q_ASSERT(pos >=0);
    }

    Q_EMIT addItemsBegin(pos,pos);
    //Q_EMIT appendItemBegin();
    loadExtraItem(name,label);
    //Q_EMIT appendItemEnd();
    Q_EMIT addItemsEnd(pos,pos);

    save();
}

void ModelColumn::changeExtraItem(int idx, QString name,QString label)
{
    if(indexOf(name) != -1)
        return;

    if(!isExtra(idx) || !isEditable(idx))
        return;

    Q_EMIT changeItemBegin(idx);

    items_[idx]->id_=name;
    items_[idx]->label_=label;

    Q_EMIT changeItemEnd(idx);

    save();
}

void ModelColumn::removeExtraItem(QString name)
{
    int idx=indexOf(name);
    if(idx != -1 &&
       items_[idx]->isExtra() && items_[idx]->isEditable())
    {
        Q_EMIT removeItemsBegin(idx,idx);

        ModelColumnItem* obj=items_[idx];
        items_.removeAt(idx);
        delete obj;

        if(hasDiag())
        {
            diagStart_--;
            diagEnd_--;
            Q_ASSERT(diagStart_ >= 0);
        }

        Q_EMIT removeItemsEnd(idx,idx);

        save();
    }
}

bool ModelColumn::isSameDiag(DiagData *diag) const
{
    if(diagStart_ >=0 && diagEnd_ >=0 && diag->count() == diagEnd_-diagStart_+1)
    {
        for(int i=diagStart_; i <= diagEnd_; i++)
        {
            if(items_[i]->id_ !=
                    QString::fromStdString(diag->columnName(i-diagStart_)))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

void ModelColumn::setDiagData(DiagData *diag)
{
    if(isSameDiag(diag))
        return;

    //Remove the current diag items
    if(diagStart_ >=0 && diagEnd_ >=0)
    {
        Q_EMIT removeItemsBegin(diagStart_,diagEnd_);
        for(int i=diagStart_; i <= diagEnd_; i++)
        {
            ModelColumnItem* obj=items_[diagStart_];
            items_.removeAt(diagStart_);
            delete obj;
        }
        Q_EMIT removeItemsEnd(diagStart_,diagEnd_);
        diagStart_=-1;
        diagEnd_=-1;
    }

    //Add the current diag items to the back of the items
    if(diag->count() <=0)
        return;

    diagStart_=items_.count();
    diagEnd_=items_.count()+diag->count()-1;
    Q_ASSERT(diagStart_ >= 0);
    Q_ASSERT(diagStart_ <= diagEnd_);
    Q_EMIT addItemsBegin(diagStart_,diagEnd_);

    for(int i=0; i < diag->count(); i++)
    {
        QString n=QString::fromStdString(diag->columnName(i));
        loadDiagItem(n,n);// these are not editable items!!!
    }
    Q_EMIT addItemsEnd(diagStart_,diagEnd_);
}

void ModelColumn::save()
{
    if(!configPath_.empty())
    {
        if(VProperty* prop=VConfig::instance()->find(configPath_))
        {
            QStringList lst;
            for(int i=0; i < items_.count(); i++)
            {
                if(items_[i]->isExtra() && items_[i]->isEditable())
                    lst << items_[i]->id_;
            }
            if(lst.isEmpty())
                prop->setValue(prop->defaultValue());
            else
                prop->setValue(lst.join("/"));
        }
    }
}

void ModelColumn::load(VProperty* group)
{
    Q_ASSERT(group);

    ModelColumn* m=new ModelColumn(group->strName());
	for(int i=0; i < group->children().size(); i++)
    {
        VProperty *p=group->children().at(i);
        m->loadItem(p);
    }

    //Define extra config property
    m->configPath_=group->param("__config__").toStdString();
}

//Called via VSettingsLoader after the users settings are read
void ModelColumn::loadSettings()
{
    for(std::map<std::string,ModelColumn*>::iterator it=defs.begin(); it != defs.end(); ++it)
    {
        it->second->loadUserSettings();
    }
}

//Load user defined settings
void ModelColumn::loadUserSettings()
{
    //Load extra config
    if(!configPath_.empty())
    {
        if(VProperty* p=VConfig::instance()->find(configPath_))
        {
            QString pval=p->valueAsString();
            if(!pval.isEmpty() && pval != "__none__")
            {
                Q_FOREACH(QString s,pval.split("/"))
                {
                    loadExtraItem(s,s);
                }
           }
        }
    }
}

ModelColumnItem::ModelColumnItem(const std::string& id, bool extra) :
    index_(-1), extra_(extra), editable_(extra)
{
	id_=QString::fromStdString(id);
}

static SimpleLoader<ModelColumn> loaderQuery("query_columns");
static SimpleLoader<ModelColumn> loaderTable("table_columns");
static SimpleLoader<ModelColumn> loaderZombie("zombie_columns");
static SimpleLoader<ModelColumn> loaderTriggerGraph("trigger_graph_columns");
static SimpleLoader<ModelColumn> loaderOutput("output_columns");

static SimpleSettingsLoader<ModelColumn> settingsLoader;
