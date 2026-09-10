/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeSearchWidget_HPP
#define ecflow_viewer_NodeSearchWidget_HPP

#include <QAbstractItemModel>
#include <QDialog>
#include <QElapsedTimer>
#include <QSettings>
#include <QWidget>

#include "ServerFilter.hpp"
#include "VInfo.hpp"
#include "ui_NodeSearchWidget.h"

class NodeQuery;
class NodeQueryEngine;
class NodeQueryResultModel;

class NodeSearchWidget : public QWidget, protected Ui::NodeSearchWidget {
    Q_OBJECT

public:
    explicit NodeSearchWidget(QWidget* parent = nullptr);
    ~NodeSearchWidget() override;

    void setServerFilter(ServerFilter*);
    void setRootNode(VInfo_ptr);

    void writeSettings(QSettings& settings);
    void readSettings(const QSettings& settings);

public Q_SLOTS:
    void slotFind();
    void slotStop();

protected Q_SLOTS:
    void slotShowDefPanel(bool);
    void slotShowQueryPanel(bool);
    void slotClose();
    void slotQueryStarted();
    void slotQueryFinished();
    void slotQueryEnabledChanged(bool queryEnabled);

Q_SIGNALS:
    void closeClicked();
    void selectionChanged(VInfo_ptr);
    void infoPanelCommand(VInfo_ptr, QString);

private:
    void adjustColumns();
    void adjustButtonState();
    void adjustButtonState(bool);

    NodeQuery* query_{nullptr};
    NodeQueryEngine* engine_;
    NodeQueryResultModel* model_;
    bool columnsAdjusted_{false};
    QElapsedTimer elapsed_;
};

#endif /* ecflow_viewer_NodeSearchWidget_HPP */
