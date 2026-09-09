/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TriggerItemWidget_HPP
#define ecflow_viewer_TriggerItemWidget_HPP

#include <QWidget>
#include <QtGlobal>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"
#include "ui_TriggerItemWidget.h"

class QAbstractButton;
class QButtonGroup;
class TriggeredScanner;

class TriggerItemWidget : public QWidget, public InfoPanelItem, protected Ui::TriggerItemWidget {
    friend class TriggerBrowser;

    Q_OBJECT

public:
    explicit TriggerItemWidget(QWidget* parent = nullptr);
    ~TriggerItemWidget() override;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override;
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

    bool dependency() const;

    void writeSettings(VComboSettings* vs) override;
    void readSettings(VComboSettings* vs) override;

protected Q_SLOTS:
    void on_dependTb__toggled(bool);
    void on_dependInfoTb__toggled(bool b);
    void on_exprTb__toggled(bool b);
    void scanStarted();
    void scanFinished();
    void scanProgressed(int);
    void slotHandleDefInfoWidgetClosure();
    void slotLinkSelected(VInfo_ptr info);
    void slotInfoPanelCommand(VInfo_ptr info, QString cmd);
    void slotDashboardCommand(VInfo_ptr info, QString cmd);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void slotChangeMode(QAbstractButton*, bool);
#else
    void slotChangeMode(int, bool);
#endif

protected:
    void load();
    void loadTable();
    void loadGraph();
    void rerender() override;
    void updateState(const ChangeFlags&) override;
    TriggeredScanner* triggeredScanner() const { return scanner_; }
    void checkActionState();
    void clearTriggers();
    void showGraphButtons(bool b);

    enum ModeIndex { TableModeIndex = 0, GraphModeIndex = 1 };

    TriggeredScanner* scanner_;
    QButtonGroup* modeGroup_;
    int exprHeight_;
    int exprEmptyHeight_;
};

#endif /* ecflow_viewer_TriggerItemWidget_HPP */
