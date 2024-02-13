/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_CustomTabWidget_HPP
#define ecflow_viewer_CustomTabWidget_HPP

#include <QTabWidget>

class CustomTabWidget : public QTabWidget {
public:
    explicit CustomTabWidget(QWidget* parent = nullptr);

    void setCustomIcon(int index, QPixmap pix);

protected:
    QSize maxIconSize() const;
};

#endif /* ecflow_viewer_CustomTabWidget_HPP */
