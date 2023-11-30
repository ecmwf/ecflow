/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_OneLineTextEditor_HPP
#define ecflow_viewer_OneLineTextEditor_HPP

#include <QTextEdit>

class OneLineTextEdit : public QTextEdit {
    Q_OBJECT

public:
    OneLineTextEdit(QWidget* parent = nullptr);
    QSize sizeHint() const override;

Q_SIGNALS:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* e) override;
};

#endif /* ecflow_viewer_OneLineTextEditor_HPP */
