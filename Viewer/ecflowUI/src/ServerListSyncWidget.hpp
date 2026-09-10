/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ServerListSyncWidget_HPP
#define ecflow_viewer_ServerListSyncWidget_HPP

#include <vector>

#include <QWidget>

#include "ui_ServerListSyncWidget.h"

class ServerListSyncChangeItem;
class QListWidgetItem;

class ServerListSyncWidget : public QWidget, protected Ui::ServerListSyncWidget {
    Q_OBJECT

public:
    explicit ServerListSyncWidget(QWidget* parent = nullptr);
    ~ServerListSyncWidget() override;
    void reload();

protected Q_SLOTS:
    void slotTypeChanged(QListWidgetItem* item, QListWidgetItem*);

private:
    void build();
    QString buildAddedChange(ServerListSyncChangeItem*);
    QString buildMatchChange(ServerListSyncChangeItem*);
    QString buildSetSysChange(ServerListSyncChangeItem* t);
    QString buildUnsetSysChange(ServerListSyncChangeItem* t);
    QString buildTable(QString name, QString host, QString port) const;
};

#endif /* ecflow_viewer_ServerListSyncWidget_HPP */
