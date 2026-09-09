/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RichTextEdit.hpp"

#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QTextBlock>
#include <QWheelEvent>
#include <QtGlobal>

#include "UiLog.hpp"
#include "VConfig.hpp"
#include "ViewerUtil.hpp"

RichTextEdit::RichTextEdit(QWidget* parent)
    : QTextBrowser(parent) {
    setFont(ViewerUtil::findMonospaceFont());
}

RichTextEdit::~RichTextEdit() {
    if (fontProp_) {
        fontProp_->removeObserver(this);
    }
}

//---------------------------------------------
// Fontsize management
//---------------------------------------------

void RichTextEdit::setFontProperty(VProperty* p) {
    fontProp_ = p;
    fontProp_->addObserver(this);
    updateFont();
}

void RichTextEdit::slotZoomIn() {
    zoomIn();
    fontSizeChangedByZoom();
}

void RichTextEdit::slotZoomOut() {
    int oriSize = font().pointSize();
    zoomOut();

    if (font().pointSize() != oriSize) {
        fontSizeChangedByZoom();
    }
}

void RichTextEdit::fontSizeChangedByZoom() {
    if (fontProp_) {
        fontProp_->setValue(font());
    }
}

void RichTextEdit::updateFont() {
    if (fontProp_) {
        auto f = fontProp_->value().value<QFont>();
        if (font() != f) {
            setFont(f);
        }
    }
}

void RichTextEdit::notifyChange(VProperty* p) {
    if (fontProp_ == p) {
        setFont(p->value().value<QFont>());
    }
}
