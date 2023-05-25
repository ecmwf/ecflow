//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ShortcutHelpDialog.hpp"

#include <QCloseEvent>
#include <QFile>
#include <QSettings>

#include "DirectoryHandler.hpp"
#include "SessionHandler.hpp"
#include "ui_ShortcutHelpDialog.h"

ShortcutHelpDialog::ShortcutHelpDialog(QWidget* parent) : QDialog(parent), ui_(new Ui::ShortcutHelpDialog) {
    ui_->setupUi(this);

    // Set css for the text formatting
    QString cssDoc =
        "td {padding-left: 3px; padding-right: 10px; paddig-top: 1px; padding-bottom: 1px; background-color: #F3F3F3;color: #323232;} \
                    td.title {padding-left: 8px; padding-top: 2px; padding-bottom: 2px;background-color: #4769A0; color:#EBEEF1; font-weight: bold;}\
                    td.maintitle {padding-left: 8px; padding-top: 2px; padding-bottom: 2px;background-color:#765B8F ; color: #eeeeee;}\
                    td.first {font-weight: bold;}\
                    td.empty {background-color: none;}\
                    body {width: 100%; background-color: #E8E8E8;}";

    ui_->tb->setReadOnly(true);
    ui_->tb->document()->setDefaultStyleSheet(cssDoc);

    QString txtFile =
        QString::fromStdString(DirectoryHandler::concatenate(DirectoryHandler::etcDir(), "shortcuts.txt"));

    loadText(txtFile);

    readSettings();
}

void ShortcutHelpDialog::loadText(QString fileName) {
    QString t = "<body><table width=\'100%\'>";
    // t += "<tr><td class=\'maintitle\' colspan=\'2\'><h2>EcFlowUI</h2></td></tr>";
    t += "<tr><td class=\'empty\' colspan=\'2\'><h3>Keyboard shortcuts</h3></td></tr>";

    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!f.atEnd()) {
            QString line = f.readLine().simplified();
            if (line.startsWith("||")) {
                QStringList lst = line.split("||");
                t += "<tr>";
                if (lst.count() >= 2) {
                    t += "<td class=\'title\' colspan=\'2\'><h4>" + lst[1] + "</h4></td>";
                }
                t += "</tr>";
            }
            else if (line.startsWith("|")) {
                QStringList lst = line.split("|");
                t += "<tr>";
                int cnt = 0;
                Q_FOREACH (QString s, lst) {
                    if (!s.isEmpty()) {
                        if (cnt == 0) {
                            t += "<td class=\'first\'>";
                        }
                        else {
                            t += "<td>";
                        }

                        t += s + "</td>";
                        cnt++;
                    }
                }
                t += "</tr>";
            }
            else if (line.isEmpty()) {
                t += "<tr><td class=\'empty\' colspan=\'2\'></tr>";
            }
        }
    }
    f.close();

    t += "</table></body>";

    ui_->tb->setHtml(t);
}

void ShortcutHelpDialog::accept() {
    writeSettings();
    QDialog::accept();
}

void ShortcutHelpDialog::closeEvent(QCloseEvent* event) {
    writeSettings();
    event->accept();
}

//------------------------------------------
// Settings read/write
//------------------------------------------

void ShortcutHelpDialog::writeSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ShortcutHelpDialog")), QSettings::NativeFormat);

    // We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size", size());
    settings.endGroup();
}

void ShortcutHelpDialog::readSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ShortcutHelpDialog")), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("size")) {
        resize(settings.value("size").toSize());
    }
    else {
        resize(QSize(630, 660));
    }
    settings.endGroup();
}
