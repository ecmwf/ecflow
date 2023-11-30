/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_NodeQueryCombo_HPP
#define ecflow_viewer_NodeQueryCombo_HPP

#include <QComboBox>

class NodeQueryCombo : public QComboBox {
    Q_OBJECT

public:
    explicit NodeQueryCombo(QWidget* parent = nullptr);

protected Q_SLOTS:
    void slotCurrentChanged(int current);

Q_SIGNALS:
    void changed(QString);
};

#endif /* ecflow_viewer_NodeQueryCombo_HPP */
