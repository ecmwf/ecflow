/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_CodeItemWidget_HPP
#define ecflow_viewer_CodeItemWidget_HPP

#include <QWidget>

#include "ui_CodeItemWidget.h"

class CodeItemWidget : public QWidget, protected Ui::CodeItemWidget {
    Q_OBJECT

public:
    explicit CodeItemWidget(QWidget* parent = nullptr);
    ~CodeItemWidget() override;

protected Q_SLOTS:
    void on_searchTb__clicked();
    void on_gotoLineTb__clicked();
    void on_fontSizeUpTb__clicked();
    void on_fontSizeDownTb__clicked();
    void on_reloadTb__clicked();
    void on_copyPathTb__clicked();
    void on_commandTb__clicked();

Q_SIGNALS:
    void editorFontSizeChanged();

protected:
    void removeSpacer();
    virtual void reloadRequested() = 0;
    virtual void commandRequested() {}
    void setCurrentFileName(const std::string&);
    void clearCurrentFileName();

private:
    std::string currentFileName_;
};

#endif /* ecflow_viewer_CodeItemWidget_HPP */
