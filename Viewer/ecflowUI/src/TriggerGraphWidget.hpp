/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TriggerGraphWidget_HPP
#define ecflow_viewer_TriggerGraphWidget_HPP

#include <QWidget>

#include "TriggerCollector.hpp"
#include "VInfo.hpp"
#include "ecflow/node/Aspect.hpp"

class TriggerTableItem;
class TriggerTableCollector;
class TriggerItemWidget;
class VComboSettings;
class QSlider;
class QToolButton;

namespace Ui {
class TriggerGraphWidget;
}

class TriggerGraphWidget;

class TriggerGraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit TriggerGraphWidget(QWidget* parent = nullptr);
    ~TriggerGraphWidget() override;

    void clear(bool keepConfig = false);
    void setInfo(VInfo_ptr info, bool dependency);
    VInfo_ptr info() const { return info_; }
    bool dependency() const;
    void adjust(VInfo_ptr info, bool dependency, TriggerTableCollector* triggerTc1, TriggerTableCollector* tc2);
    void setTriggerCollector(TriggerTableCollector* tc1, TriggerTableCollector* tc2);
    void beginTriggerUpdate();
    void endTriggerUpdate();
    void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect);
    void setTriggeredScanner(TriggeredScanner*);
    void setZoomSlider(QSlider*);
    void becameInactive();
    void rerender();
    void readSettings(VComboSettings* vs);
    void writeSettings(VComboSettings* vs);

Q_SIGNALS:
    void infoPanelCommand(VInfo_ptr, QString);
    void dashboardCommand(VInfo_ptr, QString);
    void linkSelected(VInfo_ptr);

protected Q_SLOTS:
    void setZoomLevel(int);
    void resetZoomLevel();
    void updateLegend();

private:
    Ui::TriggerGraphWidget* ui_;

private:
    void scan(bool dependency);
    void adjustButtons();

    VInfo_ptr info_;
    TriggerTableCollector* triggerTc_{nullptr};
    TriggerTableCollector* triggeredTc_{nullptr};
    VInfo_ptr lastSelectedItem_;
    QString depLabelText_;
    QSlider* zoomSlider_{nullptr};
};

#endif /* ecflow_viewer_TriggerGraphWidget_HPP */
