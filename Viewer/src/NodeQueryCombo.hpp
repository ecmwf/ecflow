//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_NODEQUERYCOMBO_HPP_
#define VIEWER_SRC_NODEQUERYCOMBO_HPP_

#include <QComboBox>

class NodeQueryCombo : public QComboBox
{
Q_OBJECT

public:
	explicit NodeQueryCombo(QWidget* parent=0);

protected Q_SLOTS:
	void slotCurrentChanged(int current);

Q_SIGNALS:
	void changed(QString);
};

#endif /* VIEWER_SRC_NODEQUERYCOMBO_HPP_ */
