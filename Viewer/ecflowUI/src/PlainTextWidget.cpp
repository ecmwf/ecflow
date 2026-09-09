/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PlainTextWidget.hpp"

#include "ui_PlainTextWidget.h"

PlainTextWidget::PlainTextWidget(QWidget* /*parent*/)
    : ui_(new Ui::PlainTextWidget) {
    ui_->setupUi(this);

    ui_->titleLabel->setProperty("fileInfo", "1");

    ui_->textEdit->setReadOnly(true);
    ui_->textEdit->setWordWrapMode(QTextOption::NoWrap);
    ui_->textEdit->setShowLineNumbers(true);

    ui_->searchLine->setEditor(ui_->textEdit);
    ui_->searchLine->setVisible(false);

    connect(ui_->searchTb, SIGNAL(clicked()), this, SLOT(slotSearch()));

    connect(ui_->gotoLineTb, SIGNAL(clicked()), this, SLOT(slotGotoLine()));

    connect(ui_->fontSizeUpTb, SIGNAL(clicked()), this, SLOT(slotFontSizeUp()));

    connect(ui_->fontSizeDownTb, SIGNAL(clicked()), this, SLOT(slotFontSizeDown()));
}

PlainTextWidget::~PlainTextWidget() {
    delete ui_;
}

void PlainTextWidget::setPlainText(QString t) {
    ui_->textEdit->setPlainText(t);
}

void PlainTextWidget::setTitle(QString t) {
    ui_->titleLabel->setText(t);
}

void PlainTextWidget::setShowTitleLabel(bool b) {
    ui_->titleLabel->setVisible(b);
}

void PlainTextWidget::slotSearch() {
    ui_->searchLine->setVisible(true);
    ui_->searchLine->setFocus();
    ui_->searchLine->selectAll();
}

void PlainTextWidget::slotGotoLine() {
    ui_->textEdit->gotoLine();
}

void PlainTextWidget::slotFontSizeUp() {
    // We need to call a custom slot here instead of "zoomIn"!!!
    ui_->textEdit->slotZoomIn();
}

void PlainTextWidget::slotFontSizeDown() {
    // We need to call a custom slot here instead of "zoomOut"!!!
    ui_->textEdit->slotZoomOut();
}
