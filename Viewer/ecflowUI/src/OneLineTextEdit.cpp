/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "OneLineTextEdit.hpp"

#include <QApplication>
#include <QDebug>
#include <QStyle>

#include "Highlighter.hpp"
// #include <QStyleOptionFrameV3>

OneLineTextEdit::OneLineTextEdit(QWidget* parent)
    : QTextEdit(parent) {
    setReadOnly(true);
    setWordWrapMode(QTextOption::NoWrap);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    document()->setDocumentMargin(2);

    QFont f;
    QFontMetrics fm(f);

    int h = fm.height() + fm.leading() + 1 + 6;

    setFixedHeight(h);

    // The document becomes the owner of the highlighter
    new Highlighter(document(), "query");
}

QSize OneLineTextEdit::sizeHint() const {
    return QTextEdit::sizeHint();
    /*
    QFontMetrics fm(font());
QStyleOptionFrameV3 opt;
QString text = document()->toHtml();

int h = qMax(fm.height(), 14) + 4;
int w = fm.width(text) + 4;

opt.initFrom(this);

return style()->sizeFromContents(
    QStyle::CT_LineEdit,
    &opt,
    QSize(w, h).expandedTo(QApplication::globalStrut()),
    this
);*/
}

void OneLineTextEdit::mousePressEvent(QMouseEvent* /*e*/) {
    Q_EMIT clicked();
}
