//============================================================================
// Copyright 2014 ECMWF.
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
#include "TextEdit.hpp"
#include "TextPagerWidget.hpp"

OutputBrowser::OutputBrowser(QWidget* parent) : QWidget(parent)
{
	QVBoxLayout *vb=new QVBoxLayout(this);
	vb->setContentsMargins(0,0,0,0);

	stacked_=new QStackedWidget(this);
	vb->addWidget(stacked_);

	//Basic textedit
	textEdit_=new TextEdit(this);
	textEdit_->setReadOnly(true);

	//This highlighter only works for jobs
	jobHighlighter_=new Highlighter(textEdit_->document(),"job");
	jobHighlighter_->setDocument(NULL);

	//Pager for very large files
	textPager_=new TextPagerWidget(this);
	//textPager_->setReadOnly(true);

	stacked_->addWidget(textEdit_);
	stacked_->addWidget(textPager_);

	stacked_->setCurrentIndex(BasicIndex);
}

void OutputBrowser::clear()
{
	textEdit_->clear();
	textPager_->clear();
}

void OutputBrowser::loadFile(QString fileName)
{
	QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QFileInfo fInfo(file);
    qint64 fSize=fInfo.size();

	if(fSize > 20*1024*1024)
	{
		stacked_->setCurrentIndex(PagerIndex);

		TextPagerDocument::DeviceMode mode = TextPagerDocument::Sparse;
		textPager_->load(fileName, mode);
	}
	else
	{
		stacked_->setCurrentIndex(BasicIndex);

		//This was the fastest implementation for files up to 125 Mb
		uchar *d=file.map(0,fSize);
		QString str((char*)d);
		textEdit_->document()->setPlainText(str);
		file.unmap(d);
	}
}

void OutputBrowser::loadText(QString text)
{
	stacked_->setCurrentIndex(BasicIndex);
	textEdit_->setPlainText(text);
}

void OutputBrowser::adjustHighlighter(QString fileName)
{
	//For job files we set the proper highlighter
	if(fileName.contains(".job"))
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

}


bool OutputBrowser::automaticSearchForKeywords()
{
	bool found = false;

	if(stacked_->currentIndex() == BasicIndex)
	{
		QTextDocument::FindFlags findFlags = QTextDocument::FindBackward;
		QTextCursor cursor(textEdit_->textCursor());
		cursor.movePosition(QTextCursor::End);

		QRegExp regexp("--(abort|complete)");
		QTextCursor findCursor = textEdit_->document()->find(regexp, cursor, findFlags);  // perform the search
		found = (!findCursor.isNull());
		if (found)
			textEdit_->setTextCursor(findCursor);
	}

#if 0
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
#endif

	return found;
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
