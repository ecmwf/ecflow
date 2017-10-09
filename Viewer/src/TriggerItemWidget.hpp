//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TRIGGERITEMWIDGET_HPP_
#define TRIGGERITEMWIDGET_HPP_

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

#include "ui_TriggerItemWidget.h"

class QActionGroup;
class TriggeredScanner;
class TriggerTableCollector;

class TriggerItemWidget : public QWidget, public InfoPanelItem, protected Ui::TriggerItemWidget
{
  friend class TriggerBrowser;

Q_OBJECT

public:
	explicit TriggerItemWidget(QWidget *parent=0);
    ~TriggerItemWidget();

	void reload(VInfo_ptr);
	QWidget* realWidget();
    void clearContents();

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&);
    void defsChanged(const std::vector<ecf::Aspect::Type>&) {}

    bool dependency() const;

    void writeSettings(VComboSettings* vs);
    void readSettings(VComboSettings* vs);

protected Q_SLOTS:
    void on_dependTb__toggled(bool);
    void on_dependInfoTb__toggled(bool b);
    void on_exprTb__toggled(bool b);
    void scanStarted();
    void scanFinished();
    void scanProgressed(int);
    void slotHandleDefInfoWidgetClosure();
    void slotLinkSelected(VInfo_ptr info);
    void slotInfoPanelCommand(VInfo_ptr info,QString cmd);
    void slotDashboardCommand(VInfo_ptr info,QString cmd);

protected:
    void load();
    void updateState(const ChangeFlags&);
    TriggeredScanner* triggeredScanner() const {return scanner_;}
    void checkActionState();
    void clearTriggers();

    TriggerTableCollector* triggerCollector_;
    TriggerTableCollector* triggeredCollector_;
    TriggeredScanner *scanner_;  
};

#endif

