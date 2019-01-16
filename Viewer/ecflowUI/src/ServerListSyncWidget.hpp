//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLISTSYNCWIDGET_HPP
#define SERVERLISTSYNCWIDGET_HPP

#include <QWidget>

#include <vector>

#include "ui_ServerListSyncWidget.h"

class ServerListSyncChangeItem;
class QListWidgetItem;

class ServerListSyncWidget : public QWidget, protected Ui::ServerListSyncWidget
{
    Q_OBJECT

public:
    ServerListSyncWidget(QWidget *parent = 0);
    ~ServerListSyncWidget();

protected Q_SLOTS:
    void slotTypeChanged(QListWidgetItem* item,QListWidgetItem*);

private:
    void build();
    QString buildAddedChange(ServerListSyncChangeItem*);
    QString buildMatchChange(ServerListSyncChangeItem*);
    QString buildSetSysChange(ServerListSyncChangeItem *t);
    QString buildUnsetSysChange(ServerListSyncChangeItem *t);
    QString buildTable(QString name,QString host,QString port) const;
};

#endif // SERVERLISTSYNCWIDGET_HPP
