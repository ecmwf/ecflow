//============================================================================
// Copyright 2009-2017 ECMWF.
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
        return;
    }

	QString labelText;
	QString ttText;
	QString s;

	QColor col(39,49,101);
    QColor colText(30,30,30);
	QColor colErr(255,0,0);

	QString fileName=QString::fromStdString(reply->fileName());

	if(fileName.isEmpty())
	{
        s=Viewer::formatBoldText("File: ",col) + Viewer::formatText(" ??? ",colErr);
		setText(s);
		setToolTip(QString());
		return;
	}

	//Name
    labelText=Viewer::formatBoldText("File: ",col);
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
                labelText+=Viewer::formatBoldText(" Size: ",col) +
                           formatFileSize(fInfo.formatSize(),fInfo.size());
                s+=Viewer::formatBoldText(" Modified: ",col.name()) + fInfo.formatModDate();

            }
        }
        else
        {
            VFileInfo f(fileName);
            if(f.exists())
            {
                labelText+=Viewer::formatBoldText(" Size: ",col);
                labelText+=formatFileSize(f.formatSize(),f.size());
                s+=Viewer::formatBoldText(" Modified: ",col) + f.formatModDate();
            }
         }

         s+="<br>";
         s+=Viewer::formatBoldText("Source: ",col) + " read from disk";

         if(f)
         {          
             s+=Viewer::formatBoldText(" at ",col) + formatDate(f->fetchDate());
         }

	}
	else if(reply->fileReadMode() == VReply::ServerReadMode)
	{
        VFile_ptr f=reply->tmpFile();
        if(f)
        {
            if(f->storageMode() == VFile::MemoryStorage)
            {
                labelText+=Viewer::formatBoldText(" Size: ",col);
                labelText+=formatFileSize(VFileInfo::formatSize(f->dataSize()),f->dataSize());
            }
            else
            {
                VFileInfo fInfo(QString::fromStdString(f->path()));
                if(fInfo.exists())
                {                   
                    labelText+=Viewer::formatBoldText(" Size: ",col);
                    labelText+=formatFileSize(fInfo.formatSize(),fInfo.size());
                }
            }

            s+="<br>";
            s+=Viewer::formatBoldText("Source: ",col) + QString::fromStdString(f->fetchModeStr());         
            s+=Viewer::formatBoldText(" at ",col) + formatDate(f->fetchDate());

            int rowLimit=f->truncatedTo();
            if(rowLimit >= 0)
            {
                s+=" (<i>text truncated to last " + QString::number(rowLimit) + " lines</i>)";
            }            
        }
        else if(reply->status() == VReply::TaskDone)
        {         
            s+="<br>";
            s+=Viewer::formatBoldText("Source: ",col) + " fetched from server " +
               Viewer::formatBoldText(" at ",col) + formatDate(QDateTime::currentDateTime());

            int rowLimit=reply->readTruncatedTo();
            if(rowLimit >= 0)
            {
                s+=" (<i>text truncated to last " + QString::number(rowLimit) + " lines</i>)";
            }            
        }
        else
        {           
            s+="<br>Fetch attempted from server" + Viewer::formatBoldText(" at ",col)  +
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
                labelText+=Viewer::formatBoldText(" Size: ",col);
                labelText+=formatFileSize(VFileInfo::formatSize(f->dataSize()),f->dataSize());
			}
			else
			{
                VFileInfo fInfo(QString::fromStdString(f->path()));
                if(fInfo.exists())
				{					
                    labelText+=Viewer::formatBoldText(" Size: ",col);
                    labelText+=formatFileSize(fInfo.formatSize(),fInfo.size());
				}
			}

			s+="<br>";

            //Source
            s+=Viewer::formatBoldText("Source: ",col);

            if(f->cached())
            {
                s+="[from cache] ";
            }
            s+=QString::fromStdString(f->fetchModeStr());
            s+=" (took " + QString::number(static_cast<float>(f->transferDuration())/1000.,'f',1) + " s)";           
            s+=Viewer::formatBoldText(" at ",col) + formatDate(f->fetchDate());
        }
	}

	ttText=s;
	labelText += ttText;
	if(!extraText.isEmpty())
	{
		labelText +=" <i>" + extraText + "</i>";
	}

	setText(labelText);
}

QString FileInfoLabel::formatDate(QDateTime dt) const
{
    QColor col(34,107,138);
    QString s=dt.toString("yyyy-MM-dd") + "&nbsp;&nbsp;" +dt.toString("HH:mm:ss");
    return Viewer::formatBoldText(s,col);
}

QString FileInfoLabel::formatFileSize(QString str,qint64 size) const
{
	if(size > 10*1024*1024)
        return Viewer::formatText(str,QColor(Qt::red));
    return str;
}

void DirInfoLabel::update(VReply* reply)
{
    VDir_ptr dir=reply->directory();

    if(!dir)
    {
        clear();
        return;
    }

	QString s;

    QColor col(39,49,101);
    QColor colErr(255,0,0);

	QString dirName=QString::fromStdString(dir->path());

	if(dirName.isEmpty())
	{
        s=Viewer::formatBoldText("Directory: ",col) + Viewer::formatText(" ??? ",colErr);
		setText(s);
		return;
	}

    //Local read
    if(dir->fetchMode() == VDir::LocalFetchMode)
    {
        s+=Viewer::formatBoldText("Directory: ",col) + " read from disk";

        QString dt=dir->fetchDate().toString("yyyy-MM-dd HH:mm:ss");
        s+=Viewer::formatBoldText(" at ",col) + dt;

    }
    else if(dir->fetchMode() == VDir::LogServerFetchMode)
    {
        s+=Viewer::formatBoldText("Directory: ",col) + QString::fromStdString(dir->fetchModeStr());

        QString dt=dir->fetchDate().toString("yyyy-MM-dd HH:mm:ss");
        s+=Viewer::formatBoldText(" at ",col) +  dt;
    }

	setText(s);
}
