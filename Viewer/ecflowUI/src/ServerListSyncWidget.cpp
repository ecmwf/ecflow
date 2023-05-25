//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerListSyncWidget.hpp"

#include <QDebug>

#include "ServerFilter.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"
#include "ViewerUtil.hpp"

ServerListSyncWidget::ServerListSyncWidget(QWidget* parent) : QWidget(parent) {
    setupUi(this);

    title_->setShowTypeTitle(false);
    errorLabel_->setShowTypeTitle(false);
    browser_->setProperty("log", "1");

    QFont f;
    QFontMetrics fm(f);
    typeList_->setFixedWidth(ViewerUtil::textWidth(fm, "Host/port changed (2222)"));

    reload();

    connect(typeList_,
            SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this,
            SLOT(slotTypeChanged(QListWidgetItem*, QListWidgetItem*)));
}

ServerListSyncWidget::~ServerListSyncWidget() = default;

void ServerListSyncWidget::reload() {
    title_->clear();
    errorLabel_->clear();
    filesTe_->clear();
    typeList_->clear();
    browser_->clear();

    QString t;
    auto manager = ServerList::instance()->systemFileManager();
    if (!manager) {
        return;
    }

    // Set the list
    if (manager->syncDate().isValid()) {
        if (manager->hasSyncChange()) {
            t = "Your copy of system server list was updated at " + manager->syncDate().toString("yyyy-MM-dd HH:mm") +
                " and some <u>changes</u> were found.";

            title_->showInfo(t);

            auto items   = manager->changedItems();
            int matchCnt = 0, addedCnt = 0, setCnt = 0, unsetCnt = 0;
            for (auto item : items) {
                if (item->type() == ServerListSyncChangeItem::MatchChange)
                    matchCnt++;
                else if (item->type() == ServerListSyncChangeItem::AddedChange)
                    addedCnt++;
                else if (item->type() == ServerListSyncChangeItem::SetSysChange)
                    setCnt++;
                else if (item->type() == ServerListSyncChangeItem::UnsetSysChange)
                    unsetCnt++;
            }

            if (matchCnt > 0) {
                auto* item = new QListWidgetItem("Host/port changed (" + QString::number(matchCnt) + ")");
                item->setData(Qt::UserRole, ServerListSyncChangeItem::MatchChange);
                typeList_->addItem(item);
            }
            if (unsetCnt > 0) {
                auto* item = new QListWidgetItem("Server removed (" + QString::number(unsetCnt) + ")");
                item->setData(Qt::UserRole, ServerListSyncChangeItem::UnsetSysChange);
                typeList_->addItem(item);
            }
            if (addedCnt > 0) {
                auto* item = new QListWidgetItem("New server (" + QString::number(addedCnt) + ")");
                item->setData(Qt::UserRole, ServerListSyncChangeItem::AddedChange);
                typeList_->addItem(item);
            }
            if (setCnt > 0) {
                auto* item = new QListWidgetItem("Marked as system (" + QString::number(setCnt) + ")");
                item->setData(Qt::UserRole, ServerListSyncChangeItem::SetSysChange);
                typeList_->addItem(item);
            }

            typeList_->setCurrentRow(0);
        }
        else {
            t = "Your copy of the system server list was updated at " +
                manager->syncDate().toString("yyyy-MM-dd HH:mm") + " but <u>no changes</u> were found.";
            title_->showInfo(t);
            typeList_->hide();
            browser_->hide();
        }
    }
    else {
        t = "Your copy of the system server list has not been updated!";
        title_->showInfo(t);
        typeList_->hide();
        browser_->hide();
    }

    t = "List of loaded input files:";
    if (!manager->fetchedFiles().empty()) {
        t += "<ul>";
        for (auto p : manager->fetchedFiles()) {
            t += "<li>" + QString::fromStdString(p) + "</li>";
        }
        t += "</ul>";
    }

    if (!manager->unfetchedFiles().empty()) {
        t += "The following input files could not be accessed/loaded:";
        t += "<ul>";
        for (auto p : manager->unfetchedFiles()) {
            t += "<li>" + QString::fromStdString(p) + "</li>";
        }
        t += "</ul>";

        errorLabel_->showError("Failed to access/load some system server list files (see the Files tab for details)");

        if (!manager->hasSyncChange()) {
            tabWidget_->setCurrentIndex(0);
        }
    }

    filesTe_->setHtml(t);
}

void ServerListSyncWidget::slotTypeChanged(QListWidgetItem* item, QListWidgetItem*) {
    if (!item)
        return;

    auto manager = ServerList::instance()->systemFileManager();
    if (!manager) {
        return;
    }
    auto items = manager->changedItems();
    ServerListSyncChangeItem::ChangeType type =
        static_cast<ServerListSyncChangeItem::ChangeType>(item->data(Qt::UserRole).toInt());

    QString s;

    for (auto item : items) {
        if (item->type() == type) {
            if (!s.isEmpty())
                s += "<hr>";

            switch (type) {
                case ServerListSyncChangeItem::MatchChange:
                    s += buildMatchChange(item);
                    break;
                case ServerListSyncChangeItem::UnsetSysChange:
                    s += buildUnsetSysChange(item);
                    break;
                case ServerListSyncChangeItem::AddedChange:
                    s += buildAddedChange(item);
                    break;
                case ServerListSyncChangeItem::SetSysChange:
                    s += buildSetSysChange(item);
                    break;
                default:
                    break;
            }
        }
    }

    browser_->setHtml(s);
}

QString ServerListSyncWidget::buildMatchChange(ServerListSyncChangeItem* t) {
    QString hostTxt = QString::fromStdString(t->local().host());
    QString portTxt = QString::fromStdString(t->local().port());

    QColor col(0, 80, 50);

    if (t->sys().host() != t->local().host()) {
        hostTxt += " -> " + QString::fromStdString(t->sys().host());
        hostTxt = "<font color=\'" + col.name() + "\'>" + hostTxt + "</font>";
    }
    if (t->sys().port() != t->local().port()) {
        portTxt += " -> " + QString::fromStdString(t->sys().port());
        portTxt = "<font color=\'" + col.name() + "\'>" + portTxt + "</font>";
    }

    QString s = buildTable(QString::fromStdString(t->local().name()), hostTxt, portTxt);
    return s;
}

QString ServerListSyncWidget::buildAddedChange(ServerListSyncChangeItem* t) {
#if 0
    QString s="<tr><td class=\'add\'>New server added</td></tr>";
    s+="<tr><td>" + buildTable(QString::fromStdString(t->sys().name()),
                           QString::fromStdString(t->sys().host()),
                           QString::fromStdString(t->sys().port())) + "</td></tr>";

#endif

    QString s = buildTable(QString::fromStdString(t->sys().name()),
                           QString::fromStdString(t->sys().host()),
                           QString::fromStdString(t->sys().port()));
    return s;
}

QString ServerListSyncWidget::buildSetSysChange(ServerListSyncChangeItem* t) {
#if 0
    QString s="<tr><td class=\'set\'>Existing server marked as system</td></tr>";
    s+="<tr><td>" + buildTable(QString::fromStdString(t->sys().name()),
                           QString::fromStdString(t->sys().host()),
                           QString::fromStdString(t->sys().port())) + "</td></tr>";
#endif

    QString s = buildTable(QString::fromStdString(t->sys().name()),
                           QString::fromStdString(t->sys().host()),
                           QString::fromStdString(t->sys().port()));
    return s;
}

QString ServerListSyncWidget::buildUnsetSysChange(ServerListSyncChangeItem* t) {
#if 0
   QString s="<tr><td class=\'set\'>Existing server unmarked as system</td></tr>";
    s+="<tr><td>" + buildTable(QString::fromStdString(t->local().name()),
                           QString::fromStdString(t->local().host()),
                           QString::fromStdString(t->local().port())) + "</td></tr>";
#endif
    QString s = buildTable(QString::fromStdString(t->local().name()),
                           QString::fromStdString(t->local().host()),
                           QString::fromStdString(t->local().port()));

    return s;
}

QString ServerListSyncWidget::buildTable(QString name, QString host, QString port) const {
    QColor col(60, 60, 60);
    return "<table><tr><td><b><font color=\'" + col.name() + "\'>&nbsp;Name:</font></b></td>" + "<td>" + name +
           "</td></tr>" + "<tr><td><b><font color=\'" + col.name() + "\'>&nbsp;Host:</font></b></td>" + "<td>" + host +
           "</td></tr>" + "<tr><td><b><font color=\'" + col.name() + "\'>&nbsp;Port:</font></b></td>" + "<td>" + port +
           "</td></tr></table>";
}
