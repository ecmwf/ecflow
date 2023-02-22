//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeSearchWindow.hpp"

#include <QCloseEvent>
#include <QDebug>
#include <QSettings>

#include "SessionHandler.hpp"
#include "VConfig.hpp"
#include "WidgetNameProvider.hpp"

NodeSearchWindow::NodeSearchWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    QString wt = windowTitle() + "  -  " + QString::fromStdString(VConfig::instance()->appLongName());
    setWindowTitle(wt);

    connect(queryWidget_, SIGNAL(closeClicked()), this, SLOT(closeIt()));

    // Read the qt settings
    readSettings();

    WidgetNameProvider::nameChildren(this);
}

NodeSearchWindow::~NodeSearchWindow() = default;

NodeSearchWidget* NodeSearchWindow::queryWidget() const {
    return queryWidget_;
}

void NodeSearchWindow::closeEvent(QCloseEvent* event) {
    queryWidget_->slotStop(); // The search thread might be running!!
    event->accept();
    writeSettings();
}

void NodeSearchWindow::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return)
        queryWidget_->slotFind();
    QMainWindow::keyReleaseEvent(event);
}

void NodeSearchWindow::closeIt() {
    writeSettings();
    close();
}

void NodeSearchWindow::slotOwnerDelete() {
    close();
    deleteLater();
}

//------------------------------------------
// Settings read/write
//------------------------------------------

void NodeSearchWindow::writeSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("NodeSearchDialog")), QSettings::NativeFormat);

    // We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("geometry", saveGeometry());
    queryWidget_->writeSettings(settings);
    settings.endGroup();
}

void NodeSearchWindow::readSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("NodeSearchDialog")), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
    else {
        resize(QSize(550, 540));
    }

    queryWidget_->readSettings(settings);
    settings.endGroup();
}
