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
#include "OutputDirProvider.hpp"
#include "OutputModel.hpp"
#include "VConfig.hpp"
#include "VReply.hpp"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMovie>
#include <QTime>
#include <QTimer>
#include "OutputFileProvider.hpp"

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

	//--------------------------------
	// The file contents
	//--------------------------------

	//This highlighter only works for jobs
	jobHighlighter_=new Highlighter(textEdit_->document(),"job");
	jobHighlighter_->setDocument(NULL);

	infoProvider_=new OutputFileProvider(this);

	//--------------------------------
	// The dir contents
	//--------------------------------

	dirProvider_=new OutputDirProvider(this);

	//The view
	dirView_->setRootIsDecorated(false);
	dirView_->setAllColumnsShowFocus(true);
	dirView_->setUniformRowHeights(true);
	dirView_->setAlternatingRowColors(true);
	dirView_->setSortingEnabled(true);
	dirView_->sortByColumn(3, Qt::DescendingOrder);  // sort with latest files first (0-based)

	//The models
	dirModel_=new OutputModel(this);
	dirSortModel_=new OutputSortModel(this);
	dirSortModel_->setSourceModel(dirModel_);
	dirSortModel_->setDynamicSortFilter(true);

	dirView_->setModel(dirSortModel_);

	//When the selection changes in the view
	connect(dirView_->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
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

	//Editor font
	textEdit_->setFontProperty(VConfig::instance()->find("panel.output.font"));
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

		//Get dir contents
		dirProvider_->info(info_);

		//Start contents update timer
		updateDirTimer_->start();
	}
}

std::string OutputItemWidget::currentFullName() const
{
	QModelIndex current=dirSortModel_->mapToSource(dirView_->currentIndex());

	std::string fullName;
	OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);

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
	messageLabel_->stopLoadLabel();
	fileLabel_->clear();
	textEdit_->clear();

	//Get the latest file contents
	infoProvider_->info(info_);
}

void OutputItemWidget::getCurrentFile()
{
	messageLabel_->hide();
	messageLabel_->stopLoadLabel();
	fileLabel_->clear();
	textEdit_->clear();

	if(info_ && info_.get())
	{
		std::string fullName=currentFullName();
		OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
		op->file(fullName);
	}
}

void OutputItemWidget::clearContents()
{
	updateDirTimer_->stop();

	InfoPanelItem::clear();
	dirProvider_->clear();

	messageLabel_->hide();
	messageLabel_->stopLoadLabel();
	fileLabel_->clear();
	textEdit_->clear();

	enableDir(false);
}

void OutputItemWidget::infoReady(VReply* reply)
{
	//------------------------
	// From output provider
	//------------------------

	if(reply->sender() == infoProvider_)
	{
		//messageLabel_->stopLoadLabel();

		//For some unknown reason the textedit font, although it is properly set in the constructor,
		//is reset to default when we first call infoready. So we need to set it again!!
		textEdit_->updateFont();

		bool hasMessage=false;
		if(reply->hasWarning())
		{
			messageLabel_->showWarning(QString::fromStdString(reply->warningText()));
			hasMessage=true;
		}
		else if(reply->hasInfo())
		{
			messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
			hasMessage=true;
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

		QTime stopper;
		stopper.start();

		//If the info is stored in a tmp file
		if(f && f.get())
		{
			if(f->storageMode() == VFile::MemoryStorage)
			{
				//messageLabel_->hide();

				QString s(f->data());
				textEdit_->setPlainText(s);
			}
			else
			{
				QFile file(QString::fromStdString(f->path()));
				file.open(QIODevice::ReadOnly);
				QFileInfo fInfo(file);

				if(fInfo.size() > 20*1024*1024)
				{
					messageLabel_->startLoadLabel();
				}

				messageLabel_->showInfo("Loading file into text editor ....");
				QApplication::processEvents();

				//This was the fastest implementation for files up to 125 Mb
				uchar *d=file.map(0,fInfo.size());
				QString str((char*)d);
				textEdit_->document()->setPlainText(str);
				file.unmap(d);

				hasMessage=false;
			}
		}
		//If the info is stored as a string in the reply object
		else
		{
			QString s=QString::fromStdString(reply->text());
			textEdit_->setPlainText(s);
		}

		searchOnReload();

		if(f && f.get())
		{
			f->setWidgetLoadDuration(stopper.elapsed());
		}

		if(!hasMessage)
		{
			messageLabel_->hide();
		}
		messageLabel_->stopLoadLabel();


		//Update the file label
		fileLabel_->update(reply);
	}

	//------------------------
	// From output dir provider
	//------------------------
	else
	{
		//Update the dir widget and select the proper file in the list
		updateDir(reply->directory(),true);
	}
}

void OutputItemWidget::infoProgress(VReply* reply)
{
	messageLabel_->showInfo(QString::fromStdString(reply->infoText()));
	messageLabel_->startLoadLabel();

	//updateDir(true);
}

void OutputItemWidget::infoFailed(VReply* reply)
{
	if(reply->sender() == infoProvider_)
	{
		QString s=QString::fromStdString(reply->errorText());

		messageLabel_->showError(s);
		messageLabel_->stopLoadLabel();

		//Update the file label
		fileLabel_->update(reply);

		//updateDir(true);
	}
}

void OutputItemWidget::on_reloadTb__clicked()
{
	userClickedReload_ = true;
	getLatestFile();
	userClickedReload_ = false;
}

//------------------------------------
// Directory contents
//------------------------------------

void OutputItemWidget::updateDir(VDir_ptr dir,bool restartTimer)
{
	if(restartTimer)
		updateDirTimer_->stop();

	bool status=(dir && dir.get());

	if(status)
	{
		std::string fullName=currentFullName();

		dirView_->selectionModel()->clearSelection();
		dirModel_->setData(dir);
		dirWidget_->show();

		//Try to preserve the selection
		ignoreOutputSelection_=true;
		dirView_->setCurrentIndex(dirSortModel_->fullNameToIndex(fullName));
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

void OutputItemWidget::updateDir(bool restartTimer)
{
	dirProvider_->info(info_);

	//Remember the selection
	//std::string fullName=currentFullName();
	//updateDir(restartTimer,fullName);
}

void OutputItemWidget::updateDir(bool restartTimer,const std::string& selectFullName)
{
	/*if(restartTimer)
		updateDirTimer_->stop();

	OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);
	VDir_ptr dir=op->directory();

	bool status=(dir && dir.get());

	if(status)
	{
		dirView_->selectionModel()->clearSelection();
		dirModel_->setData(dir);
		dirWidget_->show();

		//Try to preserve the selection
		ignoreOutputSelection_=true;
		dirView_->setCurrentIndex(dirSortModel_->fullNameToIndex(selectFullName));
		ignoreOutputSelection_=false;
	}
	else
	{
		dirWidget_->hide();
		dirModel_->clearData();
	}

	if(restartTimer)
		updateDirTimer_->start(updateDirTimeout_);*/
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

//---------------------------------------------
// Search
//---------------------------------------------

void OutputItemWidget::on_searchTb__clicked()
{
	searchLine_->setVisible(true);
	searchLine_->setFocus();
	searchLine_->selectAll();
}

void OutputItemWidget::on_gotoLineTb__clicked()
{
	textEdit_->gotoLine();
}

// search for a highlight any of the pre-defined keywords so that
// the (probably) most important piece of information is highlighted
bool OutputItemWidget::automaticSearchForKeywords()
{
	bool found = false;
	QTextDocument::FindFlags findFlags = QTextDocument::FindBackward;
	QTextCursor cursor(textEdit_->textCursor());
	cursor.movePosition(QTextCursor::End);


	QRegExp regexp("--(abort|complete)");
	QTextCursor findCursor = textEdit_->document()->find(regexp, cursor, findFlags);  // perform the search
	found = (!findCursor.isNull());
	if (found)
		textEdit_->setTextCursor(findCursor);

/*
	QStringList keywords;
	keywords << "--abort" << "--complete";// << "xabort" << "xcomplete"
	         << "System Billing Units";

	// find any of the keywords and stop at the first one
	int i = 0;
	while (!found && i < keywords.size())
	{
		cursor.movePosition(QTextCursor::End);
		textEdit_->setTextCursor(cursor);
		found = textEdit_->findString(keywords.at(i), findFlags);
		i++;
	}
*/
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
		searchLine_->slotHighlight();
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

//-----------------------------------------
// Fontsize management
//-----------------------------------------

void OutputItemWidget::on_fontSizeUpTb__clicked()
{
	//We need to call a custom slot here instead of "zoomIn"!!!
	textEdit_->slotZoomIn();
}

void OutputItemWidget::on_fontSizeDownTb__clicked()
{
	//We need to call a custom slot here instead of "zoomOut"!!!
	textEdit_->slotZoomOut();
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");
