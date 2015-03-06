//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef LINEEDIT_INC_
#define LINEEDIT_INC_

#include <QLineEdit>

class QLabel;
class QToolButton;

class LineEdit : public QLineEdit
{
Q_OBJECT

public:
	LineEdit (QWidget *parent=0);
	void setDecoration(QPixmap);

public Q_SLOTS:
    void slotClear();

Q_SIGNALS:
    void textCleared();

protected:
	void adjustSize();
	void resizeEvent(QResizeEvent*);

	QToolButton *clearTb_;
	QLabel*	iconLabel_;
};


#endif
