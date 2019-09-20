//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TIMELINEITEMWIDGET_HPP
#define TIMELINEITEMWIDGET_HPP

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

#include <QWidget>

class VNode;
class TimelineWidget;

class TimelineItemWidget : public QWidget, public InfoPanelItem
{
public:
    explicit TimelineItemWidget(QWidget *parent=nullptr);
    ~TimelineItemWidget() override;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;
    bool hasSameContents(VInfo_ptr info) override;
    void notifyInfoChanged(const std::string& path) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

    void writeSettings(VComboSettings* vs) override;
    void readSettings(VComboSettings* vs) override;

protected:
    void updateState(const ChangeFlags&) override;
    void serverSyncFinished() override;
    void connectStateChanged() override;
private:
    void load();

    TimelineWidget* w_;
    bool delayedLoad_;
};

#endif // TIMELINEITEMWIDGET_HPP

