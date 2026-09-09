/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CommandOutputDialog.hpp"

#include <QCloseEvent>
#include <QDebug>
#include <QSettings>

#include "SessionHandler.hpp"
#include "VConfig.hpp"
#include "WidgetNameProvider.hpp"

CommandOutputDialog* CommandOutputDialog::dialog_ = nullptr;

CommandOutputDialog::CommandOutputDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    QString wt = windowTitle();
    wt += "  -  " + QString::fromStdString(VConfig::instance()->appLongName());
    setWindowTitle(wt);

    // connect(queryWidget_,SIGNAL(closeClicked()),
    //		this,SLOT(accept()));

    // Read the qt settings
    readSettings();

    WidgetNameProvider::nameChildren(this);
}

CommandOutputDialog::~CommandOutputDialog() = default;

void CommandOutputDialog::closeEvent(QCloseEvent* event) {
    // queryWidget_->slotStop(); //The search thread might be running!!
    dialog_ = nullptr;
    event->accept();
    writeSettings();
}

void CommandOutputDialog::accept() {
    dialog_ = nullptr;
    writeSettings();
    QDialog::accept();
}

void CommandOutputDialog::reject() {
    dialog_ = nullptr;
    writeSettings();
    QDialog::reject();
}

void CommandOutputDialog::showDialog() {
    if (!dialog_) {
        dialog_ = new CommandOutputDialog(nullptr);
        dialog_->show();
    }
    dialog_->raise();
}

//------------------------------------------
// Settings read/write
//------------------------------------------

void CommandOutputDialog::writeSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("CommandOutputDialog")), QSettings::NativeFormat);

    // We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size", size());
    widget_->writeSettings(settings);
    settings.endGroup();
}

void CommandOutputDialog::readSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("CommandOutputDialog")), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("size")) {
        resize(settings.value("size").toSize());
    }
    else {
        resize(QSize(550, 540));
    }

    widget_->readSettings(settings);

    settings.endGroup();
}
