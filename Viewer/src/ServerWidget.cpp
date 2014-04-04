//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerWidget.hpp"

#include "ServerFilter.hpp"
#include "ServerListModel.hpp"

#include <QDialogButtonBox>
#include <QSettings>

ServerWidget::ServerWidget(ServerFilter *filter,QWidget *parent) :
   QWidget(parent),
   filter_(filter)
{
    setupUi(this);

    serverView->setRootIsDecorated(false);

    model_=new ServerListModel(filter,this);
    serverView->setModel(model_);

}

void ServerWidget::on_editTb_clicked()
{

}

void ServerWidget::on_addTb_clicked()
{

}


void ServerWidget::on_deleteTb_clicked()
{

}

void ServerWidget::on_rescanTb_clicked()
{

}

void ServerWidget::saveSelection()
{
	if(filter_)
		filter_->update(model_->selection());
}

//==============================
//
// ServerDialog
//
//==============================

ServerDialog::ServerDialog(ServerFilter * filter,QWidget *parent) :
   QDialog(parent)
{
	setWindowTitle(tr("Manage server list"));

	QVBoxLayout *vb=new QVBoxLayout;
	vb->setSpacing(0);
	vb->setContentsMargins(1,1,1,1);
	setLayout(vb);

	serverWidget_ =new ServerWidget(filter,this);
	vb->addWidget(serverWidget_,1);

	// Buttonbox
	//QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
        //                              | QDialogButtonBox::Cancel);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vb->addWidget(buttonBox);

	//connect(iconWidget_,SIGNAL(iconSelected(QString)),
	//	this,SLOT(slotIconSelected(QString)));

	//Read settings
	readSettings();

}

ServerDialog::~ServerDialog()
{
	writeSettings();
}

void ServerDialog::setServerFilter(ServerFilter *filter)
{


}

void ServerDialog::accept()
{
	serverWidget_->saveSelection();
  	QDialog::accept();
}

void ServerDialog::reject()
{
  	QDialog::reject();
}

void ServerDialog::writeSettings()
{
	QSettings settings("ECMWF","ECF-ServerDialog");

	//We have to clear it not to remember all the previous windows
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.endGroup();
}

void ServerDialog::readSettings()
{
	QSettings settings("ECMWF","ECF-ServerDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(350,500));
	}

	settings.endGroup();
}

