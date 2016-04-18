//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AttributeEditor.hpp"

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <map>

static std::map<std::string,AttributeEditorFactory*>* makers = 0;

//=========================================================================
//
// AttributeEditorFactory
//
//=========================================================================

AttributeEditorFactory::AttributeEditorFactory(const std::string& type)
{
    if(makers == 0)
        makers = new std::map<std::string,AttributeEditorFactory*>;

    (*makers)[type] = this;
}

AttributeEditorFactory::~AttributeEditorFactory()
{
    // Not called
}

AttributeEditor* AttributeEditorFactory::create(const std::string& type,VInfo_ptr info,QWidget* parent)
{
    std::map<std::string,AttributeEditorFactory*>::iterator j = makers->find(type);
    if(j != makers->end())
        return (*j).second->make(info,parent);

    return 0;
}


//=========================================================================
//
// AttributeEditor
//
//=========================================================================


AttributeEditor::AttributeEditor(VInfo_ptr info,QWidget* parent) : QDialog(parent), info_(info)
{
    Q_ASSERT(info && info->isAttribute() && info->attribute());
}

void AttributeEditor::edit(VInfo_ptr info,QWidget *parent)
{
    Q_ASSERT(info && info->isAttribute() && info->attribute());
    VAttribute* a=info->attribute();

    Q_ASSERT(a->type());
    //Q_ASSERT(a->type()->name() == "label");

    AttributeEditor* e=AttributeEditorFactory::create(a->type()->strName(),info,parent);
    e->exec();
    e->deleteLater();
}

void AttributeEditor::accept()
{
    apply();
    QDialog::accept();
}
