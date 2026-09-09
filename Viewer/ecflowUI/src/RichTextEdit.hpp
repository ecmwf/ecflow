/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_RichTextEdit_HPP
#define ecflow_viewer_RichTextEdit_HPP

#include <QTextBrowser>

#include "VProperty.hpp"

class RichTextEdit : public QTextBrowser, public VPropertyObserver {
    Q_OBJECT

public:
    explicit RichTextEdit(QWidget* parent = nullptr);
    ~RichTextEdit() override;

    void setFontProperty(VProperty* p);
    void updateFont();
    void notifyChange(VProperty* p) override;

public Q_SLOTS:
    void slotZoomIn();
    void slotZoomOut();

Q_SIGNALS:
    void fontSizeChangedByWheel();

private:
    void fontSizeChangedByZoom();

    VProperty* fontProp_{nullptr};
};

#endif /* ecflow_viewer_RichTextEdit_HPP */
