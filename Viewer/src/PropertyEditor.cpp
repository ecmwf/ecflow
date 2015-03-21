//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PropertyEditor.hpp"

#include <QDebug>
#include <QBoxLayout>

#include <QtTreePropertyBrowser>
#include <QtVariantEditorFactory>
#include <QtGroupPropertyManager>

#include "VConfig.hpp"
#include "VProperty.hpp"

PropertyEditor::PropertyEditor(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout *vb=new QVBoxLayout();
    setLayout(vb);

    browser_=new QtTreePropertyBrowser(this);
    factory_=new QtVariantEditorFactory(this);

    vb->addWidget(browser_);

    build();
}

PropertyEditor::~PropertyEditor()
{
}

//Build the property tree from the the definitions
void PropertyEditor::build()
{
    //Loop over the property groups

	const std::vector<VProperty*>& groups=VConfig::instance()->groups();

	for(std::vector<VProperty*>::const_iterator it=groups.begin();it != groups.end(); it++)
    {
        VProperty *vGroup=*it;

        //We only handle editable groups
        if(!vGroup->editable())
            continue;

        //Create editor group manager
        QtGroupPropertyManager *groupManager = new QtGroupPropertyManager;

        //Create an editor property group
        QtProperty* groupProp = groupManager->addProperty(vGroup->labelText());

        //Register it in the property map
        confMap_[groupProp]=vGroup;

        //Loop over the children of the group
        Q_FOREACH(VProperty* vProp,vGroup->children())
        {
            //Add each item to the the editor
            addItem(vProp,groupProp);
        }

        //Add edtor group to browser
        browser_->addProperty(groupProp);
    }

	browser_->show();
}

void PropertyEditor::addItem(VProperty* vProp,QtProperty* parentProp)
{
    //We only handle editable properties
    if(!vProp->editable())
        return;

    //Is it a group?
    if(vProp->hasChildren())
    {
        //Create an editor group manager
        QtGroupPropertyManager *groupManager = new QtGroupPropertyManager;

        //Create an editor property group
        QtProperty* groupProp = groupManager->addProperty(vProp->labelText());

        //Register it in the property map
        confMap_[groupProp]=vProp;

        //Add theid editor property to its parent
        parentProp->addSubProperty(groupProp);

        //Loop over the children of the group
        Q_FOREACH(VProperty* chProp,vProp->children())
        {
             //Add each item to the the editor
            addItem(chProp,groupProp);
        }
    }
    else
    {
        //Create manager
        QtVariantPropertyManager *variantManager=new QtVariantPropertyManager;

        QVariant::Type vType=vProp->defaultValue().type();

        //We cannot handle these values.
        if(vType == QVariant::Invalid)
        	return;

        QtVariantProperty *prop =
            variantManager->addProperty(vType,vProp->labelText());

        //Register it in the property map
        confMap_[prop]=vProp;

        prop->setToolTip(vProp->toolTip());
        prop->setValue(vProp->value());

        //Add to group
        parentProp->addSubProperty(prop);

        //Set factory
        browser_->setFactoryForManager(variantManager, factory_);
    }
}

void PropertyEditor::editAccepted()
{
    //Loop over the top level properties (groups) in the browser
    Q_FOREACH(QtProperty* gp, browser_->properties())
    {
        //Sync the changes to VConfig
        syncToConfig(gp);
    }
}

void PropertyEditor::syncToConfig(QtProperty *prop)
{
    qDebug() << " prop:" << prop->propertyName() << prop->propertyName() <<  prop->isModified();

    //If the roperty value has been changed.
    if(prop->hasValue()/*prop->isModified()*/)
    {
        //We lookup the corresponding VProperty in in VConfig
        //and set its current value.
        QMap<QtProperty*,VProperty*>::iterator it = confMap_.find(prop);
        if(it != confMap_.end())
        {
            if(QtVariantProperty *vp=static_cast<QtVariantProperty*>(prop))
            {
                qDebug() << "   value:  " << vp->value();
                it.value()->setValue(vp->value());
            }
        }
    }

    //Go through all the children.
    Q_FOREACH(QtProperty* sp,prop->subProperties())
    {
        syncToConfig(sp);
    }
}

