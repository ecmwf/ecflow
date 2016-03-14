//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputItemWidget.hpp"

#include "OutputDirProvider.hpp"
#include "OutputFetchInfo.hpp"
#include "OutputFileProvider.hpp"
#include "OutputModel.hpp"
#include "PlainTextEdit.hpp"
#include "TextPagerEdit.hpp"
#include "VConfig.hpp"
#include "VReply.hpp"
#include "UserMessage.hpp"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMovie>
#include <QTime>
#include <QTimer>
#include <QWidgetAction>

int OutputItemWidget::updateDirTimeout_=1000*60;

OutputItemWidget::OutputItemWidget(QWidget *parent) :
	QWidget(parent),
	userClickedReload_(false),
	ignoreOutputSelection_(false)
{
    //We try to keep the contents when clicking away
    //tryToKeepContents_=true;

    setupUi(this);

	messageLabel_->hide();
	dirLabel_->hide();

	fileLabel_->setProperty("fileInfo","1");

	//--------------------------------
	// The file contents
	//--------------------------------

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
	browser_->setFontProperty(VConfig::instance()->find("panel.output.font"));

    fetchInfo_=new OutputFetchInfo(this);
    QWidgetAction* fetchInfoAction=new QWidgetAction(this);
    fetchInfoAction->setDefaultWidget(fetchInfo_);
    fetchInfoTb_->addAction(fetchInfoAction);
}

OutputItemWidget::~OutputItemWidget()
{
}

QWidget* OutputItemWidget::realWidget()
{
	return this;
}

void OutputItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    //enabled_=true;
	info_=info;
    userClickedReload_ = false;

    //info must be a node
    if(info_ && info_->isNode() && info_->node())
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
	if(current.isValid())
	{
		fullName=dirModel_->fullName(current);
	}
	else
	{
        OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
        fullName=op->joboutFileName();
	}

	return fullName;
}

void OutputItemWidget::getLatestFile()
{
	messageLabel_->hide();
    messageLabel_->stopProgress();
    fileLabel_->clear();
    browser_->clear();
    fetchInfo_->clearInfo();

    //Get the latest file contents
	infoProvider_->info(info_);
}

void OutputItemWidget::getCurrentFile()
{
	messageLabel_->hide();
	messageLabel_->stopLoadLabel();
    messageLabel_->stopProgress();
    fileLabel_->clear();
    browser_->clear();
    fetchInfo_->clearInfo();

    if(info_)
	{
		std::string fullName=currentFullName();
        UserMessage::message(UserMessage::DBG,false,"output selected: " + fullName);
		OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
		op->file(fullName);
	}
}

void OutputItemWidget::clearContents()
{
    updateDirTimer_->stop();
    InfoPanelItem::clear();
    enableDir(false);
    messageLabel_->hide();
    messageLabel_->stopProgress();
    fileLabel_->clear();
    browser_->clearCursorCache();
    browser_->clear();
    reloadTb_->setEnabled(true);
    userClickedReload_ = false;
    fetchInfo_->clearInfo();
}

void OutputItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SelectedChanged))
    {
        if(selected_ && !suspended_)
        {            
            slotUpdateDir();
            updateDirTimer_->start();
        }
        //If unselected we stop the dir update
        else
        {
            updateDirTimer_->stop();            
        }
    }

    if(flags.isSet(SuspendedChanged))
    {
        //Suspend
        if(suspended_)
        {
            updateDirTimer_->stop();
            reloadTb_->setEnabled(false);
            enableDir(false);
        }
        //Resume
        else
        {
            if(info_ && info_->node())
            {
                reloadTb_->setEnabled(true);
                enableDir(true);
                if(selected_)
                {
                    slotUpdateDir();
                    updateDirTimer_->start();
                }
            }
            else
            {
                clearContents();
            }
        }
    }
}

void OutputItemWidget::infoReady(VReply* reply)
{
    //------------------------
    // From output provider
    //------------------------

    if(reply->sender() == infoProvider_)
    {
        messageLabel_->stopProgress();

        //For some unknown reason the textedit font, although it is properly set in the constructor,
        //is reset to default when we first call infoready. So we need to set it again!!
        browser_->updateFont();

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

        browser_->adjustHighlighter(QString::fromStdString(reply->fileName()));

        VFile_ptr f=reply->tmpFile();

        //If the info is stored in a tmp file
        if(f)
        {
            browser_->loadFile(f);
            if(f->storageMode() == VFile::DiskStorage)
                hasMessage=false;

        }
        //If the info is stored as a string in the reply object
        else
        {            
            //QString s=QString::fromStdString(reply->text());
            //browser_->loadText(s,QString::fromStdString(reply->fileName()));
        }

        if(!hasMessage)
        {
            messageLabel_->hide();
        }
        //messageLabel_->stopLoadLabel();

        //Update the file label
        fileLabel_->update(reply);

        //Search for some keywords in the current jobout

        if(f)
        {
            //We do not have dir info so the file must be the jobout
            if(dirModel_->isEmpty())
                searchOnReload();

            //We have dir info
            else
            {
                OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
                if(reply->fileName() == op->joboutFileName())
                {
                    searchOnReload();
                }
            }
        }

        userClickedReload_ = false;
        reloadTb_->setEnabled(true);

        //If we got a local file or a file via the logserver we restart the dir update timer
        if(!suspended_ &&
           (reply->fileReadMode() == VReply::LocalReadMode ||
            reply->fileReadMode() == VReply::LogServerReadMode))
        {
            updateDirTimer_->start();
        }

        //If we got a local file or a file via the logserver we set the selection
        //int the dir list according to the file
        if(reply->fileReadMode() == VReply::LocalReadMode ||
           reply->fileReadMode() == VReply::LogServerReadMode)
        {
            //if(f)
                setCurrentInDir(f->sourcePath());
            /*else
                setCurrentInDir(reply->fileName());*/
        }

        fetchInfo_->setInfo(reply,info_);
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
    //messageLabel_->startLoadLabel();
	//updateDir(true);
}

void  OutputItemWidget::infoProgressStart(const std::string& text,int max)
{
    messageLabel_->showInfo(QString::fromStdString(text));
    messageLabel_->startProgress(max);
}

void  OutputItemWidget::infoProgress(const std::string& text,int value)
{
    messageLabel_->progress(QString::fromStdString(text),value);
}

void OutputItemWidget::infoFailed(VReply* reply)
{
    if(reply->sender() == infoProvider_)
	{
		QString s=QString::fromStdString(reply->errorText());

		messageLabel_->showError(s);
        //messageLabel_->stopLoadLabel();
        messageLabel_->stopProgress();

		//Update the file label
		fileLabel_->update(reply);

        userClickedReload_ = false;
        reloadTb_->setEnabled(true);
        //updateDir(true);

        fetchInfo_->setInfo(reply,info_);
	}
    else
    {
        //We do not have directories
        enableDir(false);
        //the timer is stopped. It will be restarted again if we get a local file or
        //a file via the logserver
        updateDirTimer_->stop();
    }
}

void OutputItemWidget::on_reloadTb__clicked()
{
	userClickedReload_ = true;
    reloadTb_->setEnabled(false);
    getLatestFile();
    //userClickedReload_ = false;
}

//------------------------------------
// Directory contents
//------------------------------------

void OutputItemWidget::setCurrentInDir(const std::string& fullName)
{
    if(!dirModel_->isEmpty())
    {
        //Try to preserve the selection
        ignoreOutputSelection_=true;
        dirView_->setCurrentIndex(dirSortModel_->fullNameToIndex(fullName));
        ignoreOutputSelection_=false;
    }
}

void OutputItemWidget::updateDir(VDir_ptr dir,bool restartTimer)
{
	if(restartTimer)
		updateDirTimer_->stop();

    bool status=(dir && dir->count() >0);

	if(status)
	{
        OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
        op->setDir(dir);

        std::string fullName=currentFullName();

		dirView_->selectionModel()->clearSelection();
        dirModel_->setData(dir,op->joboutFileName());
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
	browser_->showSearchLine();
}

void OutputItemWidget::on_gotoLineTb__clicked()
{
	browser_->gotoLine();
}


// Called when we load a new node's information into the panel, or
// when we move to the panel from another one.
// If the search box is open, then search for the first matching item;
// otherwise, search for a pre-configured list of keywords. If none
// are found, and the user has clicked on the 'reload' button then
// we just go to the last line of the output
void OutputItemWidget::searchOnReload()
{
	browser_->searchOnReload(userClickedReload_);
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
	browser_->zoomIn();
}

void OutputItemWidget::on_fontSizeDownTb__clicked()
{
	//We need to call a custom slot here instead of "zoomOut"!!!
	browser_->zoomOut();
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");
