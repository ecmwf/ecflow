//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "MessageItemWidget.hpp"

#include <QtGlobal>
#include <QAction>
#include <QClipboard>

#include "InfoProvider.hpp"
#include "VReply.hpp"

#include "LogModel.hpp"

static bool firstRun=true;

MessageItemWidget::MessageItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    searchTb_->setEnabled(false);
    searchTb_->setVisible(false);
    searchLine_->setVisible(false);

    infoProvider_=new MessageProvider(this);

    model_=new LogModel(this);

    treeView_->setProperty("log","1");
    treeView_->setModel(model_);
    treeView_->setItemDelegate(new LogDelegate(this));
    treeView_->setContextMenuPolicy(Qt::ActionsContextMenu);

    //make the horizontal scrollbar work
    treeView_->header()->setStretchLastSection(false);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    treeView_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif

    syncTb_->hide();

    //Define context menu
    treeView_->addAction(actionCopyEntry_);
    treeView_->addAction(actionCopyRow_);
}

QWidget* MessageItemWidget::realWidget()
{
    return this;
}

void MessageItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();
    info_=info;

    if(info_)
    {
        infoProvider_->info(info_);
    }
}

void MessageItemWidget::clearContents()
{
    InfoPanelItem::clear();
    model_->clearData();
}

void MessageItemWidget::infoReady(VReply* reply)
{
    model_->setData(reply->textVec());

    //Adjust column size if it is the first run
    if(firstRun && model_->hasData())
    {
    	firstRun=false;
    	for(int i=0; i < model_->columnCount()-1; i++)
    	{
            treeView_->resizeColumnToContents(i);
    	}
    }
}

void MessageItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
}

void MessageItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
}

void MessageItemWidget::on_actionCopyEntry__triggered()
{
   toClipboard(model_->entryText(treeView_->currentIndex()));
}

void MessageItemWidget::on_actionCopyRow__triggered()
{
   toClipboard(model_->fullText(treeView_->currentIndex()));
}

void MessageItemWidget::toClipboard(QString txt) const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QClipboard* cb=QGuiApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#else
    QClipboard* cb=QApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#endif
}

static InfoPanelItemMaker<MessageItemWidget> maker1("message");
