//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "FileInfoLabel.hpp"

#include <QDateTime>
#include <QVariant>

#include "TextFormat.hpp"
#include "VFileInfo.hpp"
#include "VReply.hpp"

static QColor keyColour(39,49,101);
static QColor errorColour(255,0,0);
static QColor dateColour(34,107,138);
static QColor largeFileColour(Qt::red);

FileInfoLabel::FileInfoLabel(QWidget* parent) : QLabel(parent)
{
	//Define id for the css
	setProperty("fileInfo","1");
	setWordWrap(true);

	setMargin(2);
	setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

	//Other settings
    setAutoFillBackground(true);

    setFrameShape(QFrame::StyledPanel);
    setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);
}

void FileInfoLabel::update(VReply* reply,QString extraText)
{
	if(!reply)
    {
        clear();
        fullText_.clear();
        compactText_.clear();
        setToolTip("");
        return;
    }

	QString labelText;
	QString ttText;
	QString s;

	QString fileName=QString::fromStdString(reply->fileName());

	if(fileName.isEmpty())
	{
        s=Viewer::formatBoldText("File: ",keyColour) + Viewer::formatText(" ??? ", errorColour);
		setText(s);
        fullText_.clear();
        compactText_.clear();
		setToolTip(QString());
		return;
	}

	//Name
    labelText=Viewer::formatBoldText("File: ",keyColour);
    labelText+=fileName;

	s="";

	//Local read
	if(reply->fileReadMode() == VReply::LocalReadMode)
	{
        VFile_ptr f=reply->tmpFile();
        if(f)
        {
            VFileInfo fInfo(QString::fromStdString(f->path()));
            if(fInfo.exists())
            {
                labelText+=Viewer::formatBoldText(" Size: ",keyColour) +
                           formatFileSize(fInfo.formatSize(),fInfo.size());
                s+=Viewer::formatBoldText(" Modified: ",keyColour.name()) + fInfo.formatModDate();

            }
        }
        else
        {
            VFileInfo f(fileName);
            if(f.exists())
            {
                labelText+=Viewer::formatBoldText(" Size: ",keyColour);
                labelText+=formatFileSize(f.formatSize(),f.size());
                s+=Viewer::formatBoldText(" Modified: ",keyColour) + f.formatModDate();
            }
         }

         s+="<br>";
         s+=Viewer::formatBoldText("Source: ",keyColour) + " read from disk";

         if(f)
         {          
             s+=Viewer::formatBoldText(" at ",keyColour) + formatDate(f->fetchDate());
         }

         if(!reply->fileReadMethod().empty())
         {
            s+=Viewer::formatBoldText(" Lookup method: ",keyColour) + QString::fromStdString(reply->fileReadMethod());
         }

	}
	else if(reply->fileReadMode() == VReply::ServerReadMode)
	{
        VFile_ptr f=reply->tmpFile();
        if(f)
        {
            if(f->storageMode() == VFile::MemoryStorage)
            {
                labelText+=Viewer::formatBoldText(" Size: ",keyColour);
                labelText+=formatFileSize(VFileInfo::formatSize(f->dataSize()),f->dataSize());
            }
            else
            {
                VFileInfo fInfo(QString::fromStdString(f->path()));
                if(fInfo.exists())
                {                   
                    labelText+=Viewer::formatBoldText(" Size: ",keyColour);
                    labelText+=formatFileSize(fInfo.formatSize(),fInfo.size());
                }
            }

            s+="<br>";
            s+=Viewer::formatBoldText("Source: ",keyColour) + QString::fromStdString(f->fetchModeStr());
            s+=Viewer::formatBoldText(" at ",keyColour) + formatDate(f->fetchDate());

            int rowLimit=f->truncatedTo();
            if(rowLimit >= 0)
            {
                s+=" (<i>text truncated to last " + QString::number(rowLimit) + " lines</i>)";
            }            
        }
        else if(reply->status() == VReply::TaskDone)
        {         
            s+="<br>";
            s+=Viewer::formatBoldText("Source: ",keyColour) + " fetched from server " +
               Viewer::formatBoldText(" at ",keyColour) + formatDate(QDateTime::currentDateTime());

            int rowLimit=reply->readTruncatedTo();
            if(rowLimit >= 0)
            {
                s+=" (<i>text truncated to last " + QString::number(rowLimit) + " lines</i>)";
            }            
        }
        else
        {           
            s+="<br>Fetch attempted from server" + Viewer::formatBoldText(" at ",keyColour)  +
                    formatDate(QDateTime::currentDateTime());
        }
	}

	else if(reply->fileReadMode() == VReply::LogServerReadMode)
	{
        VFile_ptr f=reply->tmpFile();
        if(f)
        {
            //Path + size
            if(f->storageMode() == VFile::MemoryStorage)
			{
                labelText+=Viewer::formatBoldText(" Size: ",keyColour);
                labelText+=formatFileSize(VFileInfo::formatSize(f->dataSize()),f->dataSize());
			}
			else
			{
                VFileInfo fInfo(QString::fromStdString(f->path()));
                if(fInfo.exists())
				{					
                    labelText+=Viewer::formatBoldText(" Size: ",keyColour);
                    labelText+=formatFileSize(fInfo.formatSize(),fInfo.size());
				}
			}

			s+="<br>";

            //Source
            s+=Viewer::formatBoldText("Source: ",keyColour);

            if(f->cached())
            {
                s+="[from cache] ";
            }
            s+=QString::fromStdString(f->fetchModeStr());
            s+=" (took " + QString::number(static_cast<float>(f->transferDuration())/1000.,'f',1) + " s)";           
            s+=Viewer::formatBoldText(" at ",keyColour) + formatDate(f->fetchDate());
        }
	}

	ttText=s;
	labelText += ttText;
	if(!extraText.isEmpty())
	{
		labelText +=" <i>" + extraText + "</i>";
	}

    fullText_ = labelText;
    QFileInfo fInfo(fileName);
    compactText_ = Viewer::formatBoldText("File: ",keyColour) + fInfo.fileName();

    setText((compact_?compactText_:fullText_));
    setToolTip(buildTooltipText());
}

void FileInfoLabel::setCompact(bool st)
{
    if (st != compact_) {
        compact_ = st;
        setText((compact_?compactText_:fullText_));
        setToolTip(buildTooltipText());
    }
}

QString FileInfoLabel::buildTooltipText()
{
    if (compact_) {
        QString s = fullText_;
        s = s.replace(keyColour.name(), QColor(86,182,194).name());
        s = s.replace(dateColour.name(), QColor(214,149,69).name());
        return s;
    }
    return {};
}


QString FileInfoLabel::formatDate(QDateTime dt)
{
    QString s=dt.toString("yyyy-MM-dd") + "&nbsp;&nbsp;" +dt.toString("HH:mm:ss");
    return Viewer::formatBoldText(s,dateColour);
}

QString FileInfoLabel::formatFileSize(QString str,qint64 size)
{
	if(size > 10*1024*1024)
        return Viewer::formatText(str,largeFileColour);
    return str;
}

//=============================================
//
//  DirInfoLabel
//
//=============================================

void DirInfoLabel::update(VReply* reply)
{   
    QDateTime dt;
    if(reply)
    {
        std::vector<VDir_ptr> dVec=reply->directories();
        if(dVec.empty())
        {
            dt=QDateTime::currentDateTime();
        }
        //take the last item
        else
        {
            dt=dVec[dVec.size()-1]->fetchDate();
        }
    }
    else
    {
        dt=QDateTime::currentDateTime();
    }

    QColor col(39,49,101);
    QString s="Directory listing updated at " + Viewer::formatBoldText(" at ",col) +
            dt.toString("yyyy-MM-dd HH:mm:ss");
    setText(s);
}
