//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QMessageBox>


#include "SaveSessionAsDialog.hpp"
#include "ui_SaveSessionAsDialog.h"

#include "DirectoryHandler.hpp"


SaveSessionAsDialog::SaveSessionAsDialog(QWidget *parent) : QDialog(parent)
{
    //ui->setupUi(this);
    setupUi(this);

    refreshListOfSavedSessions();

    // ensure the correct state of the Save button
    on_sessionNameEdit__textChanged();
}

SaveSessionAsDialog::~SaveSessionAsDialog()
{
	//delete ui;
}

void SaveSessionAsDialog::refreshListOfSavedSessions()
{
	//sessionsTable_->clearContents();
	savedSessionsList_->clear();

	// get the list of existing sessions
	int numSessions = SessionHandler::instance()->numSessions();
	for (int i = 0; i < numSessions; i++)
	{
		SessionItem *s = SessionHandler::instance()->sessionFromIndex(i);
		addSessionToTable(s);
	}
}

void SaveSessionAsDialog::addSessionToTable(SessionItem *s)
{

	savedSessionsList_->addItem(QString::fromStdString(s->name()));
/*
	int lastRow = sessionsTable_->rowCount()-1;
	sessionsTable_->insertRow(lastRow+1);

	QTableWidgetItem *nameItem    = new QTableWidgetItem(QString::fromStdString(s->name()));
	sessionsTable_->setItem(lastRow+1, 0, nameItem);
*/
}

bool SaveSessionAsDialog::validSaveName(const std::string &name)
{
	QString boxName(QObject::tr("Save session"));
	// name empty?
	if (name.empty())
	{
		QMessageBox::critical(0,boxName, tr("Please enter a name for the session"));
		return false;
	}


	// is there already a session with this name?
	bool sessionWithThisName = (SessionHandler::instance()->find(name) != NULL);

	if (sessionWithThisName)
	{
		QMessageBox::critical(0,boxName, tr("A session with that name already exists - please choose another name"));
		return false;
	}
	else
	{
		return true;
	}
}

void SaveSessionAsDialog::on_sessionNameEdit__textChanged()
{
	 // only allow to save a non-empty session name
	saveButton_->setEnabled(!sessionNameEdit_->text().isEmpty());
}


void SaveSessionAsDialog::on_saveButton__clicked()
{
	std::string name = sessionNameEdit_->text().toStdString();

	if (validSaveName(name))
	{
		SessionItem* newSession = SessionHandler::instance()->copySession(SessionHandler::instance()->current(), name);
		if (newSession)
		{
			//SessionHandler::instance()->add(name);
			refreshListOfSavedSessions();
			SessionHandler::instance()->current(newSession);
			QMessageBox::information(0,tr("Session"), tr("Session saved"));
		}
		else
		{
			QMessageBox::critical(0,tr("Session"), tr("Failed to save session"));
		}
		close();
	}
}

// called when the user clicks the Save button
//void SaveSessionAsDialog::accept()
//{
//
//}
