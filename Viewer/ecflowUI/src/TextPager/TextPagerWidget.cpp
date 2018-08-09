//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <QWidget>
#include <QString>
#include <QFont>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QEvent>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>

#include "TextPagerWidget.hpp"

#include "GotoLineDialog.hpp"

bool add = false;

TextPagerWidget::TextPagerWidget(QWidget *parent)
   // : TextPagerEdit(parent),
{
	auto* hb=new QHBoxLayout(this);
	hb->setContentsMargins(0,0,0,0);
	hb->setSpacing(0);

	textEditor_=new TextPagerEdit(this);
	lineNumArea_ = new TextPagerLineNumberArea(textEditor_);

	hb->addWidget(lineNumArea_);
	hb->addWidget(textEditor_,1);

	setAttribute(Qt::WA_MouseTracking);
}

void TextPagerWidget::clear()
{
	textEditor_->document()->clear();
}

bool TextPagerWidget::load(const QString &fileName, TextPagerDocument::DeviceMode mode)
{
	return textEditor_->load(fileName, mode, nullptr);
}

void TextPagerWidget::setText(const QString &txt)
{
	textEditor_->setText(txt);
}

void TextPagerWidget::setFontProperty(VProperty* p)
{
	textEditor_->setFontProperty(p);
}

void TextPagerWidget::zoomIn()
{
	textEditor_->zoomIn();
}

void TextPagerWidget::zoomOut()
{
	textEditor_->zoomOut();
}
// ---------------------------------------------------------------------------
// TextEdit::gotoLine
// triggered when the user asks to bring up the 'go to line' dialog
// ---------------------------------------------------------------------------

void TextPagerWidget::gotoLine()
{
    // create the dialog if it does not already exist

    if (!gotoLineDialog_)
    {
        gotoLineDialog_ = new GotoLineDialog(this);

        connect(gotoLineDialog_, SIGNAL(gotoLine(int)), this, SLOT(gotoLine(int)));
    }

    // if created, set it up and display it

    if (gotoLineDialog_)
    {
        gotoLineDialog_->show();
        gotoLineDialog_->raise();
        gotoLineDialog_->activateWindow();
        gotoLineDialog_->setupUIBeforeShow();
    }
}

void TextPagerWidget::gotoLine(int line)
{
    textEditor_->gotoLine(line);
}    
