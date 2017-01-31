//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ModelColumn.hpp"

#include <map>

#include <QDebug>

#include "VConfigLoader.hpp"
#include "VProperty.hpp"

static std::map<std::string,ModelColumn*> defs;

ModelColumn::ModelColumn(const std::string& id) : id_(id)
{
	defs[id_]=this;
}

ModelColumn* ModelColumn::def(const std::string& id)
{
	std::map<std::string,ModelColumn*>::const_iterator it=defs.find(id);
	if(it != defs.end())
		return it->second;
	return NULL;
}


int ModelColumn::indexOf(const std::string& id) const
{
	QString idStr=QString::fromStdString(id);
	for(int i=0; i< items_.count(); i++)
	{
		if(items_.at(i)->id_ == idStr)
			return i;
	}
	return -1;
}

void ModelColumn::load(VProperty* group)
{
	ModelColumn* m=new ModelColumn(group->strName());

	for(int i=0; i < group->children().size(); i++)
    {
    	VProperty *p=group->children().at(i);

    	ModelColumnItem* obj=new ModelColumnItem(p->strName());
        obj->label_=p->param("label");
        obj->tooltip_=p->param("tooltip");
        obj->icon_=p->param("icon");
        obj->index_=i;

        m->items_ << obj;

    }
}

ModelColumnItem::ModelColumnItem(const std::string& id) : index_(-1)
{
	id_=QString::fromStdString(id);
}

static SimpleLoader<ModelColumn> loaderQuery("query_columns");
static SimpleLoader<ModelColumn> loaderTable("table_columns");
static SimpleLoader<ModelColumn> loaderZombie("zombie_columns");
