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

PropertyEditor::PropertyEditor(QWidget* parent) : QWidget(parent), group_(0)
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
	if(!group_)
		return;

	//Loop over the children of the group
    Q_FOREACH(VProperty* vProp,group_->children())
    {
        addItem(vProp,grid_);

    }
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

    		lineItems_ << item;
    	}

    }
    else if(vProp->name() == "group")
    {
		QGroupBox *groupBox = new QGroupBox(vProp->param("title"));
		groupBox->setObjectName("editorGroupBox");
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
}

bool PropertyEditor::applyChange()
{
    bool changed=false;
	//Loop over the top level properties (groups) in the browser
    Q_FOREACH(PropertyLine* item, lineItems_)
    {
        //Sync the changes to VConfig
    	if(item->applyChange())
    	{
    		changed=true;
    	}
    }

    return changed;
}

