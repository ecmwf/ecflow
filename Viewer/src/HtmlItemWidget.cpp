//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "HtmlItemWidget.hpp"

#include <QDebug>
#include <QFontDatabase>

HtmlItemWidget::HtmlItemWidget(QWidget *parent) :
  QWidget(parent)
{
    setupUi(this);

    externalTb_->hide();

    fileLabel_->setProperty("fileInfo","1");

    searchLine_->setEditor(textEdit_);
    searchLine_->setVisible(false);

    textEdit_->setOpenExternalLinks(false);
    textEdit_->setOpenLinks(false);
    textEdit_->setReadOnly(true);
}

HtmlItemWidget::~HtmlItemWidget()
{
}

void HtmlItemWidget::removeSpacer()
{
    //Remove the first spacer item!!
    for(int i=0; horizontalLayout->count(); i++)
    {
        if(QSpacerItem* sp=horizontalLayout->itemAt(i)->spacerItem())
        {
            horizontalLayout->takeAt(i);
            delete sp;
            break;
        }
    }
}

void HtmlItemWidget::on_searchTb__clicked()
{
    searchLine_->setVisible(true);
    searchLine_->setFocus();
    searchLine_->selectAll();
}


void HtmlItemWidget::on_fontSizeUpTb__clicked()
{
    textEdit_->slotZoomIn();
}

void HtmlItemWidget::on_fontSizeDownTb__clicked()
{
    textEdit_->slotZoomOut();
}
