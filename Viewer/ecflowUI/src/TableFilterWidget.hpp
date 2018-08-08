//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TABLEFILTERWIDGET_INC_
#define TABLEFILTERWIDGET_INC_

#include "ui_TableFilterWidget.h"

#include <QWidget>

class NodeFilterDef;
class ServerFilter;

class TableFilterWidget : public QWidget, private Ui::TableFilterWidget
{
Q_OBJECT

public:
    explicit TableFilterWidget(QWidget *parent=nullptr);
    ~TableFilterWidget() override = default;

    void build(NodeFilterDef*,ServerFilter*);

public Q_SLOTS:
	void slotEdit();
	void slotDefChanged();
	void slotHeaderFilter(QString column,QPoint globalPos);
	void slotTotalNumChanged(int);

private:
	NodeFilterDef* filterDef_;
	ServerFilter* serverFilter_;
};

#endif

