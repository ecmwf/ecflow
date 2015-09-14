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
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

#include "PropertyLine.hpp"
#include "VConfig.hpp"
#include "VProperty.hpp"

PropertyEditor::PropertyEditor(QWidget* parent) : QWidget(parent), 
    group_(0),
    currentGrid_(0)
{
    setupUi(this);

    grid_->setColumnStretch(1,1);
    grid_->setColumnStretch(2,1);
    grid_->setColumnStretch(3,1);
}

PropertyEditor::~PropertyEditor()
{
}

void PropertyEditor::edit(VProperty * vGroup,QString label)
{
	 clear();

	 group_=vGroup;

	 if(label.isEmpty())
	 {
		 headerLabel_->setText(group_->param("desc"));
	 }
	 else
	 {
		 headerLabel_->setText(label);
	 }
	 build();
}

void PropertyEditor::clear()
{
	lineItems_.clear();

	QLayoutItem* item;
	while((item=vBox_->takeAt(0))!= 0)
	{
		if(QWidget *w=item->widget())
		{
		    grid_->removeWidget(w);
		    delete w;
		}

		delete item;
	}
}


//Build the property tree from the the definitions
void PropertyEditor::build()
{
	if(!group_)
		return;

	//Loop over the children of the group
    Q_FOREACH(VProperty* vProp,group_->children())
    {
        addItem(vProp);

    }
}

void PropertyEditor::addItem(VProperty* vProp)
{
    if(vProp->name() == "line")
    {
    	if (!currentGrid_)
        {
            currentGrid_=new QGridLayout();
            vBox_->addLayout(currentGrid_);
        }    
        addLine(vProp,currentGrid_);
    }     
       
    else if(vProp->name() == "group")
    {
		currentGrid_=0;
        addGroup(vProp);
    }
    else if(vProp->name() == "grid")
    {
        currentGrid_=0;
        addGrid(vProp);
    }    
}

void PropertyEditor::addLine(VProperty *vProp,QGridLayout *gridLayout)
{
	PropertyLine* item = PropertyLineFactory::create(vProp->link(),true,this);

    if(item)
    {
    	item->reset(vProp->link()->value());

        int row=gridLayout->rowCount();

        QLabel* lw=item->label();
        QLabel* slw=item->suffixLabel();

        if(lw)
        {
        	gridLayout->addWidget(lw,row,0,Qt::AlignLeft);

            if(slw)
            {
            	QHBoxLayout* hb=new QHBoxLayout;
                hb->addWidget(item->item());
                hb->addWidget(slw);
                gridLayout->addLayout(hb,row,1,Qt::AlignLeft);
            }
            else
            {
                gridLayout->addWidget(item->item(),row,1,Qt::AlignLeft);
            }
        }
        else
        {
        	gridLayout->addWidget(item->item(),row,0,1,2,Qt::AlignLeft);
        }

        QWidget *bw=item->button();
        if(bw)
            gridLayout->addWidget(bw,row,2);


        QToolButton* defTb=item->defaultTb();
        if(defTb)
        {
           gridLayout->addWidget(defTb,row,3);
        }

        QToolButton* masterTb=item->masterTb();
        if(masterTb)
        {
           gridLayout->addWidget(masterTb,row,4);
        }

       lineItems_ << item;
    }
}    

void PropertyEditor::addGroup(VProperty* vProp)
{
   if(vProp->name() != "group")
      return;
   
        QGroupBox *groupBox = new QGroupBox(vProp->param("title"));
        groupBox->setObjectName("editorGroupBox");
        QGridLayout *grid=new QGridLayout();
        grid->setColumnStretch(1,1);
        //grid->setColumnStretch(2,1);
        //grid->setColumnStretch(3,1);

        groupBox->setLayout(grid);
        //gridLayout=grid;

        //add it to the main layout
        //nt row=grid_->rowCount();
        //grid_->addWidget(groupBox,row,0,2,4);
        vBox_->addWidget(groupBox);
        
        //Loop over the children of the group
        Q_FOREACH(VProperty* chProp,vProp->children())
        {
            //Add each item to the the editor
            addLine(chProp,grid);
    }
}

void PropertyEditor::addGrid(VProperty* vProp)
{
    if(vProp->name() != "grid")
        return;
        
    QGroupBox *groupBox = new QGroupBox(vProp->param("title"));
    groupBox->setObjectName("editorGroupBox");
    QGridLayout* grid=new QGridLayout();
    groupBox->setLayout(grid);
        
        //add it to the main layout
        //int row=grid_->rowCount();
        //grid_->addLayout(grid,row,0,2,4);
    
    vBox_->addWidget(groupBox);
        
    //Add header
    for(int i=1; i < 10; i++)
    {    
         QString h=vProp->param("h" + QString::number(i));

         if(h.isEmpty())
         {
             grid->setColumnStretch(i+1,1);
             break;
         }
         
         h+="   ";
         QLabel* hLabel=new QLabel(h);
         grid->addWidget(hLabel,0,i,Qt::AlignHCenter);
    }    

     //Add rows
     Q_FOREACH(VProperty* chProp,vProp->children())
     {
         addGridRow(chProp,grid);
     }
}    


void PropertyEditor::addGridRow(VProperty* vProp,QGridLayout *grid)
{
    if(vProp->name() != "row")
        return;

    int row=grid->rowCount();
    QString labelText=vProp->param("label");
    QLabel* label=new QLabel(labelText);
    grid->addWidget(label,row,0);

    int col=1;
    Q_FOREACH(VProperty* chProp,vProp->children())
    {
        if(chProp->name() == "line")
        {
            PropertyLine* item = PropertyLineFactory::create(chProp->link(),false,this);

            if(item)
            {
                item->reset(chProp->link()->value());

            //QLabel* lw=item->label();
            //QLabel* slw=item->suffixLabel();

                //gridLayout->addWidget(item->item(),row,col,Qt::AlignLeft);

            /*QWidget *bw=item->button();
            if(bw)
                gridLayout->addWidget(bw,row,2);*/


            QToolButton* defTb=item->defaultTb();
            if(defTb)
            {
                QHBoxLayout *hb=new QHBoxLayout();
                hb->addWidget(item->item());
                hb->addWidget(defTb);
                hb->addSpacing(15);
                hb->addStretch(1);
                grid->addLayout(hb,row,col);
            }
            else
            {
                 grid->addWidget(item->item(),row,col,Qt::AlignLeft);
            }    

                lineItems_ << item;
                col++;
            }
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

