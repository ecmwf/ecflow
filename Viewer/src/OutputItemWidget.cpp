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

OutputItemWidget::OutputItemWidget(QWidget *parent) :
	QWidget(parent)
{
	setupUi(this);

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
	model_=new OutputModel(this);
	sortModel_=new OutputSortModel(this);
	sortModel_->setSourceModel(model_);
	sortModel_->setDynamicSortFilter(true);

	outputView_->setModel(sortModel_);
}

QWidget* OutputItemWidget::realWidget()
{
	return this;
}

void OutputItemWidget::reload(VInfo_ptr info)
{
	loaded_=true;
	info_=info;

	if(!info.get())
	{
		fileLabel_->clear();
		textEdit_->clear();

		dirLabel_->clear();
		model_->clearData();

		dirLabel_->hide();
		outputView_->hide();
	}
    else
	{
	    clearContents();

	    //Get file contents
	    infoProvider_->info(info_);
	    OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);

	    //Get directory contents
	    VDir_ptr dir=op->directory();

	    if(dir)
	    {
	    	model_->setData(dir);
	    	dirLabel_->show();
	    	outputView_->show();
	    }
	    else
	    {
	    	model_->clearData();
	    	dirLabel_->hide();
	    	outputView_->hide();
	    }
	}
}

void OutputItemWidget::clearContents()
{
	loaded_=false;

	fileLabel_->clear();
	textEdit_->clear();

	dirLabel_->clear();
	model_->clearData();

	dirLabel_->hide();
	outputView_->hide();
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
    //The info is stores as a string in the reply object
    else
    {
    	QString s=QString::fromStdString(reply->text());
    	textEdit_->setPlainText(s);
    }

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

    fileLabel_->update(reply);

    if(reply->fileName().empty())
    {
    	dirLabel_->clear();
    	model_->clearData();

    	dirLabel_->hide();
    	outputView_->hide();
    }
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");
