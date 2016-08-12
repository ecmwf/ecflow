//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerListSyncDialog.hpp"

#include <QtGlobal>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>

#include "IconProvider.hpp"
#include "ServerFilter.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"
#include "SessionHandler.hpp"
#include "VConfig.hpp"

ServerListSyncDialog::ServerListSyncDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    messageLabel_->showWarning("Conflicts were found during syncing the central server list! Please");
}

ServerListSyncDialog::~ServerListSyncDialog()
{

}

void ServerListSyncDialog::init(const std::vector<ServerListSyncConflictItem*>& items)
{
    items_=items;
    for(size_t i=0; i < items_.size(); i++)
    {
        listW_->addItem(QString::fromStdString(items_[i]->local_->name()));
    }

    connect(listW_,SIGNAL(currentRowChanged(int)),
            this,SLOT(slotConflictSelected(int)));
}

void ServerListSyncDialog::slotConflictSelected(int row)
{
    if(row >=0)
    {
        Q_ASSERT(row < items_.size());
        ServerListSyncConflictItem *t=items_[row];
        if(t->type_ == ServerListSyncConflictItem::MatchConflict)
        {
            QString s="Host or/and port does not match for server: <br><br><b>" +
                    QString::fromStdString(t->sys_->name()) +"</b<br>";
            s+="<br><table>";
            s+="<tr><th>System list</th><th>Local list</th></tr>";
            s+="<tr>";
            s+="<td>" + buildTable(QString::fromStdString(t->sys_->name()),
                                   QString::fromStdString(t->sys_->host()),
                                   QString::fromStdString(t->sys_->port())) + "</td>";

            s+="<td>" + buildTable(QString::fromStdString(t->local_->name()),
                                   QString::fromStdString(t->local_->host()),
                                   QString::fromStdString(t->local_->port())) + "</td>";

            s+="</tr></table>";
            browser_->setHtml(s);
        }
    }
}

QString ServerListSyncDialog::buildTable(QString name,QString host,QString port) const
{
    return "<table><tr><td><b>Name</b></td><td>" + name + "</td></tr>" +
                "<tr><td><b>Host</b></td><td>" + host + "</td></tr>" +
                "<tr><td><b>Port</b></td><td>" + port + "</td></tr></table>";
}
