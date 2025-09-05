/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "SessionDialog.hpp"

#include <cassert>

#include <QMessageBox>

#include "DirectoryHandler.hpp"
#include "SessionRenameDialog.hpp"
#include "ui_SessionDialog.h"

SessionDialog::SessionDialog(QWidget* parent) : QDialog(parent) {
    // ui->setupUi(this);
    setupUi(this);

    savedSessionsList_->setHeaderLabel("Sessions");

    refreshListOfSavedSessions();
    savedSessionsList_->sortItems(0, Qt::AscendingOrder);

    // what was saved last time?
    std::string lastSessionName = SessionHandler::instance()->lastSessionName();
    int index                   = SessionHandler::instance()->indexFromName(lastSessionName);
    if (index != -1) {
        savedSessionsList_->setCurrentItem(savedSessionsList_->topLevelItem(index)); // select this one in the table
    }

    if (SessionHandler::instance()->loadLastSessionAtStartup()) {
        restoreLastSessionCb_->setCheckState(Qt::Checked);
    }
    else {
        restoreLastSessionCb_->setCheckState(Qt::Unchecked);
    }

    newButton_->setVisible(false); // XXX TODO: enable New Session functionality

    // ensure the correct state of the Save button
    setButtonsEnabledStatus();
}

SessionDialog::~SessionDialog() {
    // delete ui;
}

void SessionDialog::refreshListOfSavedSessions() {

    // sessionsTable_->clearContents();
    savedSessionsList_->clear();

    // get the list of existing sessions
    int numSessions = SessionHandler::instance()->numSessions();
    for (int i = 0; i < numSessions; i++) {
        SessionItem* s = SessionHandler::instance()->sessionFromIndex(i);
        addSessionToTable(s);
    }
}

void SessionDialog::addSessionToTable(SessionItem* s) {
    QStringList lst;
    lst << QString::fromStdString(s->name());
    auto* item = new QTreeWidgetItem(lst);
    savedSessionsList_->addTopLevelItem(item);
}

std::string SessionDialog::selectedSessionName() {
    QTreeWidgetItem* ci = savedSessionsList_->currentItem();
    if (ci) {
        return ci->text(0).toStdString();
    }
    else {
        return "";
    }
}

// ---------------------------------------------------------------------------------------------
// setButtonsEnabledStatus
// - checks which session has been chosen and enables/disables the action buttons appropriately
// ---------------------------------------------------------------------------------------------

void SessionDialog::setButtonsEnabledStatus() {
    std::string name = selectedSessionName();

    // if somehow no session is selected, then we need different logic
    if (name.empty()) {
        cloneButton_->setEnabled(false);
        deleteButton_->setEnabled(false);
        renameButton_->setEnabled(false);
        switchToButton_->setEnabled(false);
    }
    else {
        // the default session is special and cannot be deleted or renamed
        bool enable = (name == "default") ? false : true;
        deleteButton_->setEnabled(enable);
        renameButton_->setEnabled(enable);
        switchToButton_->setEnabled(true); // always available for a valid session
        cloneButton_->setEnabled(true);
    }
}
void SessionDialog::on_savedSessionsList__currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    setButtonsEnabledStatus();
}

void SessionDialog::on_cloneButton__clicked() {
    std::string sessionName = selectedSessionName();
    assert(!sessionName.empty()); // it should not be possible for the name to be empty

    SessionRenameDialog renameDialog;
    renameDialog.setWindowTitle("Clone Session");
    renameDialog.exec();

    int result = renameDialog.result();
    if (result == QDialog::Accepted) {
        std::string newName = renameDialog.newName();
        SessionHandler::instance()->copySession(sessionName, newName);
        refreshListOfSavedSessions();
    }
}

void SessionDialog::on_deleteButton__clicked() {
    std::string sessionName = selectedSessionName();
    assert(!sessionName.empty()); // it should not be possible for the name to be empty

    QString message = tr("Are you sure that you want to delete the session '") + QString::fromStdString(sessionName) +
                      tr("'' from disk?");
    if (QMessageBox::question(nullptr,
                              tr("Confirm: remove session"),
                              message,
                              QMessageBox::Ok | QMessageBox::Cancel,
                              QMessageBox::Cancel) == QMessageBox::Ok) {
        SessionHandler::instance()->remove(sessionName);
        refreshListOfSavedSessions();
    }
}

void SessionDialog::on_renameButton__clicked() {
    std::string sessionName = selectedSessionName();
    assert(!sessionName.empty()); // it should not be possible for the name to be empty

    SessionItem* item = SessionHandler::instance()->find(sessionName);
    assert(item); // it should not be possible for the name to be empty

    SessionRenameDialog renameDialog;
    renameDialog.setWindowTitle("Rename Session");
    renameDialog.exec();

    int result = renameDialog.result();
    if (result == QDialog::Accepted) {
        std::string newName = renameDialog.newName();
        SessionHandler::instance()->rename(item, newName);
        refreshListOfSavedSessions();
    }
}

void SessionDialog::on_switchToButton__clicked() {
    std::string sessionName = selectedSessionName();
    assert(!sessionName.empty()); // it should not be possible for the name to be empty

    SessionItem* item = SessionHandler::instance()->find(sessionName);
    assert(item); // it should not be possible for the name to be empty

    SessionHandler::instance()->current(item); // set this session as the current one

    if (restoreLastSessionCb_->checkState() == Qt::Checked) { // save details of the selected session?
        SessionHandler::instance()->saveLastSessionName();
    }
    else {
        SessionHandler::instance()->removeLastSessionName(); // no, so we can delete the file
    }

    accept(); // close the dialogue and continue loading the main user interface
}
