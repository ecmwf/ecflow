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

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"

AttributeEditor::AttributeEditor(VInfo_ptr info,QWidget* parent) : QDialog(parent), info_(info)
{    
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    Q_ASSERT(info_ && info_->isAttribute() && info_->attribute());
    messageLabel_->hide();
    
    attachInfo();
}

AttributeEditor::~AttributeEditor()
{
    detachInfo();
}    

void AttributeEditor::edit(VInfo_ptr info,QWidget *parent)
{
    Q_ASSERT(info && info->isAttribute() && info->attribute());
    VAttribute* a=info->attribute();
    Q_ASSERT(a->type());

    if(AttributeEditor* e=AttributeEditorFactory::create(a->type()->strName(),info,0))
    {
        e->show();
        //e->deleteLater();
    }
}

void AttributeEditor::addForm(QWidget* w)
{
    mainLayout_->insertWidget(3,w,1);
}

void AttributeEditor::accept()
{
    apply();
    QDialog::accept();
}

void AttributeEditor::attachInfo()
{
    if(info_)
    {
        if(info_->server())
            info_->server()->addServerObserver(this);
        
        info_->addObserver(this);
    } 
}

void AttributeEditor::detachInfo()
{
    if(info_)
    {
        if(info_->server())
            info_->server()->removeServerObserver(this);
        
        info_->removeObserver(this);
    }    
}


void AttributeEditor::notifyServerDelete(ServerHandler* server)
{
    if(info_ && info_->server() == server)
    {
        close();
        deleteLater();
    }
}
    
//This must be called at the beginning of a reset
void AttributeEditor::notifyBeginServerClear(ServerHandler* server)
{
    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {
            messageLabel_->showWarning("Server <b>" + QString::fromStdString(server->name()) + "</b> is being reloaded. \
                   Until it is finished only <b>limited functionalty</b> is avaliable in the Info Panel!");

            messageLabel_->startLoadLabel();

            //setSuspended(true);
        }
    }
}

//This must be called at the end of a reset
void AttributeEditor::notifyEndServerScan(ServerHandler* server)
{
    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {
            messageLabel_->hide();
            messageLabel_->clear();

            //We try to ressurect the info. We have to do it explicitly because it is not guaranteed
            //that notifyEndServerScan() will be first called on the VInfo then on the InfoPanel. So it
            //is possible that the node exists but is still set to NULL in VInfo.
            info_->regainData();

            //If the node is not available dataLost() will be called.
            if(!info_->node())
                return;

            //setSuspended(false);
        }
    }
}
