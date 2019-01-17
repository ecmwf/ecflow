//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef TRIGGERTABLEWIDGET_HPP_
#define TRIGGERTABLEWIDGET_HPP_

#include <QDialog>
#include "VInfo.hpp"

#include "Aspect.hpp"

#include "ui_TriggerTableWidget.h"

class TriggerTableItem;
class TriggerTableModel;
class TriggerTableCollector;
class TriggerItemWidget;
class VComboSettings;

class TriggerTableWidget : public QWidget, private Ui::triggerTableWidget
{
	Q_OBJECT

public:
    explicit TriggerTableWidget(QWidget *parent = 0);
    ~TriggerTableWidget();

    void setInfo(VInfo_ptr);
    void setTriggerCollector(TriggerTableCollector *tc1,TriggerTableCollector *tc2);
	void clear();
    void clearSelection();
    void resumeSelection();
    void beginTriggerUpdate();
    void endTriggerUpdate();
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
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);
    void depInfoWidgetClosureRequested();

private:
    VInfo_ptr info_;
    TriggerTableCollector* nodeCollector_;
    TriggerTableModel *nodeModel_;
    TriggerTableModel *triggerModel_;
    TriggerTableModel *triggeredModel_;
    VInfo_ptr lastSelectedItem_;
    QString depLabelText_;
};

#endif
