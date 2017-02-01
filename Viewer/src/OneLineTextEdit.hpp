//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_ONELINETEXTEDIT_HPP_
#define VIEWER_SRC_ONELINETEXTEDIT_HPP_

#include <QTextEdit>

class OneLineTextEdit : public QTextEdit
{
Q_OBJECT

public:
	OneLineTextEdit(QWidget* parent=0);
	QSize sizeHint() const;

Q_SIGNALS:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent *e);
};



#endif /* VIEWER_SRC_ONELINETEXTEDIT_HPP_ */
