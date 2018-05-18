//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "HtmlEdit.hpp"

#include <QDebug>
#include <QWheelEvent>

#include "UiLog.hpp"

HtmlEdit::HtmlEdit(QWidget * parent) :
    QTextBrowser(parent),
    fontProp_(0)
{
#if 0
    QFont f("Courier");
    //QFont f("Monospace");
    //f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    f.setPointSize(10);
    //f.setStyleStrategy(QFont::PreferAntialias);
    setFont(f);
#endif
}

HtmlEdit::~HtmlEdit()
{
    if(fontProp_)
        fontProp_->removeObserver(this);
}

//---------------------------------------------
// Fontsize management
//---------------------------------------------

void HtmlEdit::setFontProperty(VProperty* p)
{
    fontProp_=p;
    fontProp_->addObserver(this);
    updateFont();
}

void HtmlEdit::wheelEvent(QWheelEvent *event)
{
    int fps=font().pointSize();

    QTextBrowser::wheelEvent(event);
    if(font().pointSize() != fps)
        fontSizeChangedByZoom();
}

void HtmlEdit::fontSizeChangedByZoom()
{
    if(fontProp_)
        fontProp_->setValue(font());
}

void HtmlEdit::updateFont()
{
    if(fontProp_)
    {
        QFont f=fontProp_->value().value<QFont>();
        if(font() != f)
            setFont(f);
    }
}

void HtmlEdit::notifyChange(VProperty* p)
{
    if(fontProp_ ==p)
    {
        setFont(p->value().value<QFont>());
    }
}
