/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_PlainTextWidget_HPP
#define ecflow_viewer_PlainTextWidget_HPP

#include <QWidget>

namespace Ui {
class PlainTextWidget;
}

class PlainTextWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlainTextWidget(QWidget* parent = nullptr);
    ~PlainTextWidget() override;

    void setPlainText(QString);
    void setTitle(QString);
    void setShowTitleLabel(bool);

protected Q_SLOTS:
    void slotSearch();
    void slotGotoLine();
    void slotFontSizeUp();
    void slotFontSizeDown();

private:
    Ui::PlainTextWidget* ui_;
};

#endif /* ecflow_viewer_PlainTextWidget_HPP */
