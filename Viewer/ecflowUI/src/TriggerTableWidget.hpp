/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TriggerTableWidget_HPP
#define ecflow_viewer_TriggerTableWidget_HPP

#include <QDialog>

#include "VInfo.hpp"
#include "ecflow/node/Aspect.hpp"
#include "ui_TriggerTableWidget.h"

class TriggerTableItem;
class TriggerTableModel;
class TriggerTableCollector;
class TriggerItemWidget;
class TriggeredScanner;
class VComboSettings;

class TriggerTableWidget : public QWidget, private Ui::triggerTableWidget {
    Q_OBJECT

public:
    explicit TriggerTableWidget(QWidget* parent = nullptr);
    ~TriggerTableWidget() override;

    void setInfo(VInfo_ptr, bool);
    VInfo_ptr info() const { return info_; }

    void clear();
    void clearSelection();
    void setTriggeredScanner(TriggeredScanner* scanner);
    bool dependency() const { return dependency_; }
    void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect);
    void writeSettings(VComboSettings* vs);
    void readSettings(VComboSettings* vs);

public Q_SLOTS:
    void slotShowDependencyInfo(bool);

protected Q_SLOTS:
    void slotTriggerSelection(TriggerTableItem* item);
    void slotTriggerClicked(TriggerTableItem*);
    void slotTriggeredSelection(TriggerTableItem* item);
    void slotTriggeredClicked(TriggerTableItem*);
    void on_depInfoCloseTb__clicked();
    void anchorClicked(const QUrl& link);

Q_SIGNALS:
    void linkSelected(VInfo_ptr);
    void infoPanelCommand(VInfo_ptr, QString);
    void dashboardCommand(VInfo_ptr, QString);
    void depInfoWidgetClosureRequested();

private:
    void resumeSelection();
    void beginTriggerUpdate();
    void endTriggerUpdate();

    VInfo_ptr info_;
    TriggerTableCollector* nodeCollector_;
    TriggerTableCollector* triggerCollector_;
    TriggerTableCollector* triggeredCollector_;
    TriggeredScanner* triggeredScanner_{nullptr};
    TriggerTableModel* nodeModel_;
    TriggerTableModel* triggerModel_;
    TriggerTableModel* triggeredModel_;
    VInfo_ptr lastSelectedItem_;
    QString depLabelText_;
    bool dependency_{false};
};

#endif /* ecflow_viewer_TriggerTableWidget_HPP */
