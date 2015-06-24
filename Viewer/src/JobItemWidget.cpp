//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "JobItemWidget.hpp"

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "VReply.hpp"
#include "VNode.hpp"

JobItemWidget::JobItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
    QFont f;
    f.setFamily("Monospace");
    //f.setFamily("Courier");
    f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    textEdit_->setFont(f);

    Highlighter* ih=new Highlighter(textEdit_->document(),"script");

    infoProvider_=new JobProvider(this);
}

QWidget* JobItemWidget::realWidget()
{
    return this;
}

void JobItemWidget::reload(VInfo_ptr info)
{
    loaded_=true;
    info_=info;

    //Info must be a node
    if(!info_.get() || !info_->isNode() || !info_->node())
    {
        textEdit_->clear();
    }
    else
    {
        clearContents();
        fileLabel_->setText(tr("File: ") + QString::fromStdString(info_->node()->genVariable("ECF_JOB")));
        infoProvider_->info(info_);
    }   
}

void JobItemWidget::clearContents()
{
    loaded_=false;
    textEdit_->clear();
}

void JobItemWidget::infoReady(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void JobItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void JobItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    textEdit_->setPlainText(s);
}

static InfoPanelItemMaker<JobItemWidget> maker1("job");
