// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QFontMetrics>

class FontMetrics : public QFontMetrics {
public:
    explicit FontMetrics(const QFont& font);
    int realHeight() const { return realHeight_; }
    int topPaddingForCentre() const { return topPadding_; }
    int bottomPaddingForCentre() const { return bottomPadding_; }

protected:
    int realHeight_;
    int topPadding_;
    int bottomPadding_;

private:
    void computeRealHeight(QFont f);
};
