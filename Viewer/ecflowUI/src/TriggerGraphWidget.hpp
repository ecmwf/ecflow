//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERGRAPHWIDGET_HPP
#define TRIGGERGRAPHWIDGET_HPP

#include "VInfo.hpp"
#include "Aspect.hpp"

#include <QWidget>

class TriggerTableItem;
class TriggerGraphModel;
class TriggerTableCollector;
class TriggerItemWidget;
class TriggerGraphScene;
class TriggerGraphDelegate;

class VComboSettings;

namespace Ui {
    class TriggerGraphWidget;
}

class TriggerGraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TriggerGraphWidget(QWidget *parent = nullptr);
    ~TriggerGraphWidget() override;

    void clear();
    void setInfo(VInfo_ptr info);
    void adjust(VInfo_ptr info, TriggerTableCollector* triggerTc1, TriggerTableCollector* tc2);
    void setTriggerCollector(TriggerTableCollector *tc1,TriggerTableCollector *tc2);
    void beginTriggerUpdate();
    void endTriggerUpdate();
    void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect);

Q_SIGNALS:
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);

private:
    Ui::TriggerGraphWidget* ui_;

private:
    TriggerGraphScene* scene_;
    TriggerGraphDelegate* delegate_;
    VInfo_ptr info_;
    TriggerTableCollector* nodeCollector_;
    VInfo_ptr lastSelectedItem_;
    QString depLabelText_;
    TriggerGraphModel* model_;
};

#endif // TRIGGERGRAPHWIDGET_HPP
