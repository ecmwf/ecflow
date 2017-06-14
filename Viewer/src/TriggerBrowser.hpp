//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERBROWSER_HPP
#define TRIGGERBROWSER_HPP

#include <QWidget>

#include <set>

#include "ui_TriggerBrowser.h"

#include "VInfo.hpp"

class QPlainTextEdit;
class Highlighter;
class TriggerItemWidget;
class TriggerListCollector;
class TriggerTableCollector;

class TriggerBrowser : public QWidget, protected Ui::TriggerBrowser
{
Q_OBJECT

public:
    explicit TriggerBrowser(QWidget *parent=0);
    ~TriggerBrowser();

    void setOwner(TriggerItemWidget*);
    void clear();
    void load();
    void suspend();
    void nodeChanged(const VNode*);
    void setTextMode();
    void setTableMode();

protected Q_SLOTS:
    void on_stacked__currentChanged(int idx);
    void anchorClicked(const QUrl& link);

private:
    enum PanelIndex {TablePanelIndex=0, TextPanelIndex=1};

    void loadTriggerGraphTab(bool forceLoad=false);
    void loadTriggerTab(bool forceLoad=false);
    void loadTriggeredTab(bool forceLoad=false);   
    bool isPanelLoaded(PanelIndex idx) const;
    int panelIndexToInt(PanelIndex idx) const;
    QString makeHtml(TriggerListCollector *tc,QString,QString) const;

    TriggerItemWidget* owner_;   
    std::set<PanelIndex> loadedPanels_;
    QPlainTextEdit* exprTe_;
    Highlighter* exprHighlight_;
    TriggerListCollector* triggerCollector_;
    TriggerTableCollector* tgCollector_;
    TriggerTableCollector* tgdCollector_;
};

#endif // TRIGGERBROWSER_HPP
