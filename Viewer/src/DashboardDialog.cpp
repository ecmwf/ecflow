//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "DashboardDialog.hpp"

#include "DashboardWidget.hpp"
#include "SessionHandler.hpp"
#include "WidgetNameProvider.hpp"

#include <QAbstractButton>
#include <QCloseEvent>
#include <QPushButton>
#include <QSettings>

DashboardDialog::DashboardDialog(QWidget *parent) :
	QDialog(parent),
	dw_(0)
{
	setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	//Disable the default button
	Q_FOREACH(QAbstractButton* b,buttonBox_->buttons())
	{
		if(QPushButton* pb=buttonBox_->button(buttonBox_->standardButton(b)) )
			pb->setAutoDefault(false);
	}

	readSettings();

    WidgetNameProvider::nameChildren(this);
}

DashboardDialog::~DashboardDialog()
{
	//dw_->deleteLater();
}

void DashboardDialog::add(DashboardWidget* dw)
{
	dw_=dw;

	//The dialog takes ownership of the widget
	dw_->setParent(this);

	layout_->insertWidget(0,dw_,1);

    connect(dw_,SIGNAL(titleUpdated(QString)),
            this,SLOT(slotUpdateTitle(QString)));

    dw_->populateDialog();
    dw_->readSettingsForDialog();
}

void DashboardDialog::reject()
{
    if(dw_)
        dw_->writeSettingsForDialog();

    writeSettings();
	QDialog::reject();
}

void DashboardDialog::closeEvent(QCloseEvent * event)
{
    if(dw_)
        dw_->writeSettingsForDialog();

    Q_EMIT aboutToClose();
    event->accept();
	writeSettings();
}

void DashboardDialog::slotUpdateTitle(QString txt)
{
    setWindowTitle(txt.remove("<b>").remove("</b>"));
}

void DashboardDialog::slotOwnerDelete()
{
	this->deleteLater();
}

void DashboardDialog::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("DashboardDialog")),
                       QSettings::NativeFormat);

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.endGroup();
}

void DashboardDialog::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("DashboardDialog")),
                       QSettings::NativeFormat);

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(520,500));
	}

	settings.endGroup();
}
