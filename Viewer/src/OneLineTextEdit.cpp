//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OneLineTextEdit.hpp"

#include "Highlighter.hpp"

#include <QApplication>
#include <QStyle>
#include <QStyleOptionFrameV3>

OneLineTextEdit::OneLineTextEdit(QWidget* parent) :  QPlainTextEdit(parent)
{
	setReadOnly(true);
	setWordWrapMode(QTextOption::NoWrap);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);

	QSize s=sizeHint();
	setFixedHeight(s.height());

	Highlighter* ih=new Highlighter(document(),"query");
}

QSize OneLineTextEdit::sizeHint() const
{
    QFontMetrics fm(font());
    QStyleOptionFrameV3 opt;
    QString text = document()->toPlainText();

    int h = qMax(fm.height(), 14) + 4;
    int w = fm.width(text) + 4;

    opt.initFrom(this);

    return style()->sizeFromContents(
        QStyle::CT_LineEdit,
        &opt,
        QSize(w, h).expandedTo(QApplication::globalStrut()),
        this
    );
}


