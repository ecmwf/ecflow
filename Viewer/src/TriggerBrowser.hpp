//============================================================================
// Copyright 2016 ECMWF.
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

class TriggerItemWidget;

class TriggerBrowser : public QWidget, protected Ui::TriggerBrowser
{
Q_OBJECT

public:
    explicit TriggerBrowser(QWidget *parent=0);

    void setOwner(TriggerItemWidget*);
    void clear();
    void load();

protected Q_SLOTS:
    void on_tab__currentChanged(int idx);
    void anchorClicked(const QUrl& link);

private:
    enum TabIndex {TriggerTabIndex=0, TriggeredTabIndex=1};

    void loadTriggerTab(bool forceLoad=false);
    void loadTriggeredTab(bool forceLoad=false);
    bool isTabLoaded(TabIndex idx) const;
    int tabIndexToInt(TabIndex idx) const;

    TriggerItemWidget* owner_;
    std::set<TabIndex> loadedTabs_;
};

#endif // TRIGGERBROWSER_HPP
