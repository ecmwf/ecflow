// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QTextBrowser>

#include "VProperty.hpp"

class HtmlEdit : public QTextBrowser, public VPropertyObserver {
public:
    explicit HtmlEdit(QWidget* parent = nullptr);
    ~HtmlEdit() override;

    void setFontProperty(VProperty* p);
    void updateFont();
    void notifyChange(VProperty* p) override;

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    void fontSizeChangedByZoom();

    VProperty* fontProp_{nullptr};
};
