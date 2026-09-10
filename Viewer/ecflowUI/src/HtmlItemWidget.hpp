/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_HtmlItemWidget_HPP
#define ecflow_viewer_HtmlItemWidget_HPP

#include <QWidget>

#include "ui_HtmlItemWidget.h"

class HtmlItemWidget : public QWidget, protected Ui::HtmlItemWidget {
    Q_OBJECT

public:
    explicit HtmlItemWidget(QWidget* parent = nullptr);
    ~HtmlItemWidget() override;

protected Q_SLOTS:
    void on_searchTb__clicked();
    void on_fontSizeUpTb__clicked();
    void on_fontSizeDownTb__clicked();

Q_SIGNALS:
    void editorFontSizeChanged();

protected:
    void removeSpacer();
};

#endif /* ecflow_viewer_HtmlItemWidget_HPP */
