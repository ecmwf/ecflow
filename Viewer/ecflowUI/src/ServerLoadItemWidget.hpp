//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLOADITEMWIDGET_HPP
#define SERVERLOADITEMWIDGET_HPP

#include <QPlainTextEdit>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

class VNode;
class LogLoadWidget;
class MessageLabel;

class ServerLoadItemWidget : public QWidget, public InfoPanelItem
{
public:
    explicit ServerLoadItemWidget(QWidget *parent=nullptr);
    ~ServerLoadItemWidget() override;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;
    bool hasSameContents(VInfo_ptr info) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

    void readSettings(VComboSettings* vs) override;
    void writeSettings(VComboSettings* vs) override;

protected:
    void updateState(const ChangeFlags&) override;
    void serverSyncFinished() override;
    void connectStateChanged() override;

private:
    void load();
#ifdef ECFLOW_LOGVIEW
    LogLoadWidget* w_;
    bool delayedLoad_{false};
#else
    MessageLabel* w_;
#endif
};

#endif // SERVERLOADITEMWIDGET_HPP


