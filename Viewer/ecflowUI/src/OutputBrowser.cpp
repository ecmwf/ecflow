//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <assert.h>
#include "OutputBrowser.hpp"

#include <QtGlobal>
#include <QProcess>
#include <QVBoxLayout>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include "Highlighter.hpp"

#include "MessageLabel.hpp"
#include "HtmlEdit.hpp"
#include "PlainTextEdit.hpp"
#include "PlainTextSearchInterface.hpp"
#include "TextEditSearchLine.hpp"
#include "TextPager/TextPagerSearchInterface.hpp"
#include "TextPagerWidget.hpp"
#include "DirectoryHandler.hpp"
#include "TextFilterWidget.hpp"
#include "UserMessage.hpp"
#include "UiLog.hpp"

int OutputBrowser::minPagerTextSize_=1*1024*1024;
int OutputBrowser::minPagerSparseSize_=30*1024*1024;
int OutputBrowser::minConfirmSearchSize_=5*1024*1024;

OutputBrowser::OutputBrowser(QWidget* parent) :
    QWidget(parent),
    searchTb_(0)
{
    QVBoxLayout *vb=new QVBoxLayout(this);
    vb->setContentsMargins(0,0,0,0);
    vb->setSpacing(2);

    //Text filter editor
    textFilter_=new TextFilterWidget(this);
    vb->addWidget(textFilter_);

    connect(textFilter_,SIGNAL(runRequested(QString,bool,bool)),
            this,SLOT(slotRunFilter(QString,bool,bool)));

    connect(textFilter_,SIGNAL(clearRequested()),
            this,SLOT(slotRemoveFilter()));

    connect(textFilter_,SIGNAL(closeRequested()),
            this,SLOT(slotRemoveFilter()));

    stacked_=new QStackedWidget(this);
    vb->addWidget(stacked_,1);

    confirmSearchLabel_=new MessageLabel(this);
    confirmSearchLabel_->setShowTypeTitle(false);
    confirmSearchLabel_->setNarrowMode(true);
    vb->addWidget(confirmSearchLabel_);

    searchLine_=new TextEditSearchLine(this);
    vb->addWidget(searchLine_);

    //Basic textedit
    textEdit_=new PlainTextEdit(this);
    textEdit_->setReadOnly(true);
    textEdit_->setWordWrapMode(QTextOption::NoWrap);
    textEdit_->setShowLineNumbers(false);

    textEditSearchInterface_=new PlainTextSearchInterface();
    textEditSearchInterface_->setEditor(textEdit_);

    //This highlighter only works for jobs
    jobHighlighter_=new Highlighter(textEdit_->document(),"job");
    jobHighlighter_->setDocument(NULL);

    //Pager for very large files
    textPager_=new TextPagerWidget(this);
    textPager_->textEditor()->setShowLineNumbers(false);

    //textEdit_->setReadOnly(true);

    textPagerSearchInterface_=new TextPagerSearchInterface();
    textPagerSearchInterface_->setEditor(textPager_->textEditor());

    //Html browser for html files
    htmlEdit_=new HtmlEdit(this);

    stacked_->addWidget(textEdit_);
    stacked_->addWidget(textPager_);
    stacked_->addWidget(htmlEdit_);

    stacked_->setCurrentIndex(BasicIndex);
    searchLine_->hide();

    connect(searchLine_,SIGNAL(visibilityChanged()),
          this,SLOT(showConfirmSearchLabel()));

    //the textfilter is is hidden by default
    textFilter_->hide();
}

OutputBrowser::~OutputBrowser()
{
    delete  textEditSearchInterface_;
    delete  textPagerSearchInterface_;

    if(jobHighlighter_ && !jobHighlighter_->parent())
    {
        delete jobHighlighter_;
    }
}

void OutputBrowser::setSearchButtons(QToolButton* searchTb)
{
    searchTb_=searchTb;
}

void OutputBrowser::setFilterButtons(QToolButton* statusTb,QToolButton* optionTb)
{
    textFilter_->setExternalButtons(statusTb,optionTb);
}

void OutputBrowser::clear()
{
    textEdit_->clear();
    textPager_->clear();
    htmlEdit_->clear();
    file_.reset();
    oriFile_.reset();
}

void OutputBrowser::changeIndex(IndexType indexType,qint64 fileSize)
{
    if(indexType == BasicIndex)
    {
        stacked_->setCurrentIndex(indexType);               
        textPager_->clear();
        htmlEdit_->clear();

        //enable and init search
        if(searchTb_) searchTb_->setEnabled(true);
        searchLine_->setConfirmSearch(false);
        searchLine_->setSearchInterface(textEditSearchInterface_);

        //enable filter
        textFilter_->setEnabledExternalButtons(true);
    }
    else if(indexType == PagerIndex)
    {
        stacked_->setCurrentIndex(indexType);          
        textEdit_->clear();
        htmlEdit_->clear();

        //enable and init search
        if(searchTb_) searchTb_->setEnabled(true);
        searchLine_->setConfirmSearch(fileSize >=minConfirmSearchSize_);
        searchLine_->setSearchInterface(textPagerSearchInterface_);

        //enable filter
        textFilter_->setEnabledExternalButtons(true);
    }
    else if(indexType == HtmlIndex)
    {
        stacked_->setCurrentIndex(indexType);
        textPager_->clear();
        textEdit_->clear();
        if(oriFile_)
            oriFile_.reset();

        //Disable search
        if(searchTb_) searchTb_->setEnabled(false);
        searchLine_->setSearchInterface(0);
        searchLine_->hide();

        //Disable filter
        textFilter_->closeIt();
        textFilter_->setEnabledExternalButtons(false);
    }

    showConfirmSearchLabel();
}

//This should only be called externally when a new output is loaded
void OutputBrowser::loadFile(VFile_ptr file)
{
    if(!file)
    {
        clear();
        return;
    }

    file_=file;
    if(file_->storageMode() == VFile::DiskStorage)
    {
        loadFile(QString::fromStdString(file_->path()));
    }
    else
    {
        QString s(file_->data());
        loadText(s,QString::fromStdString(file_->sourcePath()),true);
    }

    //Run the filter if defined
    if(textFilter_->isActive())
    {
        slotRunFilter(textFilter_->filterText(),textFilter_->isMatched(),
                      textFilter_->isCaseSensitive());
    }
}

void OutputBrowser::loadFilteredFile(VFile_ptr file)
{
    if(!file)
    {
        clear();
        return;
    }

    file_=file;
    Q_ASSERT(file_->storageMode() == VFile::DiskStorage);
    loadFile(QString::fromStdString(file_->path()));
}

void OutputBrowser::loadFile(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QFileInfo fInfo(file);
    qint64 fSize=fInfo.size();
    
    if(isHtmlFile(fileName))
    {
        changeIndex(HtmlIndex,fSize);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
        QString str=file.readAll();
        htmlEdit_->document()->setHtml(str);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif

    }
    else if(!isJobFile(fileName) && fSize >= minPagerTextSize_)
    {
        changeIndex(PagerIndex,fSize);

        TextPagerDocument::DeviceMode mode=(fSize >= minPagerSparseSize_)?
        		           TextPagerDocument::Sparse:TextPagerDocument::LoadAll;
        textPager_->load(fileName, mode);
    }
    else
    {
    	changeIndex(BasicIndex,fSize);

        adjustHighlighter(fileName);
       
        QString str=file.readAll();
        textEdit_->document()->setPlainText(str);
    }
}

void OutputBrowser::loadText(QString txt,QString fileName,bool resetFile)
{
    // prior to the ability to save local copies of files, we reset the file_ member here;
    // but now we need to keep it so that we can save a copy of it
    //if(resetFile)
       //file_.reset();

    //We estimate the size in bytes
	qint64 txtSize=txt.size()*2;
    
    if(isHtmlFile(fileName))
    {
        changeIndex(HtmlIndex,txtSize);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
        htmlEdit_->document()->setHtml(txt);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
    }
    else if(!isJobFile(fileName) && txtSize > minPagerTextSize_)
    {
        changeIndex(PagerIndex,txtSize);
        textPager_->setText(txt);
    }
    else
    {
    	changeIndex(BasicIndex,txtSize);
        adjustHighlighter(fileName);  
        textEdit_->document()->setPlainText(txt);
    }

    //Set the cursor position from the cache
    //updateCursorFromCache(fileName.toStdString());
}

void OutputBrowser::saveCurrentFile(QString &fileNameToSaveTo)
{
    assert(file_);

    if (file_->storageMode() == VFile::DiskStorage)
    {
        // if we have the file on disk, then just copy it to the new location
        std::string to = fileNameToSaveTo.toStdString();
        std::string errorMessage;
        bool ok = DirectoryHandler::copyFile(file_->path(), to, errorMessage);
        if (!ok)
        {
            UserMessage::message(UserMessage::ERROR,true,"Failed to copy file. Reason:\n " + errorMessage);
        }
    }
    else
    {
        // file in memory - just dump it to file
        QFile file(fileNameToSaveTo);
        if (file.open(QFile::WriteOnly | QFile::Text))
        {
            QTextStream out(&file);
            QString s(file_->data());
            out << s;
        }
        else
        {
            UserMessage::message(UserMessage::ERROR,true,"Failed to save file to  " + fileNameToSaveTo.toStdString());
        }
    }
}

bool OutputBrowser::isFileLoaded()
{
    return (file_ != 0);
}

bool OutputBrowser::isJobFile(QString fileName)
{
    return fileName.contains(".job");
}

bool OutputBrowser::isHtmlFile(QString fileName)
{
    return fileName.endsWith(".html");
}

void OutputBrowser::adjustHighlighter(QString fileName)
{
    //For job files we set the proper highlighter
    if(isJobFile(fileName))
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
}

void OutputBrowser::gotoLine()
{
    if(stacked_->currentIndex() == BasicIndex)
       textEdit_->gotoLine();
    else if(stacked_->currentIndex() == PagerIndex)
       textPager_->gotoLine(); 
}

void OutputBrowser::showSearchLine()
{
    if(searchLine_->hasInterface())
    {
        searchLine_->setVisible(true);
        searchLine_->setFocus();
        searchLine_->selectAll();
    }
}

void OutputBrowser::searchOnReload(bool userClickedReload)
{
    if(searchLine_->hasInterface())
    {
        searchLine_->searchOnReload(userClickedReload);
    }
}

void OutputBrowser::setFontProperty(VProperty* p)
{
	textEdit_->setFontProperty(p);
	textPager_->setFontProperty(p);
    htmlEdit_->setFontProperty(p);
}

void OutputBrowser::updateFont()
{
	textEdit_->updateFont();
}

void OutputBrowser::zoomIn()
{
	textEdit_->slotZoomIn();
	textPager_->zoomIn();
    htmlEdit_->zoomIn();
}

void OutputBrowser::zoomOut()
{
	textEdit_->slotZoomOut();
	textPager_->zoomOut();
    htmlEdit_->zoomOut();
}

void OutputBrowser::showConfirmSearchLabel()
{
	if(searchLine_->isVisible() &&  searchLine_->confirmSearch())
	{
		confirmSearchLabel_->showWarning(searchLine_->confirmSearchText());
		//confirmSearchLabel_->show();
	}
	else
	{
		confirmSearchLabel_->hide();
	}
}

void OutputBrowser::setCursorPos(qint64 pos)
{
    if(stacked_->currentIndex() == BasicIndex)
    {
        QTextCursor c=textEdit_->textCursor();
        c.setPosition(pos);
        textEdit_->setTextCursor(c);
    }
    else if(stacked_->currentIndex() == PagerIndex)
    {
        textPager_->textEditor()->setCursorPosition(pos);
    }
}

void OutputBrowser::slotRunFilter(QString filter,bool matched,bool caseSensitive)
{
    if(stacked_->currentIndex() == HtmlIndex)
        return;

    assert(file_);

    if(oriFile_)
        file_=oriFile_;

    VFile_ptr fTarget=VFile::createTmpFile(true);
    VFile_ptr fSrc=VFile_ptr();

    // file is on disk - we use it as it is
    if(file_->storageMode() == VFile::DiskStorage)
    {
        fSrc=file_;
    }
    // file in memory - just dump it to the tmp file
    else
    {
        fSrc=VFile::createTmpFile(true);
        QFile file(QString::fromStdString(fSrc->path()));
        if(file.open(QFile::WriteOnly | QFile::Text))
        {
            QTextStream out(&file);
            QString s(file_->data());
            out << s;
        }
        else
        {
        }
    }

    //At this point point fSrc must contain the text to filter
    QProcess proc;
    proc.setStandardOutputFile(QString::fromStdString(fTarget->path()));

    //Run grep to filter fSrc into fTarget
    QString extraOptions;
    if(!matched)
        extraOptions+=" -v ";
    if(!caseSensitive)
        extraOptions+=" -i ";

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    proc.start("/bin/sh",
         QStringList() <<  "-c" << "grep " + extraOptions + " -e \'" + filter  + "\' " +
         QString::fromStdString(fSrc->path()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    UiLog().dbg() << "args=" << proc.arguments().join(" ");
#endif

    if(!proc.waitForStarted(1000))
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
        QString errStr;
        UI_FUNCTION_LOG
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        errStr="Failed to start text filter using command:<br> \'" +
                             proc.program() + " " + proc.arguments().join(" ") + "\'";
#else
        errStr="Failed to start grep command!";
#endif
        UserMessage::message(UserMessage::ERROR,true,errStr.toStdString());
        fTarget.reset();
        return;
    }

    proc.waitForFinished(60000);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::restoreOverrideCursor();
#endif

    QString errStr=proc.readAllStandardError();
    if(proc.exitStatus() == QProcess::NormalExit && errStr.isEmpty())
    {
        oriFile_=file_;
        textFilter_->setStatus(fTarget->isEmpty()?(TextFilterWidget::NotFoundStatus):(TextFilterWidget::FoundStatus));        
        loadFilteredFile(fTarget);
    }
    else
    {        
        QString msg;
        UI_FUNCTION_LOG
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        msg="Failed to filter output file using command:<br> \'" +
                             proc.program() + " " + proc.arguments().join(" ") + "\'";
#else
        msg="Failed to run grep command!";
#endif
        if(!errStr.isEmpty())
            msg+="<br>Error message: " + errStr;

        UserMessage::message(UserMessage::ERROR,true,msg.toStdString());
        textFilter_->setStatus(TextFilterWidget::NotFoundStatus);
        fTarget.reset(); //delete
    }

    return;
}

void OutputBrowser::slotRemoveFilter()
{
    if(stacked_->currentIndex() == HtmlIndex)
        return;

    if(oriFile_)
    {
        loadFile(oriFile_);
        oriFile_.reset();
    }
}
