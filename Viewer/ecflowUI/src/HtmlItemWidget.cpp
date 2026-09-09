/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HtmlItemWidget.hpp"

#include <QDebug>
#include <QFontDatabase>

HtmlItemWidget::HtmlItemWidget(QWidget* parent)
    : QWidget(parent) {
    setupUi(this);

    externalTb_->hide();

    fileLabel_->setProperty("fileInfo", "1");

    searchLine_->setEditor(textEdit_);
    searchLine_->setVisible(false);

    textEdit_->setOpenExternalLinks(false);
    textEdit_->setOpenLinks(false);
    textEdit_->setReadOnly(true);
}

HtmlItemWidget::~HtmlItemWidget() = default;

void HtmlItemWidget::removeSpacer() {
    // Remove the first spacer item!!
    for (int i = 0; horizontalLayout->count(); i++) {
        if (QSpacerItem* sp = horizontalLayout->itemAt(i)->spacerItem()) {
            horizontalLayout->takeAt(i);
            delete sp;
            break;
        }
    }
}

void HtmlItemWidget::on_searchTb__clicked() {
    searchLine_->setVisible(true);
    searchLine_->setFocus();
    searchLine_->selectAll();
}

void HtmlItemWidget::on_fontSizeUpTb__clicked() {
    textEdit_->slotZoomIn();
}

void HtmlItemWidget::on_fontSizeDownTb__clicked() {
    textEdit_->slotZoomOut();
}
