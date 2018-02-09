//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "ServerHandler.hpp"
#include "TextFormat.hpp"
#include "TextPagerEdit.hpp"
#include "VConfig.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <QClipboard>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMovie>
#include <QTime>
#include <QTimer>
#include <QWidgetAction>
#include <QFileDialog>

#define _UI_OUTPUTITEMWIDGET_DEBUG

int OutputItemWidget::updateDirTimeout_=1000*60;

OutputItemWidget::OutputItemWidget(QWidget *parent) :
	QWidget(parent),
	userClickedReload_(false),
    ignoreOutputSelection_(false),
    dirColumnsAdjusted_(false),
    submittedWarning_(false)
{
    //We try to keep the contents when clicking away
    //tryToKeepContents_=true;

    setupUi(this);

	//--------------------------------
	// The file contents
	//--------------------------------

    messageLabel_->hide();
    messageLabel_->setShowTypeTitle(false);
    warnLabel_->hide();
    fileLabel_->setProperty("fileInfo","1");

	infoProvider_=new OutputFileProvider(this);

	//--------------------------------
	// The dir contents
	//--------------------------------

    dirMessageLabel_->hide();
    dirMessageLabel_->setShowTypeTitle(false);
    //dirLabel_->hide();
    dirLabel_->setProperty("fileInfo","1");

	dirProvider_=new OutputDirProvider(this);

	//The view
    OutputDirLitsDelegate* dirDelegate=new OutputDirLitsDelegate(this);
    dirView_->setItemDelegate(dirDelegate);
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
    dirSortModel_->setSortRole(Qt::UserRole);
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

    browser_->setFilterButtons(filterTb_,filterOptionTb_);
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

    //set the info
    adjust(info);

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
    dirMessageLabel_->hide();
    fetchInfo_->clearInfo();

    //Get the latest file contents
    infoProvider_->info(info_);

    updateDir(false);  // get the directory listing
}

void OutputItemWidget::getCurrentFile(bool doReload)
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
#ifdef _UI_OUTPUTITEMWIDGET_DEBUG
        UiLog().dbg()  << "output selected: " << fullName;
#endif
        OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
        op->file(fullName,!doReload);

        updateDir(false);  // get the directory listing
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
    browser_->clear();
    reloadTb_->setEnabled(true);
    userClickedReload_ = false;
    fetchInfo_->clearInfo();
    submittedWarning_=false;
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

        //TODO: make it possible to show warning and info at the same time
        bool hasMessage=false;
        submittedWarning_=false;
        OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
        if(reply->fileName() == op->joboutFileName() && !op->isTryNoZero(reply->fileName()) &&
           info_ && info_->isNode() && info_->node() && info_->node()->isSubmitted())
        {
            hasMessage=true;
            submittedWarning_=true;
            messageLabel_->showWarning("This is the current job output (as defined by variable ECF_JOBOUT), but \
                   because the node status is <b>submitted</b> it may contain the ouput from a previous run!");
        }
        else
        {
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
        //Update the selection in the dir list according to the file
        if(f)
        {          
            setCurrentInDir(f->sourcePath());
        }
#if 0
        if(reply->tmpFile() && reply->fileReadMode() == VReply::LocalReadMode &&
            info_ && !info_->server()->isLocalHost())
        {
            QString msg="The output file was read <b>from disk</b> but the server's \
                       host (" + QString::fromStdString(info_->server()->host()) +
                       ") is not running on the local machine. If the path is machine-specific (e.g. /tmp) \
                       and there exists a file with the same path on the local machine, then\
                       this will have been read instead.";

            warnLabel_->showWarning(msg);
        }
#endif


        fetchInfo_->setInfo(reply,info_);
    }

    //------------------------
    // From output dir provider
    //------------------------
    else
    {    
        //We do not display info/warning here! The dirMessageLabel_ is not part of the dirWidget_
        //and is only supposed to display error messages!

        enableDir(true);

        //Update the dir widget and select the proper file in the list
        updateDir(reply->directories(),true);

        //Update the dir label
        dirLabel_->update(reply);

        //Enable the update button
        dirReloadTb_->setEnabled(true);
#if 0
        //Even though infoReady is called there could be some errors since we could
        //try to read multiple directories
        displayDirErrors(reply->errorTextVec());
#endif
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
    //File
    if(reply->sender() == infoProvider_)
	{
		QString s=QString::fromStdString(reply->errorText());
		messageLabel_->showError(s);       
        messageLabel_->stopProgress();
        submittedWarning_=false;

		//Update the file label
		fileLabel_->update(reply);

        userClickedReload_ = false;
        reloadTb_->setEnabled(true);       

        fetchInfo_->setInfo(reply,info_);
	}
    //Directories
    else
    {
        //We do not have directories
        enableDir(false);

        displayDirErrors(reply->errorTextVec());

        dirReloadTb_->setEnabled(true);

        //the timer is stopped. It will be restarted again if we get a local file or
        //a file via the logserver
        updateDirTimer_->stop();
    }
}

void OutputItemWidget::on_reloadTb__clicked()
{
	userClickedReload_ = true;
    reloadTb_->setEnabled(false);
    getCurrentFile(true);
    //userClickedReload_ = false;
}

//------------------------------------
// Directory contents
//------------------------------------

void OutputItemWidget::on_dirReloadTb__clicked()
{
    dirReloadTb_->setEnabled(false);
    updateDir(false);  // get the directory listing
}

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

void OutputItemWidget::updateDir(const std::vector<VDir_ptr>& dirs,bool restartTimer)
{
UI_FUNCTION_LOG

    if(restartTimer)
		updateDirTimer_->stop();

    bool status=false;
    for(std::size_t i=0; i < dirs.size(); i++)
    {
        if(dirs[i] && dirs[i]->count() > 0)
        {
            status=true;
            break;
        }
    }

	if(status)
	{
        OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
        op->setDirectories(dirs);

        std::string fullName=currentFullName();

		dirView_->selectionModel()->clearSelection();
        dirModel_->setData(dirs,op->joboutFileName());
        //dirWidget_->show();

        //Adjust column width
        if(!dirColumnsAdjusted_)
        {
            dirColumnsAdjusted_=true;
            for(int i=0; i< dirModel_->columnCount()-1; i++)               
                dirView_->resizeColumnToContents(i);

            if(dirModel_->columnCount() > 1)
                dirView_->setColumnWidth(1,dirView_->columnWidth(0));

        }
#ifdef _UI_OUTPUTITEMWIDGET_DEBUG
        UiLog().dbg() << " dir item count=" << dirModel_->rowCount();
#endif
		//Try to preserve the selection
		ignoreOutputSelection_=true;
		dirView_->setCurrentIndex(dirSortModel_->fullNameToIndex(fullName));
		ignoreOutputSelection_=false;
	}
	else
	{
        //dirWidget_->hide();
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

void OutputItemWidget::slotUpdateDir()
{
	updateDir(false);
}

void OutputItemWidget::enableDir(bool status)
{
	if(status)
	{       
        dirWidget_->show();
        dirMessageLabel_->hide();
        reloadTb_->setEnabled(true);
	}
	else
	{
        dirWidget_->hide();
        dirModel_->clearData();
        dirMessageLabel_->show();
	}
}

void OutputItemWidget::displayDirErrors(const std::vector<std::string>& errorVec)
{   
    QString s;
    if(errorVec.size() > 0)
    {
        QColor col(70,71,72);
        s=Viewer::formatBoldText("Output directory: ",col);

        if(errorVec.size() > 1)
        {
            for(size_t i=0; i < errorVec.size(); i++)
                s+=Viewer::formatBoldText("[" + QString::number(i+1) + "] ",col) +
                    QString::fromStdString(errorVec[i]) + ". &nbsp;&nbsp;";
        }
        else if(errorVec.size() == 1)
            s+=QString::fromStdString(errorVec[0]);
    }

    if(!s.isEmpty())
        dirMessageLabel_->showError(s);
    else
        dirMessageLabel_->hide();
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
        getCurrentFile(false);
}

//---------------------------------------------
// Filter
//---------------------------------------------

void OutputItemWidget::on_filterTb__clicked()
{
    browser_->showFilterLine();
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

//-----------------------------------------
// Save local copy of file
//-----------------------------------------

void OutputItemWidget::on_saveFileAsTb__clicked()
{
	if (browser_->isFileLoaded())
	{
		QString fileName = QFileDialog::getSaveFileName(this);
		if (fileName.isEmpty())
			return;

		browser_->saveCurrentFile(fileName);
	}
	else
	{
        UserMessage::message(UserMessage::INFO,true,"No file loaded!");
	}
}

//-----------------------------------------
// Copy file path
//-----------------------------------------

void OutputItemWidget::on_copyPathTb__clicked()
{
    std::string fullName=currentFullName();

    if(!fullName.empty())
    {
        QString txt=QString::fromStdString(fullName);

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
}

//-------------------------
// Update
//-------------------------

void OutputItemWidget::nodeChanged(const VNode* n, const std::vector<ecf::Aspect::Type>& aspect)
{
    //Changes in the nodes
    for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
    {
        if(*it == ecf::Aspect::STATE || *it == ecf::Aspect::DEFSTATUS ||
            *it == ecf::Aspect::SUSPENDED)
        {
            if(submittedWarning_)
               getLatestFile();
            else if(info_ && info_->node() == n && info_->node()->isSubmitted())
            {
                OutputFileProvider* op=static_cast<OutputFileProvider*>(infoProvider_);
                Q_ASSERT(op);
                if(currentFullName() == op->joboutFileName())
                    getLatestFile();
            }
            return;
        }
    }
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");
