/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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

RichTextEdit::RichTextEdit(QWidget* parent) : QTextBrowser(parent) {
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
