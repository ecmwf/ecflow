//============================================================================
// Copyright 2009-2017 ECMWF.
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

#include "ui_TriggerTableWidget.h"

class TriggerTableItem;
class TriggerTableModel;
class TriggerTableCollector;
class TriggerItemWidget;


class TriggerTableWidget : public QWidget, private Ui::triggerTableWidget
{
	Q_OBJECT

public:
    explicit TriggerTableWidget(QWidget *parent = 0);
    ~TriggerTableWidget();

    void setInfo(VInfo_ptr);
    void setTriggerCollector(TriggerTableCollector *tc1,TriggerTableCollector *tc2);
	void clear();
    void beginTriggerUpdate();
    void endTriggerUpdate();

public Q_SLOTS:
    void slotShowDependencyInfo(bool);

protected Q_SLOTS:
    void slotTriggerSelection(TriggerTableItem* item);
    void slotTriggeredSelection(TriggerTableItem* item);
    void on_depInfoCloseTb__clicked();

Q_SIGNALS:
    void depInfoWidgetClosureRequested();

private:
    VInfo_ptr info_;
    TriggerTableCollector* triggerCollector_;
    TriggerTableCollector* triggeredCollector_;
    TriggerTableModel *triggerModel_;
    TriggerTableModel *triggeredModel_;
    QString depLabelText_;
};

#endif
