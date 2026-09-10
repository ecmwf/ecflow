/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TimelineInfoDelegate_HPP
#define ecflow_viewer_TimelineInfoDelegate_HPP

#include <string>

#include <QBrush>
#include <QDateTime>
#include <QPen>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

#include "NodeViewDelegate.hpp"
#include "VProperty.hpp"

class TimelineInfoDailyModel;

class TimelineInfoDelegate : public NodeViewDelegate {
public:
    explicit TimelineInfoDelegate(QWidget* parent = nullptr);
    ~TimelineInfoDelegate() override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
    void updateSettings() override { updateSettingsInternal(); }

private:
    void updateSettingsInternal();
    QPen borderPen_;
};

#endif /* ecflow_viewer_TimelineInfoDelegate_HPP */
