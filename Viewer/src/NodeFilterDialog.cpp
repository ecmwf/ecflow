//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================



//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <QCloseEvent>
#include <QDebug>
#include <QSettings>

#include "NodeFilterDialog.hpp"
#include "VConfig.hpp"

NodeFilterDialog::NodeFilterDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    //setAttribute(Qt::WA_DeleteOnClose);

	QString wt=windowTitle();
	wt+="  -  " + QString::fromStdString(VConfig::instance()->appLongName());
	setWindowTitle(wt);

	connect(applyPb_,SIGNAL(clicked()),
	    		this,SLOT(accept()));

    connect(cancelPb_,SIGNAL(clicked()),
    		this,SLOT(reject()));

    editor_->setQueryTeCanExpand(true);


    //Read the qt settings
    readSettings();
}

NodeFilterDialog::~NodeFilterDialog()
{
}

void NodeFilterDialog::setQuery(NodeQuery* q)
{
	editor_->setQuery(q);
}

NodeQuery* NodeFilterDialog::query() const
{
	return editor_->query();
}

void NodeFilterDialog::setServerFilter(ServerFilter* sf)
{
	editor_->setServerFilter(sf);
}

void NodeFilterDialog::closeEvent(QCloseEvent * event)
{
	event->accept();
	writeSettings();
}

void NodeFilterDialog::accept()
{
	writeSettings();
    QDialog::accept();
}


void NodeFilterDialog::reject()
{
	writeSettings();
	QDialog::reject();
}

//------------------------------------------
// Settings read/write
//------------------------------------------

void NodeFilterDialog::writeSettings()
{
	QSettings settings("ECMWF","ecFlowUI-NodeFilterDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	//settings.setValue("current",list_->currentRow());
	settings.endGroup();
}

void NodeFilterDialog::readSettings()
{
	QSettings settings("ECMWF","ecFlowUI-NodeFilterDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(550,540));
	}

	/*if(settings.contains("current"))
	{
		int current=settings.value("current").toInt();
		if(current >=0)
			list_->setCurrentRow(current);
	}*/
	settings.endGroup();
}

