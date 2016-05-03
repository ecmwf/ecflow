//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <assert.h>
#include <QMessageBox>


#include "SessionDialog.hpp"
#include "ui_SessionDialog.h"

#include "DirectoryHandler.hpp"
#include "SessionRenameDialog.hpp"


SessionDialog::SessionDialog(QWidget *parent) : QDialog(parent)
{
    //ui->setupUi(this);
    setupUi(this);

    refreshListOfSavedSessions();

    // ensure the correct state of the Save button
    on_sessionNameEdit__textChanged();
    setButtonsEnabledStatus();
}

SessionDialog::~SessionDialog()
{
	//delete ui;
}

void SessionDialog::refreshListOfSavedSessions()
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

void SessionDialog::addSessionToTable(SessionItem *s)
{

	savedSessionsList_->addItem(QString::fromStdString(s->name()));
/*
	int lastRow = sessionsTable_->rowCount()-1;
	sessionsTable_->insertRow(lastRow+1);

	QTableWidgetItem *nameItem    = new QTableWidgetItem(QString::fromStdString(s->name()));
	sessionsTable_->setItem(lastRow+1, 0, nameItem);
*/
}


std::string SessionDialog::selectedSessionName()
{
	QListWidgetItem *ci = savedSessionsList_->currentItem();
	if (ci)
		return ci->text().toStdString();
	else
		return "";
}



// ---------------------------------------------------------------------------------------------
// setButtonsEnabledStatus
// - checks which session has been chosen and enables/disables the action buttons appropriately
// ---------------------------------------------------------------------------------------------

void SessionDialog::setButtonsEnabledStatus()
{
	std::string name = selectedSessionName();

	// if somehow no session is selected, then we need different logic
	if (name.empty())
	{
		cloneButton_   ->setEnabled(false);
		deleteButton_  ->setEnabled(false);
		renameButton_  ->setEnabled(false);
		switchToButton_->setEnabled(false);
	}
	else
	{
		// the default session is special and cannot be deleted or renamed
		bool enable = (name == "default") ? false : true;
		deleteButton_  ->setEnabled(enable);
		renameButton_  ->setEnabled(enable);
		switchToButton_->setEnabled(true);  // always available for a valid session
		cloneButton_   ->setEnabled(true);
	}
}


bool SessionDialog::validSaveName(const std::string &name)
{
/*
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
	}*/
}

void SessionDialog::on_sessionNameEdit__textChanged()
{
	 // only allow to save a non-empty session name
//	saveButton_->setEnabled(!sessionNameEdit_->text().isEmpty());
}

void SessionDialog::on_savedSessionsList__currentRowChanged(int currentRow)
{
	setButtonsEnabledStatus();
}

void SessionDialog::on_cloneButton__clicked()
{
	std::string sessionName = selectedSessionName();
	assert(!sessionName.empty());  // it should not be possible for the name to be empty

	SessionRenameDialog renameDialog;
	renameDialog.exec();

	int result = renameDialog.result();
	if (result == QDialog::Accepted)
	{
		std::string newName = renameDialog.newName();
		// XXX TODO: check it does not clash?
		SessionHandler::instance()->copySession(sessionName, newName);
	}
}


void SessionDialog::on_saveButton__clicked()
{
/*
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
	}*/
}

// called when the user clicks the Save button
//void SaveSessionAsDialog::accept()
//{
//
//}
