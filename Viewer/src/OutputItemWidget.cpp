//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputItemWidget.hpp"

#include "Highlighter.hpp"
#include "OutputProvider.hpp"
#include "OutputModel.hpp"
#include "VReply.hpp"

#include <QDebug>
#include <QFile>
#include <QItemSelectionModel>

OutputItemWidget::OutputItemWidget(QWidget *parent) :
	QWidget(parent)
{
	setupUi(this);

	messageLabel_->hide();
	dirLabel_->hide();
	searchLine_->hide();

	QFont f;
	f.setFamily("Monospace");
	//f.setFamily("Courier");
	f.setStyleHint(QFont::TypeWriter);
	f.setFixedPitch(true);
	textEdit_->setFont(f);

	Highlighter* ih=new Highlighter(textEdit_->document(),"output");

	infoProvider_=new OutputProvider(this);

	//The view
	outputView_->setRootIsDecorated(false);
	outputView_->setAllColumnsShowFocus(true);
	outputView_->setUniformRowHeights(true);
	outputView_->setAlternatingRowColors(true);
	outputView_->setSortingEnabled(true);

	//The models
	dirModel_=new OutputModel(this);
	dirSortModel_=new OutputSortModel(this);
	dirSortModel_->setSourceModel(dirModel_);
	dirSortModel_->setDynamicSortFilter(true);

	outputView_->setModel(dirSortModel_);

	//When the selection changes in the view
	connect(outputView_->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotOutputSelected(QModelIndex,QModelIndex)));

	//Splitter
	int wHeight=size().height();
	if(wHeight > 100)
	{
		QList<int> sizes;
		sizes << wHeight-80 << 80;
		splitter_->setSizes(sizes);
	}
}

QWidget* OutputItemWidget::realWidget()
{
	return this;
}

void OutputItemWidget::reload(VInfo_ptr info)
{
	clearContents();

	loaded_=true;
	info_=info;

    if(info_ && info_.get())
	{
	    //Get file contents
	    infoProvider_->info(info_);

	    //Get directory contents
	    OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);
	    updateDir(op->directory());

	}
}
void OutputItemWidget::updateDir(VDir_ptr dir)
{
	bool status=(dir && dir.get());

	if(status)
	{
		dirModel_->setData(dir);
		//dirLabel_->update(dir);
		dirWidget_->show();
	}
	else
	{
		dirWidget_->hide();
		//dirLabel_->clear();
		dirModel_->clearData();
	}
}

void OutputItemWidget::enableDir(bool status)
{
	if(status)
	{
		dirWidget_->show();
	}
	else
	{
		dirWidget_->hide();
		//dirLabel_->clear();
		dirModel_->clearData();
	}
}

void OutputItemWidget::clearContents()
{
	//loaded_=false;

	InfoPanelItem::clear();

	fileLabel_->clear();
	textEdit_->clear();

	enableDir(false);
}

void OutputItemWidget::infoReady(VReply* reply)
{
    VFile_ptr f=reply->tmpFile();

    //If the info is stored in a tmp file
    if(f && f.get())
    {
    	QFile file(QString::fromStdString(f->path()));
    	file.open(QIODevice::ReadOnly);
    	textEdit_->setPlainText(file.readAll());
    }
    //If the info is stored as a string in the reply object
    else
    {
    	QString s=QString::fromStdString(reply->text());
    	textEdit_->setPlainText(s);
    }

    //Update the file label
    fileLabel_->update(reply);
}

void OutputItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void OutputItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    textEdit_->setPlainText(s);

    //Update the file label
    fileLabel_->update(reply);

}

void OutputItemWidget::on_searchTb__toggled(bool b)
{
	searchLine_->setVisible(b);
	if(b)
	{
		searchLine_->setFocus();
	}
}

//This slot is called when a file item is selected in the output view.
void OutputItemWidget::slotOutputSelected(QModelIndex,QModelIndex)
{
	QModelIndex current=dirSortModel_->mapToSource(outputView_->currentIndex());
	std::string fullName=dirModel_->fullName(current);

	//Get the file via the provider. This will call infoReady() etc. in the end.
	OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);
	op->file(fullName);
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");
