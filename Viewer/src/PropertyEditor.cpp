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
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QToolButton>

#include "ChangeNotifyEditor.hpp"
#include "IconProvider.hpp"
#include "PropertyLine.hpp"
#include "VConfig.hpp"
#include "VProperty.hpp"

PropertyEditor::PropertyEditor(QWidget* parent) : QWidget(parent), 
    group_(0),
    currentGrid_(0),
	holder_(0)
{
    setupUi(this);

    headerWidget_->setProperty("editorHeader","1");
    scArea_->setProperty("editor","1");
    scAreaContents_->setProperty("editorArea","1");

    pixLabel_->clear();
}

PropertyEditor::~PropertyEditor()
{
}

void PropertyEditor::edit(VProperty * vGroup,QPixmap pix)
{
	 clear();

	 group_=vGroup;

	 QString txt=group_->param("desc");
	 headerLabel_->setText(txt);

	 pixLabel_->setPixmap(pix);

	 build();
}

void PropertyEditor::edit(VProperty * vGroup,QString serverName)
{
	 clear();

	 group_=vGroup;

	 headerWidget_->hide();

	 serverName_=serverName;

	 build();
}

void PropertyEditor::clear()
{
	if(holder_)
	{
		vBox_->removeWidget(holder_);
		delete holder_;
		holder_=NULL;
	}

	currentGrid_=0;
	lineItems_.clear();
}


//Build the property tree from the the definitions
void PropertyEditor::build()
{
	if(!group_)
		return;

	assert(holder_==NULL);

	holder_=new QWidget(scAreaContents_);
    holder_->setObjectName("h");
	QVBoxLayout *vb=new QVBoxLayout(holder_);
	vb->setContentsMargins(0,0,0,0);
	vBox_->addWidget(holder_);

	//Loop over the children of the group
    Q_FOREACH(VProperty* vProp,group_->children())
    {
        addItem(vProp,vb,holder_);
    }

    addHelpers();
}

void PropertyEditor::addHelpers()
{
	QMap<std::string,PropertyLine*> lineMap;
	Q_FOREACH(PropertyLine* line,lineItems_)
	{
		lineMap[line->property()->path()]=line;
	}

	Q_FOREACH(PropertyLine* line,lineItems_)
	{
		QString h=line->guiProperty()->param("helpers");
		if(!h.isEmpty())
		{
			Q_FOREACH(QString s,h.split("/"))
			{
				if(PropertyLine* hl=lineMap.value(s.toStdString(),NULL))
				{
					line->addHelper(hl);
				}
			}
		}
	}
}


void PropertyEditor::addItem(VProperty* vProp,QVBoxLayout *layout,QWidget *parent)
{
    if(vProp->name() == "line")
    {
    	if (!currentGrid_)
        {
            currentGrid_=new QGridLayout();
            layout->addLayout(currentGrid_);
        }    
        addLine(vProp,currentGrid_,parent);
    }     
       
    else if(vProp->name() == "group")
    {
		currentGrid_=0;
        addGroup(vProp,layout,parent);
    }
    else if(vProp->name() == "grid")
    {
        currentGrid_=0;
        addGrid(vProp,layout,parent);
    }
    else if(vProp->name() == "custom-notification")
    {
        currentGrid_=0;
        addNotification(vProp,layout,parent);
    }
    else if(vProp->name() == "note")
    {
        if(currentGrid_)
        {
        	addNote(vProp,currentGrid_,parent);
        }
        else
        {
        	addNote(vProp,layout,parent);
        }
    }
    else if(vProp->name() == "tabs")
    {
        currentGrid_=0;
        addTabs(vProp,layout,parent);
    }     

}

PropertyLine* PropertyEditor::addLine(VProperty *vProp,QGridLayout *gridLayout,QWidget *parent)
{
	PropertyLine* item = PropertyLineFactory::create(vProp,true,parent);

    if(item)
    {
    	item->init();
    	//item->reset(vProp->link()->value());

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
            	if(item->canExpand())
            	{
            		QHBoxLayout* hb=new QHBoxLayout;
            	    hb->addWidget(item->item());
            	    gridLayout->addLayout(hb,row,1,Qt::AlignLeft);
            	}
            	else
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

       connect(item,SIGNAL(changed()),
    		   this,SIGNAL(changed()));

       lineItems_ << item;
    }

    return item;
}    

void PropertyEditor::addGroup(VProperty* vProp,QVBoxLayout * layout,QWidget *parent)
{
   if(vProp->name() != "group")
      return;
   
   QGroupBox *groupBox = new QGroupBox(vProp->param("title"),parent);
   groupBox->setObjectName("editorGroupBox");
   QGridLayout *grid=new QGridLayout();
   grid->setColumnStretch(1,1);
   groupBox->setLayout(grid);
   layout->addWidget(groupBox);
        
   currentGrid_=grid;

   //Loop over the children of the group
   Q_FOREACH(VProperty* chProp,vProp->children())
   {
       //Add each item to the the editor
       addItem(chProp,layout,groupBox);
   }
   currentGrid_=0;
}

void PropertyEditor::addGrid(VProperty* vProp,QVBoxLayout *layout,QWidget *parent)
{
    if(vProp->name() != "grid")
        return;
        
    QGroupBox *groupBox = new QGroupBox(vProp->param("title"),parent);
    groupBox->setObjectName("editorGroupBox");
    QGridLayout* grid=new QGridLayout();
    groupBox->setLayout(grid);
        
    layout->addWidget(groupBox);
        
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
         QLabel* hLabel=new QLabel(h,groupBox);
         grid->addWidget(hLabel,0,i,Qt::AlignHCenter);
    }    

     //Add rows
     Q_FOREACH(VProperty* chProp,vProp->children())
     {
         addGridRow(chProp,grid,groupBox);
     }
}    


void PropertyEditor::addGridRow(VProperty* vProp,QGridLayout *grid,QWidget *parent)
{
    if(vProp->name() != "row")
    {
    	if(vProp->name() == "note")
    	{
    		 QLabel *empty=new QLabel(" ",parent);
    		 grid->addWidget(empty,grid->rowCount(),0,1,-1,Qt::AlignVCenter);
    		 QLabel *label=new QLabel("&nbsp;&nbsp;&nbsp;<b>Note:</b> " + vProp->value().toString(),parent);
    		 grid->addWidget(label,grid->rowCount(),0,1,-1,Qt::AlignVCenter);
    	}
    	return;
    }

    int row=grid->rowCount();
    QString labelText=vProp->param("label");
    QLabel* label=new QLabel(labelText,parent);
    grid->addWidget(label,row,0);

    int col=1;
    Q_FOREACH(VProperty* chProp,vProp->children())
    {
        if(chProp->name() == "line")
        {
            PropertyLine* item = PropertyLineFactory::create(chProp,false,parent);

            if(item)
            {
                item->init();
            	//item->reset(chProp->link()->value());

            //QLabel* lw=item->label();
            //QLabel* slw=item->suffixLabel();

                //gridLayout->addWidget(item->item(),row,col,Qt::AlignLeft);

            /*QWidget *bw=item->button();
            if(bw)
                gridLayout->addWidget(bw,row,2);*/


            QToolButton* defTb=item->defaultTb();
            QToolButton* masterTb=item->masterTb();
            if(defTb || masterTb)
            {
                QHBoxLayout *hb=new QHBoxLayout();
                hb->addWidget(item->item());
                if(defTb)
                	hb->addWidget(defTb);
                if(masterTb)
                	hb->addWidget(masterTb);

                hb->addSpacing(15);
                hb->addStretch(1);
                grid->addLayout(hb,row,col);
            }
            else
            {
                 grid->addWidget(item->item(),row,col,Qt::AlignLeft);
            }    

            connect(item,SIGNAL(changed()),
               		   this,SIGNAL(changed()));
                lineItems_ << item;
                col++;
            }
        }    
    }


}

void PropertyEditor::addNotification(VProperty* vProp,QVBoxLayout* layout,QWidget *parent)
{
    if(vProp->name() != "custom-notification")
        return;

    //ChangeNotifyEditor* ne=new ChangeNotifyEditor(parent);

    QTabWidget* tab=new QTabWidget(parent);

    bool useGroup=(vProp->param("group") == "true");

    if(useGroup)
    {
    	QString labelText=vProp->param("title");
    	QGroupBox *groupBox = new QGroupBox(labelText,parent);
    	groupBox->setObjectName("editorGroupBox");
    	QVBoxLayout* vb=new QVBoxLayout();
    	groupBox->setLayout(vb);
    	vb->addWidget(tab);
    	layout->addWidget(groupBox);

    }
    else
    {
    	layout->addWidget(tab);
    }

    //Add rows
    Q_FOREACH(VProperty* chProp,vProp->children())
    {
    	if(chProp->name() == "row")
    	{
    		QString labelText=chProp->param("label");

    		QList<PropertyLine*> lineLst;

    		QWidget* w=new QWidget(parent);
    		QVBoxLayout* vb=new QVBoxLayout(w);
    		//vb->setContentsMargins(4,4,4,4);

            currentGrid_=0;
            
            if(VProperty *root=VConfig::instance()->find(chProp->param("root").toStdString()))
            {
            	QLabel *labelDesc=new QLabel(tr("Description: <b>") + root->param("description") + "</b>",w);
            	//labelDesc->setProperty("editorNotifyHeader","1");
            	vb->addWidget(labelDesc);
            	vb->addSpacing(5);
            }

            int lineLstPos=lineItems_.count();
    		Q_FOREACH(VProperty* lineProp,chProp->children())
    	    {
    	        addItem(lineProp,vb,w);
    	    }
    	    for(int i=lineLstPos; i < lineItems_.count(); i++)
                lineLst << lineItems_[i];

    	    tab->addTab(w,labelText);

    	    //Connect up different components
    	    PropertyLine* enabledLine=0;
    	    PropertyLine* popupLine=0;
    	    PropertyLine* soundLine=0;
    	    Q_FOREACH(PropertyLine* pl,lineLst)
    		{
    			if(pl->property()->name() == "enabled")
    			{
    				enabledLine=pl;
    			}
    			if(pl->property()->name() == "popup")
    			{
    				popupLine=pl;
    			}
    			if(pl->property()->name() == "sound")
    			{
    				soundLine=pl;
    			}
    		}

    		if(enabledLine)
    		{
    			if(popupLine)
    			{
    				connect(enabledLine,SIGNAL(changed(QVariant)),
    						popupLine,SLOT(slotEnabled(QVariant)));
    				//init
    				popupLine->slotEnabled(enabledLine->property()->value());
    			}
    			if(soundLine)
    			{
    				connect(enabledLine,SIGNAL(changed(QVariant)),
    						soundLine,SLOT(slotEnabled(QVariant)));
    				//init
    				soundLine->slotEnabled(enabledLine->property()->value());
    			}
    		}

    		//ne->addRow(labelText,lineLst,w);
    	 }
     }
}

void PropertyEditor::addTabs(VProperty* vProp,QVBoxLayout *layout,QWidget* parent)
{
    if(vProp->name() != "tabs")
        return;
    
    QTabWidget *t=new QTabWidget(parent);
    t->setObjectName("tab");
    layout->addWidget(t);
   
    int col=1;
    Q_FOREACH(VProperty* chProp,vProp->children())
    {
        if(chProp->name() == "tab")
        {
            addTab(chProp,t);
        }    
    }
} 

void PropertyEditor::addTab(VProperty* vProp,QTabWidget* tab)
{
    if(vProp->name() != "tab")
        return;

    QWidget *w=new QWidget(tab);
    QVBoxLayout* vb=new QVBoxLayout();
    w->setLayout(vb);
    
    tab->addTab(w,vProp->param("label"));
    
    Q_FOREACH(VProperty* chProp,vProp->children())
    {
        addItem(chProp,vb,w);
    }

    vb->addStretch(1);
}    

void PropertyEditor::addNote(VProperty* vProp,QVBoxLayout* layout,QWidget *parent)
{
    if(vProp->name() != "note")
        return;

    QString txt=vProp->value().toString();
    txt.replace("%SERVER%",(serverName_.isEmpty())?"?":"<b>" + serverName_ + "</b>");

    layout->addSpacing(5);
    QLabel *label=new QLabel("<i>Note:</i> " + txt,parent);
    layout->addWidget(label);
}

void PropertyEditor::addNote(VProperty* vProp,QGridLayout* layout,QWidget *parent)
{
    if(vProp->name() != "note")
        return;

    QString txt=vProp->value().toString();
    txt.replace("%SERVER%",(serverName_.isEmpty())?"?":"<b>" + serverName_ + "</b>");

    //QLabel *empty=new QLabel(" ",parent);
    //layout->addWidget(empty,layout->rowCount(),0,1,-1,Qt::AlignVCenter);
   	//QLabel *label=new QLabel("&nbsp;&nbsp;&nbsp;<b>Note:</b> " + txt,parent);

    //QFrame* fr=new QFrame(parent);
    //fr->setFrameShape(QFrame::HLine);
    //layout->addWidget(fr,layout->rowCount(),0,1,-1,Qt::AlignVCenter);

    QLabel *label=new QLabel("<table><tr><td><b>&nbsp;&nbsp;&nbsp;</b></td><td><i>Note:</i> " + txt + "</td></tr></table>",parent);
    label->setWordWrap(true);
    layout->addWidget(label,layout->rowCount(),0,1,-1,Qt::AlignVCenter);
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

