//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputBrowser.hpp"

#include <QVBoxLayout>

#include "Highlighter.hpp"
#include "MessageLabel.hpp"
#include "PlainTextEdit.hpp"
#include "PlainTextSearchInterface.hpp"
#include "TextEditSearchLine.hpp"
#include "TextPager/TextPagerSearchInterface.hpp"
#include "TextPagerWidget.hpp"
#include "DirectoryHandler.hpp"
#include "UserMessage.hpp"


int OutputBrowser::minPagerTextSize_=1*1024*1024;
int OutputBrowser::minPagerSparseSize_=30*1024*1024;
int OutputBrowser::minConfirmSearchSize_=5*1024*1024;

OutputBrowser::OutputBrowser(QWidget* parent) :
    QWidget(parent),
    lastPos_(0)
{
    QVBoxLayout *vb=new QVBoxLayout(this);
    vb->setContentsMargins(0,0,0,0);
    vb->setSpacing(2);

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

    textEditSearchInterface_=new PlainTextSearchInterface();
    textEditSearchInterface_->setEditor(textEdit_);

    //This highlighter only works for jobs
    jobHighlighter_=new Highlighter(textEdit_->document(),"job");
    jobHighlighter_->setDocument(NULL);

    //Pager for very large files
    textPager_=new TextPagerWidget(this);
    //textEdit_->setReadOnly(true);

    textPagerSearchInterface_=new TextPagerSearchInterface();
    textPagerSearchInterface_->setEditor(textPager_->textEditor());

    stacked_->addWidget(textEdit_);
    stacked_->addWidget(textPager_);

    stacked_->setCurrentIndex(BasicIndex);
    searchLine_->hide();

    connect(searchLine_,SIGNAL(visibilityChanged()),
          this,SLOT(showConfirmSearchLabel()));
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

void OutputBrowser::clear()
{
    if(stacked_->currentIndex() == BasicIndex)
        cursorCache_[currentSourceFile_].pos_=textEdit_->textCursor().position();
    else
        cursorCache_[currentSourceFile_].pos_=textPager_->textEditor()->textCursor().position();

    currentSourceFile_.clear();

    textEdit_->clear();
	textPager_->clear();
    file_.reset();
}

void OutputBrowser::changeIndex(IndexType indexType,qint64 fileSize)
{
    if(indexType == BasicIndex)
    {
        stacked_->setCurrentIndex(indexType);
        searchLine_->setConfirmSearch(false);
        searchLine_->setSearchInterface(textEditSearchInterface_);
        //confirmSearchLabel_->clear();
        //confirmSearchLabel_->hide();

        textPager_->clear();
    }
    else
    {
        stacked_->setCurrentIndex(indexType);
        searchLine_->setConfirmSearch(fileSize >=minConfirmSearchSize_);
        searchLine_->setSearchInterface(textPagerSearchInterface_);
        //confirmSearchLabel_->show();
        //confirmSearchLabel_->showWarning(searchLine_->confirmSearchText());
        textEdit_->clear();
    }

    showConfirmSearchLabel();
}

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

        //Set the cursor position from the cache
        updateCursorFromCache(file_->sourcePath());
    }
    else
    {
        QString s(file_->data());
        loadText(s,QString::fromStdString(file_->sourcePath()),true);
    }
}

void OutputBrowser::loadFile(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QFileInfo fInfo(file);
    qint64 fSize=fInfo.size();
    
    if(!isJobFile(fileName) && fSize >= minPagerTextSize_)
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
    
    if(!isJobFile(fileName) && txtSize > minPagerTextSize_)
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
    updateCursorFromCache(fileName.toStdString());
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


void OutputBrowser::updateCursorFromCache(const std::string& sourcePath)
{
#if 0
    //Set the cursor position from the cache
    QMap<std::string,CursorCacheItem>::const_iterator it=cursorCache_.find(sourcePath);
    if(it != cursorCache_.end())
    {
        setCursorPos(it.value().pos_);
    }
    currentSourceFile_=file_->sourcePath();
#endif
}

bool OutputBrowser::isJobFile(QString fileName)
{
    return fileName.contains(".job");
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
    else
       textPager_->gotoLine(); 
}

void OutputBrowser::showSearchLine()
{
	searchLine_->setVisible(true);
	searchLine_->setFocus();
	searchLine_->selectAll();
}

void OutputBrowser::searchOnReload(bool userClickedReload)
{
	searchLine_->searchOnReload(userClickedReload);
}

void OutputBrowser::setFontProperty(VProperty* p)
{
	textEdit_->setFontProperty(p);
	textPager_->setFontProperty(p);
}

void OutputBrowser::updateFont()
{
	textEdit_->updateFont();
}

void OutputBrowser::zoomIn()
{
	textEdit_->slotZoomIn();
	textPager_->zoomIn();
}

void OutputBrowser::zoomOut()
{
	textEdit_->slotZoomOut();
	textPager_->zoomOut();
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
    else
    {
        textPager_->textEditor()->setCursorPosition(pos);
    }
}
