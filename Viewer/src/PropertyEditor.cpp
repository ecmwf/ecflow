//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PropertyEditor.hpp"

#include <QDebug>
#include <QGroupBox>
#include <QLabel>

#include "PropertyLine.hpp"
#include "VConfig.hpp"
#include "VProperty.hpp"

PropertyEditor::PropertyEditor(QWidget* parent) //, group_(vGroup)
{
    setupUi(this);

    grid_->setColumnStretch(1,1);
}

PropertyEditor::~PropertyEditor()
{
}

void PropertyEditor::edit(VProperty * vGroup)
{
	 group_=vGroup;

	 headerLabel_->setText(group_->param("desc"));

	 build();
}


//Build the property tree from the the definitions
void PropertyEditor::build()
{






	//Loop over the property groups

	//const std::vector<VProperty*>& groups=VConfig::instance()->groups();

	//for(std::vector<VProperty*>::const_iterator it=groups.begin();it != groups.end(); it++)
   // {
        //VProperty *vGroup=*it;

		VProperty *vGroup=group_;

        //We only handle editable groups
        //if(!vGroup->editable())
        //    //continue;
        //	return;

        //Create editor group manager
        //QtGroupPropertyManager *groupManager = new QtGroupPropertyManager;

        //Create an editor property group
       // QtProperty* groupProp = groupManager->addProperty(vGroup->labelText());

        //Register it in the property map
        //confMap_[groupProp]=vGroup;

        int row=0;

        qDebug() << vGroup->name();

        //Loop over the children of the group
        Q_FOREACH(VProperty* vProp,vGroup->children())
        {

        	qDebug() << vProp->name();

        	addItem(vProp,grid_);


        	/*if(vProp->param(labelText().isEmpty())
        	{
        		QGridLayout *grid=new QGridLayout();
        		grid->setColumnStretch(1,1);
        		layout_->addLayout(grid);

        		//Add each item to the the editor
        		addItem(vProp,grid);
        	}
        	else
        	{
        		//addItem(vProp,0);
        	}*/

        	//MvQRequestPanelLine::build(*this,p);
        	/*PropertyLine* item = PropertyLineFactory::create(vProp,this);

        	if(item)
        	{
        		layout_->addWidget(item->label(),row,0);
        		layout_->addWidget(item->item(),row,1);
        		row++;
        	}*/
        	//p->init(w);

        	//addItem(vProp,NULL);
        }

        //Add editor group to browser
        //browser_->addProperty(groupProp);
  //  }

	//browser_->show();
}

void PropertyEditor::addItem(VProperty* vProp,QGridLayout* gridLayout)
{
    if(vProp->name() == "line")
    {
    	PropertyLine* item = PropertyLineFactory::create(vProp->link(),this);

    	if(item)
    	{
    		item->reset(vProp->link()->value());

    		int row=gridLayout->rowCount();

    		QLabel* lw=item->label();
    		if(lw)
    		{
    			gridLayout->addWidget(lw,row,0,Qt::AlignLeft);
    			gridLayout->addWidget(item->item(),row,1,Qt::AlignLeft);
    		}
    		else
    		{
    			gridLayout->addWidget(item->item(),row,0,1,2,Qt::AlignLeft);
    		}

    		QWidget *bw=item->button();
    		if(bw)
    			gridLayout->addWidget(bw,row,2);
    	}

    }
    else if(vProp->name() == "group")
    {
		QGroupBox *groupBox = new QGroupBox(vProp->param("title"));
		QGridLayout *grid=new QGridLayout();
		grid->setColumnStretch(1,1);
		groupBox->setLayout(grid);
		gridLayout=grid;

		//add it to the main layout
		int row=grid_->rowCount();
		grid_->addWidget(groupBox,row,0,2,3);

		//Loop over the children of the group
		Q_FOREACH(VProperty* chProp,vProp->children())
		{
		    //Add each item to the the editor
		    addItem(chProp,gridLayout);
		}

    }

    /*
	//We only handle editable properties
    if(!vProp->editable())
        return;

    //Is it a group?
    if(vProp->hasChildren())
    {
        //Create an editor group manager


    	//If the property has a label it is a group!!!
    	if(!gridLayout && !vProp->labelText().isEmpty())
    	{
    		QGroupBox *groupBox = new QGroupBox(vProp->labelText());
    		QGridLayout *grid=new QGridLayout();
    		grid->setColumnStretch(1,1);
    		groupBox->setLayout(grid);
    		gridLayout=grid;
    	}

        //Loop over the children of the group
        Q_FOREACH(VProperty* chProp,vProp->children())
        {
             //Add each item to the the editor
            addItem(chProp,gridLayout);
        }
    }

    else if(gridLayout)
    {
    	PropertyLine* item = PropertyLineFactory::create(vProp,this);

    	if(item)
    	{
    		 item->reset(vProp->value());

    		 int row=gridLayout->rowCount();

    		 QLabel* lw=item->label();
    		 if(lw)
    		 {
    			 gridLayout->addWidget(lw,row,0,Qt::AlignLeft);
    			 gridLayout->addWidget(item->item(),row,1,Qt::AlignLeft);
    		 }
    		 else
    		 {
    			 gridLayout->addWidget(item->item(),row,0,1,2,Qt::AlignLeft);
    		 }

    	     QWidget *bw=item->button();
    	     if(bw)
    	    	 gridLayout->addWidget(bw,row,2);
    	}

    	/*

    	QVariant::Type vType=vProp->defaultValue().type();
    	qDebug() << vProp->labelText() << vType << QVariant();


        //We cannot handle these values.
        if(vType == QVariant::Invalid)
        	return;

        if(vType != QVariant::Color)
        {
        	//Create manager
        	//QtVariantPropertyManager *variantManager=new QtVariantPropertyManager;
        	QtVariantPropertyManager *variantManager=new VariantManager;


        	QtVariantProperty* prop =variantManager->addProperty(vType,vProp->labelText());

        	//Register it in the property map
        	confMap_[prop]=vProp;

        	prop->setToolTip(vProp->toolTip());
        	prop->setValue(vProp->value());

        	//Add to group
        	if(parentProp)
        		parentProp->addSubProperty(prop);
        	else
        		browser_->addProperty(prop);

        	//Set factory
        	browser_->setFactoryForManager(variantManager, factory_);
        }
        else
        {
        	QtCustomColorPropertyManager *variantManager=new QtCustomColorPropertyManager;

        	QtProperty* prop =variantManager->addProperty(vProp->labelText());

        	prop->setToolTip(vProp->toolTip());
        	variantManager->setValue(prop,vProp->value().value<QColor>());

        	//Add to group
        	if(parentProp)
        	     parentProp->addSubProperty(prop);
        	else
        	     browser_->addProperty(prop);

        	QtCustomColorEditorFactory* f=new QtCustomColorEditorFactory(this);
        	browser_->setFactoryForManager(variantManager, f);
        }

    }

    else
    {
    	assert(0);
    }

    */
}

void PropertyEditor::editAccepted()
{
    //Loop over the top level properties (groups) in the browser
    /*Q_FOREACH(QtProperty* gp, browser_->properties())
    {
        //Sync the changes to VConfig
        syncToConfig(gp);
    }*/
}

/*
void PropertyEditor::syncToConfig(QtProperty *prop)
{
    qDebug() << " prop:" << prop->propertyName() << prop->propertyName() <<  prop->isModified();

    //If the property value has been changed.
    if(prop->hasValue())
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
}*/


