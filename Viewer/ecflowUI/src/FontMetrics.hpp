/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_FontMetrics_HPP
#define ecflow_viewer_FontMetrics_HPP

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

#endif /* ecflow_viewer_FontMetrics_HPP */
