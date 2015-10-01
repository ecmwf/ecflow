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
#include <QTimer>

int OutputItemWidget::updateDirTimeout_=1000*60;

OutputItemWidget::OutputItemWidget(QWidget *parent) :
	QWidget(parent),
	userClickedReload_(false),
	ignoreOutputSelection_(false),
	jobHighlighter_(0)
{
	setupUi(this);

	messageLabel_->hide();
	dirLabel_->hide();
	searchLine_->hide();

	fileLabel_->setProperty("fileInfo","1");

	//This highlighter only works for jobs
	jobHighlighter_=new Highlighter(textEdit_->document(),"job");
	jobHighlighter_->setDocument(NULL);

	infoProvider_=new OutputProvider(this);

	//The view
	outputView_->setRootIsDecorated(false);
	outputView_->setAllColumnsShowFocus(true);
	outputView_->setUniformRowHeights(true);
	outputView_->setAlternatingRowColors(true);
	outputView_->setSortingEnabled(true);
	outputView_->sortByColumn(3, Qt::DescendingOrder);  // sort with latest files first (0-based)

	//The models
	dirModel_=new OutputModel(this);
	dirSortModel_=new OutputSortModel(this);
	dirSortModel_->setSourceModel(dirModel_);
	dirSortModel_->setDynamicSortFilter(true);

	outputView_->setModel(dirSortModel_);

	//When the selection changes in the view
	connect(outputView_->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotOutputSelected(QModelIndex,QModelIndex)));

	//Connect the searchline to the editor
	searchLine_->setEditor(textEdit_);

	//Set splitter's initial size.
	int wHeight=size().height();
	if(wHeight > 100)
	{
		QList<int> sizes;
		sizes << wHeight-80 << 80;
		splitter_->setSizes(sizes);
	}

	//Dir contents update timer
	updateDirTimer_=new QTimer(this);
	updateDirTimer_->setInterval(updateDirTimeout_);

	connect(updateDirTimer_,SIGNAL(timeout()),
			this,SLOT(slotUpdateDir()));
}

OutputItemWidget::~OutputItemWidget()
{
	if(jobHighlighter_ && !jobHighlighter_->parent())
	{
		delete jobHighlighter_;
	}
}

QWidget* OutputItemWidget::realWidget()
{
	return this;
}

void OutputItemWidget::reload(VInfo_ptr info)
{
	clearContents();

	enabled_=true;
	info_=info;

	if(info_ && info_.get())
	{
	    //Get file contents
	    infoProvider_->info(info_);

	    //Start contents update timer
	    updateDirTimer_->start();
	}
}

std::string OutputItemWidget::currentFullName() const
{
	QModelIndex current=dirSortModel_->mapToSource(outputView_->currentIndex());

	std::string fullName;
	OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);

	if(current.isValid())
	{
		fullName=dirModel_->fullName(current);
	}
	else
	{
		fullName=op->joboutFileName();
	}

	return fullName;
}

void OutputItemWidget::getLatestFile()
{
	messageLabel_->hide();
	fileLabel_->clear();
	textEdit_->clear();

	//Get the latest file contents
	infoProvider_->info(info_);
}

void OutputItemWidget::getCurrentFile()
{
	messageLabel_->hide();
	fileLabel_->clear();
	textEdit_->clear();

	if(info_ && info_.get())
	{
		std::string fullName=currentFullName();
		OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);
		op->file(fullName);
	}
}

void OutputItemWidget::updateDir(bool restartTimer)
{
	//Remember the selection
	std::string fullName=currentFullName();
	updateDir(restartTimer,fullName);
}

void OutputItemWidget::updateDir(bool restartTimer,const std::string& selectFullName)
{
	if(restartTimer)
		updateDirTimer_->stop();

	OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);
	VDir_ptr dir=op->directory();

	bool status=(dir && dir.get());

	if(status)
	{
		outputView_->selectionModel()->clearSelection();
		dirModel_->setData(dir);
		dirWidget_->show();

		//Try to preserve the selection
		ignoreOutputSelection_=true;
		outputView_->setCurrentIndex(dirSortModel_->fullNameToIndex(selectFullName));
		ignoreOutputSelection_=false;
	}
	else
	{
		dirWidget_->hide();
		dirModel_->clearData();
	}

	if(restartTimer)
		updateDirTimer_->start(updateDirTimeout_);
}

void OutputItemWidget::slotUpdateDir()
{
	updateDir(false);
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
		dirModel_->clearData();
	}
}

void OutputItemWidget::clearContents()
{
	updateDirTimer_->stop();

	InfoPanelItem::clear();

	messageLabel_->hide();
	fileLabel_->clear();
	textEdit_->clear();

	enableDir(false);
}

void OutputItemWidget::infoReady(VReply* reply)
{
	if(reply->hasWarning())
	{
		messageLabel_->showWarning(QString::fromStdString(reply->warningText()));
	}
	else if(reply->hasInfo())
	{
	    messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
	}

	//For job files we set the proper highlighter
	if(reply->fileName().find(".job") != std::string::npos)
	{
		if(!jobHighlighter_)
		{
			jobHighlighter_=new Highlighter(textEdit_->document(),"job");
		}
		else if(jobHighlighter_->document() != textEdit_->document())
		{
			jobHighlighter_->setDocument(textEdit_->document());
		}
	}
	else if(jobHighlighter_)
	{

		jobHighlighter_->setDocument(NULL);
	}

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

    searchOnReload();

    //Update the file label
    fileLabel_->update(reply);

    //Update the dir widget and select the proper file in the list
    updateDir(true,reply->fileName());
}

void OutputItemWidget::infoProgress(VReply* reply)
{
	messageLabel_->showInfo(QString::fromStdString(reply->infoText()));

    //updateDir(true);
}

void OutputItemWidget::infoFailed(VReply* reply)
{
	QString s=QString::fromStdString(reply->errorText());
	messageLabel_->showError(s);

    //Update the file label
    fileLabel_->update(reply);

    updateDir(true);
}

void OutputItemWidget::on_searchTb__toggled(bool b)
{
	searchLine_->setVisible(b);
	if(b)
	{
		searchLine_->setFocus();
	}
}

void OutputItemWidget::on_reloadTb__clicked()
{
	userClickedReload_ = true;
	getLatestFile();
	userClickedReload_ = false;
}

// search for a highlight any of the pre-defined keywords so that
// the (probably) most important piece of information is highlighted
bool OutputItemWidget::automaticSearchForKeywords()
{
	QStringList keywords;
	keywords << "--abort" << "--complete" << "xabort" << "xcomplete"
	         << "System Billing Units";
	bool found = false;
	int i = 0;
	QTextDocument::FindFlags findFlags = QTextDocument::FindBackward;

	// find any of the keywords and stop at the first one
	while (!found && i < keywords.size())
	{
		found = textEdit_->findString(keywords.at(i), findFlags);
		i++;
	}

	return found;
}

// Called when we load a new node's information into the panel, or
// when we move to the panel from another one.
// If the search box is open, then search for the first matching item;
// otherwise, search for a pre-configured list of keywords. If none
// are found, and the user has clicked on the 'reload' button then
// we just go to the last line of the output
void OutputItemWidget::searchOnReload()
{
	if (searchLine_->isVisible() && !searchLine_->isEmpty())
	{
		searchLine_->slotFindNext();
	}
	else
	{
		if (!automaticSearchForKeywords())
		{
			if (userClickedReload_)
			{
				// move the cursor to the start of the last line
				QTextCursor cursor = textEdit_->textCursor();
				cursor.movePosition(QTextCursor::End);
				cursor.movePosition(QTextCursor::StartOfLine);
				textEdit_->setTextCursor(cursor);   
			}
		}
	}
}

//This slot is called when a file item is selected in the output view.
void OutputItemWidget::slotOutputSelected(QModelIndex idx1,QModelIndex idx2)
{
	if(!ignoreOutputSelection_)
		getCurrentFile();
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");
